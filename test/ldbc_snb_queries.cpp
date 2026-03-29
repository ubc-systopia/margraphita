// LDBC SNB benchmark queries on Flexograph
//
// Usage: ./ldbc_snb_queries <data_dir> [graph_type]
//
// graph_type: adj | splitekey  (default: splitekey)
//
// Expects LDBC SNB CSV files (pipe-delimited) under <data_dir>/dynamic/:
//   person_0_0.csv
//   post_0_0.csv
//   person_knows_person_0_0.csv
//   post_hasCreator_person_0_0.csv
//   person_likes_post_0_0.csv
//
// Implements 16 queries across 5 categories:
//   Writes          (3): insert Person, insert knows, insert Post+hasCreator
//   Single-type (3): person profile, friends list by date, BFS shortest path
//   Cross-type  (5): post profile, post→author, IC-2 friends' posts,
//                    insert likes, count likes in date range
//   Aggregates  (3): degree count, knows edges in date range, posts liked in range
//   BI queries  (2): BI-1 posting summary, BI-12 message distribution per person
//                    (COLUMNAR mode only — demonstrate colgroup I/O benefit)

#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

#include "common_defs.h"
#include "graph_engine.h"
#include "ldbc_snb_loader.h"
#include "prop_schema.h"

// ============================================================
// Typed-vertex-ID helpers (O(1), no runtime state)
// ============================================================

static inline bool is_person(node_id_t id) { return VTYPE_OF(id) == VT_PERSON; }
static inline bool is_post(node_id_t id)   { return VTYPE_OF(id) == VT_POST; }

// ============================================================
// Property read helpers (decode blobs by ID range)
// ============================================================

struct PersonProps {
    int64_t creation_date = 0;
    int64_t birthday = 0;
    int8_t  gender = 0;
    char    first_name[SNBPersonSchema::STR_LEN]   = {};
    char    last_name[SNBPersonSchema::STR_LEN]    = {};
    char    browser_used[SNBPersonSchema::STR_LEN] = {};
    char    location_ip[SNBPersonSchema::STR_LEN]  = {};
};

struct PostProps {
    int64_t creation_date = 0;
    int32_t length = 0;
};

struct KnowsProps {
    int64_t creation_date = 0;
};

struct LikesProps {
    int64_t creation_date = 0;
};

static PersonProps decode_person(const prop_blob &pb)
{
    PersonProps p;
    if (pb.data && pb.size >= SNBPersonSchema::TOTAL_SIZE) {
        p.creation_date = SNBPersonSchema::get_creation_date(pb.data);
        p.birthday      = SNBPersonSchema::get_birthday(pb.data);
        p.gender        = SNBPersonSchema::get_gender(pb.data);
        std::memcpy(p.first_name,   SNBPersonSchema::get_first_name(pb.data),   SNBPersonSchema::STR_LEN);
        std::memcpy(p.last_name,    SNBPersonSchema::get_last_name(pb.data),    SNBPersonSchema::STR_LEN);
        std::memcpy(p.browser_used, SNBPersonSchema::get_browser_used(pb.data), SNBPersonSchema::STR_LEN);
        std::memcpy(p.location_ip,  SNBPersonSchema::get_location_ip(pb.data),  SNBPersonSchema::STR_LEN);
    }
    return p;
}

static PostProps decode_post(const prop_blob &pb)
{
    PostProps p;
    if (pb.data && pb.size >= SNBPostSchema::TOTAL_SIZE) {
        p.creation_date = SNBPostSchema::get_creation_date(pb.data);
        p.length        = SNBPostSchema::get_length(pb.data);
    }
    return p;
}

static KnowsProps decode_knows(const prop_blob &pb)
{
    KnowsProps k;
    if (pb.data && pb.size >= SNBKnowsSchema::TOTAL_SIZE)
        k.creation_date = SNBKnowsSchema::get_creation_date(pb.data);
    return k;
}

static LikesProps decode_likes(const prop_blob &pb)
{
    LikesProps l;
    if (pb.data && pb.size >= SNBLikesSchema::TOTAL_SIZE)
        l.creation_date = SNBLikesSchema::get_creation_date(pb.data);
    return l;
}

// ============================================================
// Timing helper
// ============================================================

#include <chrono>
using Clock = std::chrono::steady_clock;
using Ms = std::chrono::duration<double, std::milli>;

#define TIME_START(label)                                   \
    auto _t0_##label = Clock::now();                        \
    fprintf(stderr, "[QUERY] running %-40s ... ", #label);

#define TIME_END(label)                                     \
    {                                                       \
        double _ms = Ms(Clock::now() - _t0_##label).count();\
        fprintf(stderr, "%.3f ms\n", _ms);                  \
    }

// Like TIME_END but also writes elapsed ms into a double variable.
#define TIME_END_CAP(label, ms_out)                             \
    {                                                           \
        (ms_out) = Ms(Clock::now() - _t0_##label).count();     \
        fprintf(stderr, "%.3f ms\n", (ms_out));                 \
    }

// Holds per-query timings for the EMBEDDED vs COLUMNAR comparison table.
// -1.0 means "not measured / N/A".
struct QueryTimes {
    double r2        = -1.0;
    double a2        = -1.0;
    double a3        = -1.0;
    double bi1       = -1.0;
    double bi12_fast = -1.0;
};

// ============================================================
// Separator
// ============================================================
#define SEP() fprintf(stderr, "------------------------------------------------------------\n")

// ============================================================
// ============================================================
//  WRITE QUERIES
// ============================================================
// ============================================================

// W1: Insert a new Person vertex
static void w1_insert_person(GraphBase &graph, node_id_t &person_counter,
                             bool has_props)
{
    SEP();
    node_id_t new_id = MAKE_TYPED_ID(VT_PERSON, person_counter++);
    TIME_START(w1_insert_person)

    node n;
    n.id = new_id;
    graph.add_node(n, false);

    if (has_props) {
        uint8_t buf[SNBPersonSchema::TOTAL_SIZE] = {};
        SNBPersonSchema::set_creation_date(buf, 1700000000000LL);
        SNBPersonSchema::set_birthday(buf,      631152000000LL);
        SNBPersonSchema::set_gender(buf, 0);
        graph.set_node_properties(new_id, buf, SNBPersonSchema::TOTAL_SIZE);
    }

    TIME_END(w1_insert_person)
    fprintf(stderr, "  inserted Person id=%llu\n", (unsigned long long)new_id);
}

// W2: Insert a knows edge between two existing persons
static void w2_insert_knows(GraphBase &graph, node_id_t src, node_id_t dst,
                            int64_t creation_date, bool has_props)
{
    SEP();
    TIME_START(w2_insert_knows)

    edge e;
    e.src_id = src;
    e.dst_id = dst;
    graph.add_edge(e, false);

    if (has_props) {
        uint8_t buf[SNBKnowsSchema::TOTAL_SIZE] = {};
        SNBKnowsSchema::set_creation_date(buf, creation_date);
        graph.set_edge_properties(src, dst, buf, SNBKnowsSchema::TOTAL_SIZE);
    }

    TIME_END(w2_insert_knows)
    fprintf(stderr, "  inserted knows %llu→%llu\n",
            (unsigned long long)src, (unsigned long long)dst);
}

// W3: Insert a Post vertex and its hasCreator edge
static void w3_insert_post_with_creator(GraphBase &graph,
                                        node_id_t &next_post_id,
                                        node_id_t author_id,
                                        int64_t creation_date,
                                        int32_t length,
                                        bool has_props)
{
    SEP();
    node_id_t post_id = next_post_id++;
    TIME_START(w3_insert_post_hasCreator)

    node n;
    n.id = post_id;
    graph.add_node(n, false);

    if (has_props) {
        uint8_t buf[SNBPostSchema::TOTAL_SIZE] = {};
        SNBPostSchema::set_creation_date(buf, creation_date);
        SNBPostSchema::set_length(buf, length);
        graph.set_node_properties(post_id, buf, SNBPostSchema::TOTAL_SIZE);
    }

    // Insert hasCreator edge (Post → Person, no properties)
    edge e;
    e.src_id = post_id;
    e.dst_id = author_id;
    graph.add_edge(e, false);

    TIME_END(w3_insert_post_hasCreator)
    fprintf(stderr, "  inserted Post id=%llu, hasCreator→%llu\n",
            (unsigned long long)post_id, (unsigned long long)author_id);
}

// ============================================================
// ============================================================
//  SINGLE-TYPE READ QUERIES
// ============================================================
// ============================================================

// R1: Person profile lookup — fetch all properties of a person by ID
static PersonProps r1_person_profile(GraphBase &graph, node_id_t pid)
{
    SEP();
    TIME_START(r1_person_profile)
    prop_blob pb = graph.get_node_properties(pid);
    PersonProps p = decode_person(pb);
    TIME_END(r1_person_profile)
    fprintf(stderr, "  Person %llu: %s %s  gender=%d  birthday=%lld  creationDate=%lld\n"
                    "              browser=%s  ip=%s\n",
            (unsigned long long)pid,
            p.first_name, p.last_name, (int)p.gender,
            (long long)p.birthday, (long long)p.creation_date,
            p.browser_used, p.location_ip);
    return p;
}

// R2: Friends list — get all out-neighbors of a person, sorted by knows.creationDate
//
// EMBEDDED mode: fetches each edge's property blob individually via
//   get_edge_properties(pid, nb) — one random seek per friend.
//
// COLUMNAR mode: opens colgroup:knows_props:temporal once and does a single
//   forward range-scan from key (pid, 0).  Reads only the temporal B-tree;
//   never touches the identity B-tree or topology tables.
static std::vector<std::pair<node_id_t, int64_t>>
r2_friends_sorted_by_date(GraphBase &graph, node_id_t pid, bool has_props,
                           PropStorageMode prop_mode = EMBEDDED,
                           double *out_ms = nullptr)
{
    SEP();
    TIME_START(r2_friends_sorted_by_date)

    std::vector<std::pair<node_id_t, int64_t>> result;

    if (has_props && prop_mode == COLUMNAR) {
        // Range-scan colgroup:knows_props:temporal for all edges with src == pid.
        WT_CURSOR *cur = graph.open_colgroup_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
        cur->set_key(cur, (uint64_t)pid, (uint64_t)0);
        int cmp = 0;
        int ret = cur->search_near(cur, &cmp);
        if (ret == 0) {
            if (cmp < 0) ret = cur->next(cur);
            while (ret == 0) {
                uint64_t src, dst;
                cur->get_key(cur, &src, &dst);
                if (src != (uint64_t)pid) break;
                uint64_t cDate;
                cur->get_value(cur, &cDate);
                result.emplace_back((node_id_t)dst, (int64_t)cDate);
                ret = cur->next(cur);
            }
        }
        cur->close(cur);
    } else {
        // One get_edge_properties (full blob) per knows neighbor.
        std::vector<node_id_t> neighbors = graph.get_out_nodes_id(pid);
        result.reserve(neighbors.size());
        for (node_id_t nb : neighbors) {
            if (!is_person(nb)) continue;  // skip likes (person→post) edges
            int64_t cd = 0;
            if (has_props) {
                prop_blob pb = graph.get_edge_properties(pid, nb);
                cd = decode_knows(pb).creation_date;
            }
            result.emplace_back(nb, cd);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    if (out_ms) { TIME_END_CAP(r2_friends_sorted_by_date, *out_ms) }
    else        { TIME_END(r2_friends_sorted_by_date) }

    fprintf(stderr, "  Person %llu has %zu friends\n",
            (unsigned long long)pid, result.size());
    int printed = 0;
    for (auto &[fid, cd] : result) {
        fprintf(stderr, "    friend=%llu creationDate=%lld\n",
                (unsigned long long)fid, (long long)cd);
        if (++printed >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// R3: BFS shortest path — unweighted distance between two persons
static int r3_bfs_shortest_path(GraphBase &graph, node_id_t src, node_id_t dst)
{
    SEP();
    TIME_START(r3_bfs_shortest_path)

    if (src == dst) {
        TIME_END(r3_bfs_shortest_path)
        return 0;
    }

    std::unordered_map<node_id_t, int> dist;
    std::deque<node_id_t> q;
    dist[src] = 0;
    q.push_back(src);

    int found_dist = -1;
    while (!q.empty() && found_dist < 0) {
        node_id_t u = q.front(); q.pop_front();
        std::vector<node_id_t> nbrs = graph.get_out_nodes_id(u);
        for (node_id_t v : nbrs) {
            // Only traverse knows edges (person→person)
            if (!is_person(v)) continue;
            if (dist.count(v)) continue;
            dist[v] = dist[u] + 1;
            if (v == dst) { found_dist = dist[v]; break; }
            q.push_back(v);
        }
    }

    TIME_END(r3_bfs_shortest_path)
    fprintf(stderr, "  BFS(%llu → %llu) = %d hops\n",
            (unsigned long long)src, (unsigned long long)dst, found_dist);
    return found_dist;
}

// ============================================================
// ============================================================
//  CROSS-TYPE READ QUERIES
// ============================================================
// ============================================================

// X1: Post profile lookup — fetch all properties of a post by ID
static PostProps x1_post_profile(GraphBase &graph, node_id_t post_id)
{
    SEP();
    TIME_START(x1_post_profile)
    prop_blob pb = graph.get_node_properties(post_id);
    PostProps p = decode_post(pb);
    TIME_END(x1_post_profile)
    fprintf(stderr, "  Post %llu: creationDate=%lld length=%d\n",
            (unsigned long long)post_id, (long long)p.creation_date, p.length);
    return p;
}

// X2: Post → author (hasCreator reverse lookup)
// In our model: hasCreator is stored as Post→Person out-edge.
// So: look at out-edges of a Post, find the one pointing to a Person.
static node_id_t x2_post_author(GraphBase &graph, node_id_t post_id)
{
    SEP();
    TIME_START(x2_post_author)

    std::vector<node_id_t> out = graph.get_out_nodes_id(post_id);
    node_id_t author = OutOfBand_ID_MAX;
    for (node_id_t nb : out) {
        if (is_person(nb)) {
            author = nb;
            break;
        }
    }

    TIME_END(x2_post_author)
    if (author != OutOfBand_ID_MAX)
        fprintf(stderr, "  Post %llu → author Person %llu\n",
                (unsigned long long)post_id, (unsigned long long)author);
    else
        fprintf(stderr, "  Post %llu → author not found\n",
                (unsigned long long)post_id);
    return author;
}

// X3: IC-2 — given a person P, get friends' most recent posts (before cutoff)
// Returns up to limit (post_id, post.creationDate) pairs sorted by date desc.
static std::vector<std::pair<node_id_t, int64_t>>
x3_ic2_friends_recent_posts(GraphBase &graph, node_id_t pid,
                             int64_t cutoff_ms, int limit,
                             bool has_props)
{
    SEP();
    TIME_START(x3_ic2_friends_recent_posts)

    // Step 1: get friends (persons in out-edges of pid)
    std::vector<node_id_t> friends = graph.get_out_nodes_id(pid);
    std::vector<std::pair<node_id_t, int64_t>> result;

    // Step 2: for each friend, scan their in-edges to find posts where
    // the post's hasCreator points TO the friend.
    // In our model: hasCreator is Post→Person out-edge, so we look at
    // in-edges of each friend (Person) — those are Posts whose hasCreator is this friend.
    for (node_id_t friend_id : friends) {
        if (!is_person(friend_id)) continue;

        std::vector<node_id_t> creators_in = graph.get_in_nodes_id(friend_id);
        for (node_id_t post_id : creators_in) {
            if (!is_post(post_id)) continue;

            int64_t post_date = 0;
            if (has_props) {
                prop_blob pb = graph.get_node_properties(post_id);
                post_date = decode_post(pb).creation_date;
            }
            if (post_date < cutoff_ms)
                result.emplace_back(post_id, post_date);
        }
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if ((int)result.size() > limit)
        result.resize(limit);

    TIME_END(x3_ic2_friends_recent_posts)
    fprintf(stderr, "  IC-2 for Person %llu (cutoff=%lld): %zu posts found\n",
            (unsigned long long)pid, (long long)cutoff_ms, result.size());
    int printed = 0;
    for (auto &[post_id, cd] : result) {
        fprintf(stderr, "    post=%llu creationDate=%lld\n",
                (unsigned long long)post_id, (long long)cd);
        if (++printed >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// X4: Insert a likes edge (Person → Post) with creationDate property
static void x4_insert_likes(GraphBase &graph, node_id_t person_id,
                             node_id_t post_id, int64_t creation_date,
                             bool has_props)
{
    SEP();
    TIME_START(x4_insert_likes)

    edge e;
    e.src_id = person_id;
    e.dst_id = post_id;
    graph.add_edge(e, false);

    if (has_props) {
        uint8_t buf[SNBLikesSchema::TOTAL_SIZE] = {};
        SNBLikesSchema::set_creation_date(buf, creation_date);
        graph.set_edge_properties(person_id, post_id, buf, SNBLikesSchema::TOTAL_SIZE);
    }

    TIME_END(x4_insert_likes)
    fprintf(stderr, "  inserted likes %llu→%llu\n",
            (unsigned long long)person_id, (unsigned long long)post_id);
}

// X5: Count likes on a post within a date range [lo_ms, hi_ms]
static int64_t x5_count_likes_in_range(GraphBase &graph, node_id_t post_id,
                                        int64_t lo_ms, int64_t hi_ms,
                                        bool has_props)
{
    SEP();
    TIME_START(x5_count_likes_in_range)

    // Persons who liked this post are in-edges of the post from the person range
    std::vector<node_id_t> likers = graph.get_in_nodes_id(post_id);
    int64_t count = 0;
    for (node_id_t liker : likers) {
        if (!is_person(liker)) continue;
        if (!has_props) { count++; continue; }
        prop_blob pb = graph.get_edge_properties(liker, post_id);
        int64_t cd = decode_likes(pb).creation_date;
        if (cd >= lo_ms && cd <= hi_ms)
            count++;
    }

    TIME_END(x5_count_likes_in_range)
    fprintf(stderr, "  Post %llu likes in range [%lld, %lld]: %lld\n",
            (unsigned long long)post_id, (long long)lo_ms, (long long)hi_ms,
            (long long)count);
    return count;
}

// ============================================================
// ============================================================
//  AGGREGATE QUERIES
// ============================================================
// ============================================================

// A1: Degree count — out-degree and in-degree of a node
static std::pair<degree_t, degree_t>
a1_degree_count(GraphBase &graph, node_id_t id)
{
    SEP();
    TIME_START(a1_degree_count)
    degree_t out = graph.get_out_degree(id);
    degree_t in  = graph.get_in_degree(id);
    TIME_END(a1_degree_count)
    fprintf(stderr, "  Node %llu: out-degree=%u in-degree=%u\n",
            (unsigned long long)id, out, in);
    return {out, in};
}

// A2: Count knows edges originating from a person within a date range
//
// COLUMNAR mode: range-scans colgroup:knows_props:temporal — one B-tree read,
//   no topology lookup, date filter applied inline.
// EMBEDDED mode: one get_edge_properties blob fetch per knows neighbor.
static int64_t a2_knows_in_date_range(GraphBase &graph, node_id_t pid,
                                       int64_t lo_ms, int64_t hi_ms,
                                       bool has_props,
                                       PropStorageMode prop_mode = EMBEDDED,
                                       double *out_ms = nullptr)
{
    SEP();
    TIME_START(a2_knows_in_date_range)

    int64_t count = 0;

    if (has_props && prop_mode == COLUMNAR) {
        WT_CURSOR *cur = graph.open_colgroup_cursor(KNOWS_PROPS_TABLE, CG_TEMPORAL);
        cur->set_key(cur, (uint64_t)pid, (uint64_t)0);
        int cmp = 0;
        int ret = cur->search_near(cur, &cmp);
        if (ret == 0) {
            if (cmp < 0) ret = cur->next(cur);
            while (ret == 0) {
                uint64_t src, dst;
                cur->get_key(cur, &src, &dst);
                if (src != (uint64_t)pid) break;
                uint64_t cDate;
                cur->get_value(cur, &cDate);
                if ((int64_t)cDate >= lo_ms && (int64_t)cDate <= hi_ms)
                    count++;
                ret = cur->next(cur);
            }
        }
        cur->close(cur);
    } else {
        std::vector<node_id_t> friends = graph.get_out_nodes_id(pid);
        for (node_id_t nb : friends) {
            if (!is_person(nb)) continue;
            if (!has_props) { count++; continue; }
            prop_blob pb = graph.get_edge_properties(pid, nb);
            int64_t cd = decode_knows(pb).creation_date;
            if (cd >= lo_ms && cd <= hi_ms)
                count++;
        }
    }

    if (out_ms) { TIME_END_CAP(a2_knows_in_date_range, *out_ms) }
    else        { TIME_END(a2_knows_in_date_range) }

    fprintf(stderr, "  Person %llu knows edges in date range: %lld\n",
            (unsigned long long)pid, (long long)count);
    return count;
}

// A3: Count posts liked by a person within a date range
//
// COLUMNAR mode: range-scans colgroup:likes_props:temporal — one B-tree read,
//   date filter applied inline.
// EMBEDDED mode: one get_edge_properties blob fetch per liked post.
static int64_t a3_posts_liked_in_range(GraphBase &graph, node_id_t pid,
                                        int64_t lo_ms, int64_t hi_ms,
                                        bool has_props,
                                        PropStorageMode prop_mode = EMBEDDED,
                                        double *out_ms = nullptr)
{
    SEP();
    TIME_START(a3_posts_liked_in_range)

    int64_t count = 0;

    if (has_props && prop_mode == COLUMNAR) {
        WT_CURSOR *cur = graph.open_colgroup_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
        cur->set_key(cur, (uint64_t)pid, (uint64_t)0);
        int cmp = 0;
        int ret = cur->search_near(cur, &cmp);
        if (ret == 0) {
            if (cmp < 0) ret = cur->next(cur);
            while (ret == 0) {
                uint64_t src, dst;
                cur->get_key(cur, &src, &dst);
                if (src != (uint64_t)pid) break;
                uint64_t cDate;
                cur->get_value(cur, &cDate);
                if ((int64_t)cDate >= lo_ms && (int64_t)cDate <= hi_ms)
                    count++;
                ret = cur->next(cur);
            }
        }
        cur->close(cur);
    } else {
        std::vector<node_id_t> liked = graph.get_out_nodes_id(pid);
        for (node_id_t post_id : liked) {
            if (!is_post(post_id)) continue;  // skip knows edges
            if (!has_props) { count++; continue; }
            prop_blob pb = graph.get_edge_properties(pid, post_id);
            int64_t cd = decode_likes(pb).creation_date;
            if (cd >= lo_ms && cd <= hi_ms)
                count++;
        }
    }

    if (out_ms) { TIME_END_CAP(a3_posts_liked_in_range, *out_ms) }
    else        { TIME_END(a3_posts_liked_in_range) }

    fprintf(stderr, "  Person %llu posts-liked in date range: %lld\n",
            (unsigned long long)pid, (long long)count);
    return count;
}

// ============================================================
// BI queries (COLUMNAR mode only)
// ============================================================

// BI-1: Posting Summary
// Scan all posts, group by (year, lengthCategory), output counts and lengths.
// lengthCategory: 0 = short (<40), 1 = medium (40-79), 2 = long (80-254), 3 = very long (>=255)
// Only valid when prop_mode == COLUMNAR; falls back to stderr notice otherwise.
static void bi1_posting_summary(GraphBase &graph, node_id_t total_posts,
                                 double *out_ms = nullptr)
{
    TIME_START(bi1_posting_summary)

    struct Key { int year; int cat; };
    struct Stats { int64_t count = 0; int64_t sum_len = 0; };
    std::map<std::pair<int,int>, Stats> groups;

    auto year_of = [](uint64_t epoch_ms) -> int {
        // Rough year extraction: epoch_ms / (365.25 * 24 * 3600 * 1000) + 1970
        return (int)(epoch_ms / 31557600000ULL) + 1970;
    };
    auto classify_length = [](int32_t len) -> int {
        if (len < 40)  return 0;
        if (len < 80)  return 1;
        if (len < 255) return 2;
        return 3;
    };

    WT_CURSOR *cur = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
    while (cur->next(cur) == 0) {
        uint64_t vid, cDate; int32_t length;
        cur->get_key(cur, &vid);
        cur->get_value(cur, &cDate, &length);
        int yr  = year_of(cDate);
        int cat = classify_length(length);
        auto &s = groups[{yr, cat}];
        s.count++;
        s.sum_len += length;
    }
    cur->close(cur);

    if (out_ms) { TIME_END_CAP(bi1_posting_summary, *out_ms) }
    else        { TIME_END(bi1_posting_summary) }

    fprintf(stderr, "  BI-1 Posting Summary (%llu posts scanned):\n",
            (unsigned long long)total_posts);
    fprintf(stderr, "  %-6s  %-3s  %-9s  %-8s  %-14s  %-12s\n",
            "year", "cat", "count", "sum_len", "avg_len", "pct_of_total");
    for (auto &[k, s] : groups) {
        double avg = s.count > 0 ? (double)s.sum_len / s.count : 0.0;
        double pct = total_posts > 0 ? 100.0 * s.count / total_posts : 0.0;
        fprintf(stderr, "  %-6d  %-3d  %-9lld  %-8lld  %-14.2f  %-12.2f%%\n",
                k.first, k.second,
                (long long)s.count, (long long)s.sum_len, avg, pct);
    }
}

// BI-12: Message Distribution by Person
// Scan all posts, join via hasCreator out-edge, count posts per person.
// Uses the temporal colgroup to scan post creation dates; joins topology for creator.
static void bi12_message_distribution(GraphBase &graph, node_id_t total_posts)
{
    TIME_START(bi12_message_distribution)

    std::unordered_map<node_id_t, int64_t> creator_count;

    WT_CURSOR *cur = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
    while (cur->next(cur) == 0) {
        uint64_t vid;
        cur->get_key(cur, &vid);
        // Find the hasCreator out-edge for this post (POST→PERSON edge)
        std::vector<node_id_t> creators = graph.get_out_nodes_id((node_id_t)vid);
        for (node_id_t c : creators) {
            if (VTYPE_OF(c) == VT_PERSON)
                creator_count[c]++;
        }
    }
    cur->close(cur);

    TIME_END(bi12_message_distribution)

    // Sort by count descending, then personId ascending
    std::vector<std::pair<node_id_t, int64_t>> ranked(creator_count.begin(), creator_count.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    fprintf(stderr, "  BI-12 Message Distribution (%llu posts, %zu creators):\n",
            (unsigned long long)total_posts, ranked.size());
    int shown = 0;
    for (auto &[pid, cnt] : ranked) {
        if (shown++ >= 10) { fprintf(stderr, "  ... (showing top 10)\n"); break; }
        fprintf(stderr, "  person %llu: %lld posts\n",
                (unsigned long long)VCOUNTER_OF(pid), (long long)cnt);
    }
}

// BI-12 (optimised): Message Distribution by Person — two sequential scans, no point lookups.
//
// PROBLEM WITH THE NAIVE VERSION
// --------------------------------
// bi12_message_distribution() opens a colgroup cursor on post_props:temporal and,
// for each of the N_posts rows, calls get_out_nodes_id(post_id) to find the creator.
// get_out_nodes_id() issues one WT cursor seek per post, so the total cost is:
//
//   O(N_posts × point_lookup_latency)
//
// On SF3 (2.5M posts) this takes ~33 s because each seek hits a random leaf page in
// the OUT_EDGES B-tree.  The colgroup scan itself (334 ms) is not the bottleneck.
//
// THE OPTIMISATION: TWO SEQUENTIAL SCANS
// ----------------------------------------
// hasCreator edges are POST→PERSON edges in the OUT_EDGES topology table.  Because
// vertex types are encoded in the top 8 bits of every node_id_t (bit-reservation
// scheme), we can identify them without any secondary index:
//
//   VTYPE_OF(src) == VT_POST  &&  VTYPE_OF(dst) == VT_PERSON  →  hasCreator edge
//
// Pass 1  [colgroup scan, O(N_posts)]:
//   Scan colgroup:post_props:temporal.  For each row, check creationDate and length
//   against the query filters.  Record qualifying post IDs in an unordered_set.
//   Cost: one sequential read of the temporal B-tree (≈12B/row, no content pages).
//
// Pass 2  [topology scan, O(N_edges)]:
//   Scan OUT_EDGES via get_edge_iter().  For each POST→PERSON edge whose src is in
//   the qualifying set, increment creator_count[dst].
//   Cost: one sequential read of the OUT_EDGES B-tree.
//
// Both passes are fully sequential; no random seeks.  Total cost:
//
//   O(N_posts + N_edges)   ≈ O(N_posts)  since N_hasCreator = N_posts
//
// MEMORY TRADE-OFF
// -----------------
// Pass 1 materialises the qualifying post set in an unordered_set<node_id_t>.
// Worst case (no filter): N_posts × 8B ≈ 20 MB for SF3.  Acceptable.
// With a tight date range the set is much smaller.
//
// WHY NOT A SINGLE PASS?
// -----------------------
// A single pass over OUT_EDGES would miss the date/length filter from post_props.
// We need the property scan to know which posts qualify before we count by creator.
// If no filter is needed, Pass 1 can be skipped and OUT_EDGES scanned alone.
//
// Parameters:
//   max_date       — include posts with creationDate <= max_date
//                    (INT64_MAX = no filter)
//   min_length     — include posts with length >= min_length
//                    (0 = no filter)
static void bi12_message_distribution_fast(GraphBase &graph,
                                            node_id_t total_posts,
                                            int64_t max_date   = INT64_MAX,
                                            int32_t min_length = 0,
                                            double *out_ms     = nullptr)
{
    TIME_START(bi12_message_distribution_fast)

    // Pass 1: build qualifying post set from temporal colgroup.
    std::unordered_set<node_id_t> qualifying;
    qualifying.reserve(total_posts);

    bool no_filter = (max_date == INT64_MAX && min_length == 0);
    if (!no_filter) {
        WT_CURSOR *prop_cur = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        while (prop_cur->next(prop_cur) == 0) {
            uint64_t vid, cDate; int32_t length;
            prop_cur->get_key(prop_cur, &vid);
            prop_cur->get_value(prop_cur, &cDate, &length);
            if ((int64_t)cDate <= max_date && length >= min_length)
                qualifying.insert((node_id_t)vid);
        }
        prop_cur->close(prop_cur);
    }

    // Pass 2: scan topology once, accumulate creator counts for qualifying posts.
    std::unordered_map<node_id_t, int64_t> creator_count;

    EdgeCursor *ec = graph.get_edge_iter();
    edge found;
    ec->next(&found);
    while (found.src_id != OutOfBand_ID_MAX) {
        node_id_t src = found.src_id;
        node_id_t dst = found.dst_id;
        if (VTYPE_OF(src) == VT_POST && VTYPE_OF(dst) == VT_PERSON) {
            if (no_filter || qualifying.count(src))
                creator_count[dst]++;
        }
        ec->next(&found);
    }
    delete ec;

    if (out_ms) { TIME_END_CAP(bi12_message_distribution_fast, *out_ms) }
    else        { TIME_END(bi12_message_distribution_fast) }

    // Sort by count descending, then personId ascending.
    std::vector<std::pair<node_id_t, int64_t>> ranked(creator_count.begin(), creator_count.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });

    fprintf(stderr, "  BI-12-fast Message Distribution (%llu posts, %zu creators):\n",
            (unsigned long long)total_posts, ranked.size());
    int shown = 0;
    for (auto &[pid, cnt] : ranked) {
        if (shown++ >= 10) { fprintf(stderr, "  ... (showing top 10)\n"); break; }
        fprintf(stderr, "  person %llu: %lld posts\n",
                (unsigned long long)VCOUNTER_OF(pid), (long long)cnt);
    }
}

// ============================================================
// main
// ============================================================

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <data_dir> [graph_type] [--log <file>] [--dry-run]\n", argv[0]);
        fprintf(stderr, "  data_dir    directory containing LDBC SNB dynamic/ CSV files\n");
        fprintf(stderr, "  graph_type  adj | splitekey  (default: splitekey)\n");
        fprintf(stderr, "  --log <f>   write every NODE/EDGE insertion to <f> (converted IDs)\n");
        fprintf(stderr, "  --dry-run   parse CSVs and write log without inserting into the DB\n");
        return 1;
    }

    std::string data_dir = argv[1];
    // Normalize path: remove trailing slash
    while (!data_dir.empty() && data_dir.back() == '/')
        data_dir.pop_back();

    std::string dyn = data_dir + "/dynamic";

    // ---- Parse optional flags ----
    GraphType graph_type = GraphType::SplitEKey;
    std::string insertion_log_path;
    bool dry_run = false;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "adj")
            graph_type = GraphType::Adj;
        else if (arg == "splitekey")
            graph_type = GraphType::SplitEKey;
        else if (arg == "--log" && i + 1 < argc)
            insertion_log_path = argv[++i];
        else if (arg == "--dry-run")
            dry_run = true;
        else {
            fprintf(stderr, "Unknown argument '%s'\n", argv[i]);
            return 1;
        }
    }

    if (dry_run && insertion_log_path.empty()) {
        fprintf(stderr, "Note: --dry-run without --log produces no output. Adding --log is recommended.\n");
    }

    // ---- Graph engine setup ----
    graph_opts opts;
    opts.create_new      = true;
    opts.optimize_create = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.is_weighted     = false;
    opts.has_node_props  = true;
    opts.has_edge_props  = true;
    opts.prop_mode       = COLUMNAR;
    opts.type            = graph_type;
    opts.db_name         = "ldbc_snb_queries";
    opts.db_dir          = "./db";
    opts.conn_config     = "cache_size=2GB";
    opts.stat_log        = "./";

    fprintf(stderr, "=== Graph type: %s ===\n", graph_type == GraphType::Adj ? "adj" : "splitekey");
    if (dry_run)
        fprintf(stderr, "=== DRY RUN: CSV parsing only, no DB insertions ===\n");

    // ---- Timing comparison buckets ----
    QueryTimes emb_times, col_times;

    // ---- PASS 1: EMBEDDED (baseline for timing comparison) ----
    // Load into a separate DB with blob-in-edge-table storage,
    // then run only the three queries whose colgroup benefit we measure.
    if (!dry_run) {
        graph_opts emb_opts = opts;
        emb_opts.prop_mode = EMBEDDED;
        emb_opts.db_dir    = "./db_emb";

        GraphEngine emb_engine(1, emb_opts);
        GraphBase *emb_ptr = emb_engine.create_graph_handle();
        GraphBase &emb_graph = *emb_ptr;

        fprintf(stderr, "\n=== PASS 1 (EMBEDDED baseline): loading data ===\n");
        LDBCLoader emb_loader(&emb_graph, emb_opts);
        emb_loader.load_persons(dyn + "/person_0_0.csv");
        emb_loader.load_posts(dyn + "/post_0_0.csv");
        emb_loader.load_knows(dyn + "/person_knows_person_0_0.csv");
        emb_loader.load_has_creator(dyn + "/post_hasCreator_person_0_0.csv");
        emb_loader.load_likes(dyn + "/person_likes_post_0_0.csv");
        emb_loader.flush_node_props();
        emb_loader.flush_edge_props();

        node_id_t emb_pc = emb_loader.person_count;
        node_id_t emb_sp0 = MAKE_TYPED_ID(VT_PERSON, 0);
        node_id_t emb_sp1 = (emb_pc > 1) ? MAKE_TYPED_ID(VT_PERSON, 1) : emb_sp0;

        fprintf(stderr, "=== PASS 1 (EMBEDDED baseline): timing queries ===\n");
        r2_friends_sorted_by_date(emb_graph, emb_sp0, emb_opts.has_edge_props,
                                   EMBEDDED, &emb_times.r2);
        a2_knows_in_date_range(emb_graph, emb_sp0, 0LL, INT64_MAX,
                                emb_opts.has_edge_props, EMBEDDED, &emb_times.a2);
        a3_posts_liked_in_range(emb_graph, emb_sp1, 0LL, INT64_MAX,
                                 emb_opts.has_edge_props, EMBEDDED, &emb_times.a3);

        emb_ptr->close(false);
        emb_engine.close_graph();
        fprintf(stderr, "=== PASS 1 complete ===\n\n");
    }

    // ---- PASS 2: COLUMNAR ----
    GraphEngine engine(1, opts);
    GraphBase *graph_ptr = engine.create_graph_handle();
    GraphBase &graph = *graph_ptr;

    // ---- Load data ----
    fprintf(stderr, "=== Loading LDBC SNB data from %s ===\n", data_dir.c_str());

    LDBCLoader loader(&graph, opts);
    loader.dry_run = dry_run;
    if (!insertion_log_path.empty()) {
        loader.enable_insertion_log(insertion_log_path);
        fprintf(stderr, "=== Insertion log: %s ===\n", insertion_log_path.c_str());
    }

    fprintf(stderr, "[LOAD] persons ... ");
    loader.load_persons(dyn + "/person_0_0.csv");
    fprintf(stderr, "%llu persons\n", (unsigned long long)loader.person_count);

    fprintf(stderr, "[LOAD] posts ... ");
    loader.load_posts(dyn + "/post_0_0.csv");
    fprintf(stderr, "%llu posts\n", (unsigned long long)loader.post_count);

    // All add_node calls complete before any add_edge
    fprintf(stderr, "[LOAD] knows edges ... ");
    loader.load_knows(dyn + "/person_knows_person_0_0.csv");
    fprintf(stderr, "done\n");

    fprintf(stderr, "[LOAD] hasCreator edges ... ");
    loader.load_has_creator(dyn + "/post_hasCreator_person_0_0.csv");
    fprintf(stderr, "done\n");

    fprintf(stderr, "[LOAD] likes edges ... ");
    loader.load_likes(dyn + "/person_likes_post_0_0.csv");
    fprintf(stderr, "done\n");

    // Flush node and edge properties after all structural inserts
    fprintf(stderr, "[LOAD] flushing node properties ... ");
    loader.flush_node_props();
    fprintf(stderr, "done\n");

    fprintf(stderr, "[LOAD] flushing edge properties ... ");
    loader.flush_edge_props();
    fprintf(stderr, "done\n");

    node_id_t person_count = loader.person_count;
    node_id_t post_count   = loader.post_count;

    fprintf(stderr, "\n=== Graph loaded: %llu persons, %llu posts ===\n\n",
            (unsigned long long)person_count,
            (unsigned long long)post_count);

    // ---- Node count verification ----
    // Cross-check three sources:
    //   (a) loader counters  — what we tried to insert
    //   (b) get_num_nodes()  — in-memory atomic counter incremented by add_node
    //   (c) get_nodes()      — full WT cursor scan (ground truth in the table)
    // Also breaks down (c) by vertex type to catch ID-partitioning bugs where
    // Post IDs lose their type bits and collide with Person IDs.
    if (!dry_run) {
        node_id_t expected_nodes = person_count + post_count;
        node_id_t atomic_count   = graph.get_num_nodes();

        fprintf(stderr, "=== Node count verification ===\n");
        fprintf(stderr, "  loader inserted:   %llu persons + %llu posts = %llu total\n",
                (unsigned long long)person_count,
                (unsigned long long)post_count,
                (unsigned long long)expected_nodes);
        fprintf(stderr, "  get_num_nodes():   %llu  (in-memory atomic counter)\n",
                (unsigned long long)atomic_count);

        // Full WT scan — counts what is actually stored
        std::vector<node> all_nodes = graph.get_nodes();
        node_id_t scanned_persons = 0, scanned_posts = 0, scanned_unknown = 0;
        for (const node &nd : all_nodes) {
            uint64_t vtype = VTYPE_OF(nd.id);
            if (vtype == VT_PERSON)      scanned_persons++;
            else if (vtype == VT_POST)   scanned_posts++;
            else                         scanned_unknown++;
        }
        node_id_t scanned_total = (node_id_t)all_nodes.size();

        fprintf(stderr, "  get_nodes() scan:  %llu total  "
                "(%llu persons, %llu posts, %llu unknown type)\n",
                (unsigned long long)scanned_total,
                (unsigned long long)scanned_persons,
                (unsigned long long)scanned_posts,
                (unsigned long long)scanned_unknown);

        if (atomic_count != expected_nodes)
            fprintf(stderr, "  MISMATCH: atomic counter %llu != expected %llu\n",
                    (unsigned long long)atomic_count,
                    (unsigned long long)expected_nodes);
        if (scanned_total != expected_nodes)
            fprintf(stderr, "  MISMATCH: scanned %llu nodes != expected %llu\n",
                    (unsigned long long)scanned_total,
                    (unsigned long long)expected_nodes);
        if (scanned_persons != person_count)
            fprintf(stderr, "  MISMATCH: scanned %llu person nodes != inserted %llu\n",
                    (unsigned long long)scanned_persons,
                    (unsigned long long)person_count);
        if (scanned_posts != post_count)
            fprintf(stderr, "  MISMATCH: scanned %llu post nodes != inserted %llu "
                    "(type bits may have been dropped — is B64 active?)\n",
                    (unsigned long long)scanned_posts,
                    (unsigned long long)post_count);
        if (atomic_count == expected_nodes && scanned_total == expected_nodes &&
            scanned_persons == person_count && scanned_posts == post_count)
            fprintf(stderr, "  OK: all counts match\n");

        // Metadata table state — num_nodes is written only on close(true).
        // It will read 0 here because sync_metadata() has not been called yet.
        fprintf(stderr, "\n=== Metadata table (WT) ===\n");
        graph.dump_meta_data();
        fprintf(stderr, "  Note: num_nodes/num_edges above are 0 because sync_metadata()\n"
                "  is only called by close(true) — close(false) skips the flush.\n");
        fprintf(stderr, "\n");
    }

    // ---- Pick sample IDs for queries ----
    // Use person 0 and person 1 as query subjects (always exist if SF > 0)
    node_id_t sample_person0 = MAKE_TYPED_ID(VT_PERSON, 0);
    node_id_t sample_person1 = (person_count > 1) ? MAKE_TYPED_ID(VT_PERSON, 1)
                                                   : sample_person0;
    node_id_t sample_post0   = (post_count > 0) ? MAKE_TYPED_ID(VT_POST, 0)
                                                 : OutOfBand_ID_MAX;

    fprintf(stderr, "=== WRITE QUERIES ===\n");

    // W1: insert a new person
    w1_insert_person(graph, person_count, opts.has_node_props);  // person_count used as counter only

    // W2: insert knows edge 0→1 (may already exist, but demonstrates the API)
    if (person_count >= 2) {
        w2_insert_knows(graph, sample_person0, sample_person1,
                        1700000001000LL, opts.has_edge_props);
    }

    // W3: insert a new post + hasCreator edge
    node_id_t next_post_id = MAKE_TYPED_ID(VT_POST, post_count);
    w3_insert_post_with_creator(graph, next_post_id, sample_person0,
                                1700000002000LL, 42, opts.has_node_props);

    fprintf(stderr, "\n=== SINGLE-TYPE READ QUERIES ===\n");

    // R1: person profile
    r1_person_profile(graph, sample_person0);

    // R2: friends sorted by date
    r2_friends_sorted_by_date(graph, sample_person0, opts.has_edge_props,
                               opts.prop_mode, &col_times.r2);

    // R3: BFS shortest path (may be slow for large graphs — demo only)
    if (person_count >= 2)
        r3_bfs_shortest_path(graph, sample_person0, sample_person1);

    fprintf(stderr, "\n=== CROSS-TYPE READ QUERIES ===\n");

    // X1: post profile
    if (sample_post0 != OutOfBand_ID_MAX)
        x1_post_profile(graph, sample_post0);

    // X2: post → author
    if (sample_post0 != OutOfBand_ID_MAX)
        x2_post_author(graph, sample_post0);

    // X3: IC-2 friends' recent posts
    {
        int64_t cutoff = INT64_MAX;  // all posts before "now"
        x3_ic2_friends_recent_posts(graph, sample_person0, cutoff, 10,
                                    opts.has_node_props);
    }

    // X4: insert a likes edge
    if (sample_post0 != OutOfBand_ID_MAX && sample_person1 != sample_person0) {
        x4_insert_likes(graph, sample_person1, sample_post0,
                        1700000003000LL, opts.has_edge_props);
    }

    // X5: count likes in date range
    if (sample_post0 != OutOfBand_ID_MAX) {
        x5_count_likes_in_range(graph, sample_post0,
                                0LL, INT64_MAX,
                                opts.has_edge_props);
    }

    fprintf(stderr, "\n=== AGGREGATE QUERIES ===\n");

    // A1: degree count
    a1_degree_count(graph, sample_person0);

    // A2: knows edges in date range for person 0
    a2_knows_in_date_range(graph, sample_person0,
                           0LL, INT64_MAX,
                           opts.has_edge_props, opts.prop_mode, &col_times.a2);

    // A3: posts liked in date range for person 1
    if (sample_person1 != sample_person0)
        a3_posts_liked_in_range(graph, sample_person1,
                                0LL, INT64_MAX,
                                opts.has_edge_props, opts.prop_mode, &col_times.a3);

    if (opts.prop_mode == COLUMNAR && !dry_run) {
        fprintf(stderr, "\n=== BI QUERIES (COLUMNAR mode) ===\n");
        bi1_posting_summary(graph, post_count, &col_times.bi1);
        bi12_message_distribution(graph, post_count);
        bi12_message_distribution_fast(graph, post_count, INT64_MAX, 0, &col_times.bi12_fast);
    }

    fprintf(stderr, "\n=== All queries complete ===\n");

    // ---- EMBEDDED vs COLUMNAR timing comparison ----
    if (!dry_run && opts.prop_mode == COLUMNAR) {
        fprintf(stderr, "\n=== EMBEDDED vs COLUMNAR timing comparison ===\n");
        fprintf(stderr, "%-36s  %14s  %14s  %10s\n",
                "Query", "EMBEDDED (ms)", "COLUMNAR (ms)", "Speedup");
        fprintf(stderr, "%-36s  %14s  %14s  %10s\n",
                "-----", "-------------", "-------------", "-------");

        auto print_row = [](const char *name, double emb, double col) {
            if (emb < 0.0) {
                fprintf(stderr, "%-36s  %14s  %14.3f  %10s\n", name, "N/A", col, "—");
            } else if (col < 0.0) {
                fprintf(stderr, "%-36s  %14.3f  %14s  %10s\n", name, emb, "N/A", "—");
            } else {
                double speedup = (col > 0.0) ? emb / col : 0.0;
                fprintf(stderr, "%-36s  %14.3f  %14.3f  %9.2fx\n",
                        name, emb, col, speedup);
            }
        };

        print_row("r2_friends_sorted_by_date",    emb_times.r2,        col_times.r2);
        print_row("a2_knows_in_date_range",        emb_times.a2,        col_times.a2);
        print_row("a3_posts_liked_in_range",       emb_times.a3,        col_times.a3);
        print_row("bi1_posting_summary",           -1.0,                col_times.bi1);
        print_row("bi12_message_distribution_fast",-1.0,                col_times.bi12_fast);
        fprintf(stderr, "\n");
    }

    graph_ptr->close(false);
    engine.close_graph();
    return 0;
}
