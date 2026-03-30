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
// Implements 22 queries across 5 categories:
//   Writes          (3): insert Person, insert knows, insert Post+hasCreator
//   Single-type (3): person profile, friends list by date, BFS shortest path
//   Cross-type  (5): post profile, post→author, IC-2 friends' posts,
//                    insert likes, count likes in date range
//   Aggregates  (3): degree count, knows edges in date range, posts liked in range
//   BI queries  (2): BI-1 posting summary, BI-12 message distribution per person
//   New (C1-C4) (6): IC-7 message likes, IC-9 friends' messages, IC-8 latest replies,
//                    BI-2 two-window message count, IC-5 popular forums, IC-3 FoF by country

#include <omp.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <map>
#include <mutex>
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
static inline bool is_comment(node_id_t id) { return VTYPE_OF(id) == VT_COMMENT; }
static inline bool is_forum(node_id_t id)   { return VTYPE_OF(id) == VT_FORUM; }

// Sentinel for "not found" in graph lookups
static constexpr node_id_t ID_NOT_FOUND = (node_id_t)UINT64_MAX;

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
    int64_t     creation_date = 0;
    int32_t     length = 0;
    int8_t      tag = 0;          // 0=content, 1=imageFile
    std::string content;
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
        p.tag           = SNBPostSchema::get_tag(pb.data);
        p.content       = SNBPostSchema::get_content(pb.data, pb.size);
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
    double w1        = -1.0;
    double w2        = -1.0;
    double w3        = -1.0;
    double r1        = -1.0;
    double r2        = -1.0;
    double r3        = -1.0;
    double x1        = -1.0;
    double x2        = -1.0;
    double x3        = -1.0;
    double x4        = -1.0;
    double x5        = -1.0;
    double a1        = -1.0;
    double a2        = -1.0;
    double a3        = -1.0;
    double bi1           = -1.0;
    double bi12          = -1.0;
    double bi12_fast     = -1.0;
    double bi1_par       = -1.0;
    double bi12_fast_par = -1.0;
    // C1–C4 new queries
    double ic7_message_likes    = -1.0;
    double ic8_latest_replies   = -1.0;
    double ic9_friends_messages = -1.0;
    double bi2_two_windows      = -1.0;
    double ic5_forums           = -1.0;
    double ic3_fof_country      = -1.0;
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
                             bool has_props, double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(w1_insert_person, *out_ms) }
    else        { TIME_END(w1_insert_person) }
    fprintf(stderr, "  inserted Person id=%llu\n", (unsigned long long)new_id);
}

// W2: Insert a knows edge between two existing persons
static void w2_insert_knows(GraphBase &graph, node_id_t src, node_id_t dst,
                            int64_t creation_date, bool has_props,
                            double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(w2_insert_knows, *out_ms) }
    else        { TIME_END(w2_insert_knows) }
    fprintf(stderr, "  inserted knows %llu→%llu\n",
            (unsigned long long)src, (unsigned long long)dst);
}

// W3: Insert a Post vertex and its hasCreator edge
static void w3_insert_post_with_creator(GraphBase &graph,
                                        node_id_t &next_post_id,
                                        node_id_t author_id,
                                        int64_t creation_date,
                                        int32_t length,
                                        bool has_props,
                                        double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(w3_insert_post_hasCreator, *out_ms) }
    else        { TIME_END(w3_insert_post_hasCreator) }
    fprintf(stderr, "  inserted Post id=%llu, hasCreator→%llu\n",
            (unsigned long long)post_id, (unsigned long long)author_id);
}

// ============================================================
// ============================================================
//  SINGLE-TYPE READ QUERIES
// ============================================================
// ============================================================

// R1: Person profile lookup — fetch all properties of a person by ID
static PersonProps r1_person_profile(GraphBase &graph, node_id_t pid,
                                     double *out_ms = nullptr)
{
    SEP();
    TIME_START(r1_person_profile)
    prop_blob pb = graph.get_node_properties(pid);
    PersonProps p = decode_person(pb);
    if (out_ms) { TIME_END_CAP(r1_person_profile, *out_ms) }
    else        { TIME_END(r1_person_profile) }
    auto emails = graph.get_person_emails(pid);
    auto langs  = graph.get_person_languages(pid);

    fprintf(stderr, "  Person %llu: %s %s  gender=%d  birthday=%lld  creationDate=%lld\n"
                    "              browser=%s  ip=%s\n",
            (unsigned long long)pid,
            p.first_name, p.last_name, (int)p.gender,
            (long long)p.birthday, (long long)p.creation_date,
            p.browser_used, p.location_ip);

    if (!emails.empty()) {
        fprintf(stderr, "              emails:");
        for (const auto &e : emails) fprintf(stderr, " %s", e.c_str());
        fprintf(stderr, "\n");
    }
    if (!langs.empty()) {
        fprintf(stderr, "              speaks:");
        for (const auto &l : langs) fprintf(stderr, " %s", l.c_str());
        fprintf(stderr, "\n");
    }
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
static int r3_bfs_shortest_path(GraphBase &graph, node_id_t src, node_id_t dst,
                                 double *out_ms = nullptr)
{
    SEP();
    TIME_START(r3_bfs_shortest_path)

    if (src == dst) {
        if (out_ms) { TIME_END_CAP(r3_bfs_shortest_path, *out_ms) }
        else        { TIME_END(r3_bfs_shortest_path) }
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

    if (out_ms) { TIME_END_CAP(r3_bfs_shortest_path, *out_ms) }
    else        { TIME_END(r3_bfs_shortest_path) }
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
static PostProps x1_post_profile(GraphBase &graph, node_id_t post_id,
                                  double *out_ms = nullptr)
{
    SEP();
    TIME_START(x1_post_profile)
    prop_blob pb = graph.get_node_properties(post_id);
    PostProps p = decode_post(pb);
    if (out_ms) { TIME_END_CAP(x1_post_profile, *out_ms) }
    else        { TIME_END(x1_post_profile) }
    const char *type_str = (p.tag == 0) ? "content" : "imageFile";
    std::string preview = p.content.substr(0, 60);
    fprintf(stderr, "  Post %llu: creationDate=%lld length=%d [%s] \"%s%s\"\n",
            (unsigned long long)post_id, (long long)p.creation_date, p.length,
            type_str, preview.c_str(), p.content.size() > 60 ? "..." : "");
    return p;
}

// X2: Post → author (hasCreator reverse lookup)
// In our model: hasCreator is stored as Post→Person out-edge.
// So: look at out-edges of a Post, find the one pointing to a Person.
static node_id_t x2_post_author(GraphBase &graph, node_id_t post_id,
                                 double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(x2_post_author, *out_ms) }
    else        { TIME_END(x2_post_author) }
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
//
// ORIGINAL ALGORITHM (slow):
//   For each friend → get_in_nodes_id() → for each post → get_node_properties()
//   get_node_properties() for a Post seeks the full post_props table, which
//   stores (creationDate, length, tag, content) with a variable-length content
//   string.  Each seek loads a full B-tree leaf page just to read 8 bytes of
//   creationDate, and seeks are random in post_id order.
//
// FIX 1 — sort post IDs before property lookups:
//   Collect all candidate post IDs from the topology scan first, then sort
//   them.  post_props:temporal is a B-tree keyed by post_id; sorted seeks
//   advance the cursor monotonically, turning O(N) random page loads into a
//   near-sequential scan and dramatically improving buffer-pool hit rates.
//
// FIX 2 — use the temporal colgroup instead of the full post_props table:
//   post_props:temporal stores only (creationDate:Q, length:i) = 12 B/row,
//   vs the full table which appends the variable-length content string.
//   A denser B-tree means: more rows per leaf page, shallower tree height,
//   and better cache utilisation for both sequential and point accesses.
//   We only need creationDate here, so loading content bytes is pure waste.
static std::vector<std::pair<node_id_t, int64_t>>
x3_ic2_friends_recent_posts(GraphBase &graph, node_id_t pid,
                             int64_t cutoff_ms, int limit,
                             bool has_props, double *out_ms = nullptr)
{
    SEP();
    TIME_START(x3_ic2_friends_recent_posts)

    // Step 1: get friends (persons in out-edges of pid)
    std::vector<node_id_t> friends = graph.get_out_nodes_id(pid);

    // Step 2: collect all candidate post IDs across all friends.
    // Topology scan only — property lookups are deferred to Step 3 so we can
    // sort first (Fix 1).
    std::vector<node_id_t> candidate_posts;
    for (node_id_t friend_id : friends) {
        if (!is_person(friend_id)) continue;
        std::vector<node_id_t> in_nodes = graph.get_in_nodes_id(friend_id);
        for (node_id_t post_id : in_nodes) {
            if (is_post(post_id))
                candidate_posts.push_back(post_id);
        }
    }

    // Fix 1: sort post IDs so that colgroup seeks are monotonically increasing.
    // The temporal colgroup B-tree is keyed by post_id, so sorted order ≈
    // sequential leaf-page access rather than random seeks.
    std::sort(candidate_posts.begin(), candidate_posts.end());

    // Step 3: fetch creationDate for each candidate using the temporal colgroup.
    std::vector<std::pair<node_id_t, int64_t>> result;

    if (has_props && !candidate_posts.empty()) {
        // Fix 2: open one colgroup cursor on post_props:temporal for the whole
        // batch.  This reads only (creationDate:Q, length:i) = 12 B/row,
        // vs the full post_props table which loads the variable-length content
        // string on every page access.
        WT_CURSOR *cg = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t post_id : candidate_posts) {
            cg->set_key(cg, (uint64_t)post_id);
            if (cg->search(cg) != 0) continue;
            uint64_t cDate; int32_t length;
            cg->get_value(cg, &cDate, &length);
            if ((int64_t)cDate < cutoff_ms)
                result.emplace_back(post_id, (int64_t)cDate);
        }
        cg->close(cg);
    } else if (!has_props) {
        for (node_id_t post_id : candidate_posts)
            result.emplace_back(post_id, 0LL);
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if ((int)result.size() > limit)
        result.resize(limit);

    if (out_ms) { TIME_END_CAP(x3_ic2_friends_recent_posts, *out_ms) }
    else        { TIME_END(x3_ic2_friends_recent_posts) }
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
                             bool has_props, double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(x4_insert_likes, *out_ms) }
    else        { TIME_END(x4_insert_likes) }
    fprintf(stderr, "  inserted likes %llu→%llu\n",
            (unsigned long long)person_id, (unsigned long long)post_id);
}

// X5: Count likes on a post within a date range [lo_ms, hi_ms]
static int64_t x5_count_likes_in_range(GraphBase &graph, node_id_t post_id,
                                        int64_t lo_ms, int64_t hi_ms,
                                        bool has_props, double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(x5_count_likes_in_range, *out_ms) }
    else        { TIME_END(x5_count_likes_in_range) }
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
a1_degree_count(GraphBase &graph, node_id_t id, double *out_ms = nullptr)
{
    SEP();
    TIME_START(a1_degree_count)
    degree_t out = graph.get_out_degree(id);
    degree_t in  = graph.get_in_degree(id);
    if (out_ms) { TIME_END_CAP(a1_degree_count, *out_ms) }
    else        { TIME_END(a1_degree_count) }
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
static void bi12_message_distribution(GraphBase &graph, node_id_t total_posts,
                                       double *out_ms = nullptr)
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

    if (out_ms) { TIME_END_CAP(bi12_message_distribution, *out_ms) }
    else        { TIME_END(bi12_message_distribution) }

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
// OPTIMISATION 1: TWO SEQUENTIAL SCANS
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
//   Scan OUT_EDGES starting from the first VT_POST source (see Optimisation 2).
//   For each POST→PERSON edge whose src is in the qualifying set, increment
//   creator_count[dst].  Cost: one sequential read of the VT_POST slice of the
//   OUT_EDGES B-tree.
//
// Both passes are fully sequential; no random seeks.  Total cost:
//
//   O(N_posts + N_hasCreator)
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
// OPTIMISATION 2: SEEK PAST VT_PERSON EDGES IN PASS 2
// -----------------------------------------------------
// The OUT_EDGES table is a single B-tree keyed by (src_id, dst_id).  Vertex types
// are encoded in the top 8 bits of node_id_t via MAKE_TYPED_ID(type, counter):
//
//   VT_PERSON = 0  →  src keys in [0x0000000000000000, 0x00FFFFFFFFFFFFFF]
//   VT_POST   = 1  →  src keys in [0x0100000000000000, 0x01FFFFFFFFFFFFFF]
//
// Because VT_PERSON < VT_POST, all knows edges (Person→Person) occupy the LOW end
// of the B-tree and all hasCreator edges (Post→Person) occupy a HIGHER contiguous
// segment.  On SF3, knows edges account for ~17 M rows — roughly 7× more than the
// ~2.5 M hasCreator edges we actually need.
//
// By calling set_key_range() with start = (MAKE_TYPED_ID(VT_POST, 0), 1) we issue
// a single search_near() that positions the cursor directly at the first VT_POST
// row, skipping the entire VT_PERSON segment without reading any of it.
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

    // Pass 2: scan only the VT_POST slice of OUT_EDGES (see Optimisation 2 above).
    // set_key_range() issues one search_near() that positions the cursor at the
    // first VT_POST source, skipping all VT_PERSON (knows) rows entirely.
    std::unordered_map<node_id_t, int64_t> creator_count;

    EdgeCursor *ec = graph.get_edge_iter();
    ec->set_key_range({{MAKE_TYPED_ID(VT_POST, 0), 1},
                       {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
    edge found;
    ec->next(&found);
    while (found.src_id != OutOfBand_ID_MAX) {
        if (VTYPE_OF(found.src_id) != VT_POST) break; // past the VT_POST range
        if (VTYPE_OF(found.dst_id) == VT_PERSON) {
            if (no_filter || qualifying.count(found.src_id))
                creator_count[found.dst_id]++;
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
// Parallel BI queries
// ============================================================
//
// PARALLELISM STRATEGY
// ----------------------
// Both bi1 and bi12_fast are full sequential scans over the post_props:temporal
// colgroup (keyed by post_id) and/or the VT_POST slice of OUT_EDGES.  Because
// WiredTiger sessions are NOT thread-safe, each thread must use its own
// WT_SESSION and cursors.  engine.create_ro_graph_handle(chkpt) opens a fresh
// read-only session per call, so N threads get N independent sessions with no
// sharing.
//
// KEY RANGE SPLIT
// ----------------
// Post IDs are assigned sequentially: MAKE_TYPED_ID(VT_POST, 0) through
// MAKE_TYPED_ID(VT_POST, post_count-1).  Thread i scans:
//
//   [MAKE_TYPED_ID(VT_POST, i*chunk), MAKE_TYPED_ID(VT_POST, (i+1)*chunk))
//
// where chunk = ceil(post_count / n_threads).  This gives balanced slices
// without needing calculate_thread_offsets().
//
// MERGE
// ------
// Each thread accumulates into a thread-local map.  After the OMP barrier,
// the main thread merges all local maps into a single result by summing values.
// No locking is needed during the parallel phase.

// BI-1 (parallel): partition the post_props:temporal colgroup scan across
// n_threads, each with its own session and cursor.
// Uses the existing WiredTiger connection (conn) directly to avoid opening a
// second connection.  Handles are constructed with ro_opts (read_only=true,
// create_new=false, checkpoint_name set) so no GraphEngine machinery is needed.
static void bi1_posting_summary_parallel(WT_CONNECTION *conn,
                                          graph_opts &ro_opts,
                                          node_id_t total_posts,
                                          int n_threads,
                                          double *out_ms = nullptr)
{
    TIME_START(bi1_posting_summary_parallel)

    auto year_of = [](uint64_t epoch_ms) -> int {
        return (int)(epoch_ms / 31557600000ULL) + 1970;
    };
    auto classify_length = [](int32_t len) -> int {
        if (len < 40)  return 0;
        if (len < 80)  return 1;
        if (len < 255) return 2;
        return 3;
    };

    using GroupMap = std::map<std::pair<int,int>, std::pair<int64_t,int64_t>>; // {count, sum_len}
    std::vector<GroupMap> local_groups(n_threads);

    // Compute post_id slices.  Post IDs are sequential from counter 0.
    node_id_t chunk = (total_posts + n_threads - 1) / n_threads;

    // Construct one read-only handle per thread using the existing connection.
    // This avoids opening a second WiredTiger connection and bypasses the
    // GraphEngine partitioning logic (not needed — we partition by post_id).
    std::vector<GraphBase*> bi1_handles(n_threads);
    for (int t = 0; t < n_threads; t++)
        bi1_handles[t] = (ro_opts.type == GraphType::Adj)
            ? (GraphBase*) new AdjList(ro_opts, conn)
            : (GraphBase*) new SplitEdgeKey(ro_opts, conn);

#pragma omp parallel for num_threads(n_threads) schedule(static,1)
    for (int t = 0; t < n_threads; t++) {
        node_id_t start_id = MAKE_TYPED_ID(VT_POST, (node_id_t)t * chunk);
        node_id_t end_id   = MAKE_TYPED_ID(VT_POST,
                                 std::min((node_id_t)(t + 1) * chunk, total_posts));

        WT_CURSOR *cg = bi1_handles[t]->open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);

        // Seek to this thread's start key.
        cg->set_key(cg, (uint64_t)start_id);
        int cmp = 0;
        bool ok = (cg->search_near(cg, &cmp) == 0);
        if (ok && cmp < 0) ok = (cg->next(cg) == 0);
        while (ok) {
            uint64_t vid, cDate; int32_t length;
            cg->get_key(cg, &vid);
            if ((node_id_t)vid >= end_id) break;
            cg->get_value(cg, &cDate, &length);
            int yr  = year_of(cDate);
            int cat = classify_length(length);
            auto &s = local_groups[t][{yr, cat}];
            s.first++;
            s.second += length;
            ok = (cg->next(cg) == 0);
        }
        cg->close(cg);
    }

    for (int t = 0; t < n_threads; t++)
        bi1_handles[t]->close(false);

    // Merge thread-local maps into one.
    std::map<std::pair<int,int>, std::pair<int64_t,int64_t>> groups;
    for (int t = 0; t < n_threads; t++) {
        for (auto &[k, s] : local_groups[t]) {
            groups[k].first  += s.first;
            groups[k].second += s.second;
        }
    }

    if (out_ms) { TIME_END_CAP(bi1_posting_summary_parallel, *out_ms) }
    else        { TIME_END(bi1_posting_summary_parallel) }

    fprintf(stderr, "  BI-1-parallel Posting Summary (%llu posts, %d threads):\n",
            (unsigned long long)total_posts, n_threads);
    fprintf(stderr, "  %-6s  %-3s  %-9s  %-8s  %-14s\n",
            "year", "cat", "count", "sum_len", "avg_len");
    for (auto &[k, s] : groups) {
        double avg = s.first > 0 ? (double)s.second / s.first : 0.0;
        fprintf(stderr, "  %-6d  %-3d  %-9lld  %-8lld  %-14.2f\n",
                k.first, k.second, (long long)s.first, (long long)s.second, avg);
    }
}

// BI-12-fast (parallel): partition the VT_POST slice of OUT_EDGES across
// n_threads.  Each thread scans its post_id range and accumulates a local
// creator_count map.  Maps are merged (summed) after the parallel section.
//
// When a date/length filter is active (Pass 1), the qualifying set is built
// with a parallel colgroup scan first, then shared (read-only) across all
// threads in Pass 2.
static void bi12_message_distribution_fast_parallel(WT_CONNECTION *conn,
                                                     graph_opts &ro_opts,
                                                     node_id_t total_posts,
                                                     int n_threads,
                                                     int64_t max_date   = INT64_MAX,
                                                     int32_t min_length = 0,
                                                     double *out_ms     = nullptr)
{
    TIME_START(bi12_message_distribution_fast_parallel)

    bool no_filter = (max_date == INT64_MAX && min_length == 0);
    node_id_t chunk = (total_posts + n_threads - 1) / n_threads;

    // Pass 1 (parallel): build qualifying post set from temporal colgroup.
    // Each thread scans its post slice; results are merged into one set.
    // Skipped entirely when no filter is active.
    std::unordered_set<node_id_t> qualifying;
    if (!no_filter) {
        std::vector<std::unordered_set<node_id_t>> local_q(n_threads);
        // Construct handles directly from the existing connection.
        std::vector<GraphBase*> p1_handles(n_threads);
        for (int t = 0; t < n_threads; t++)
            p1_handles[t] = (ro_opts.type == GraphType::Adj)
                ? (GraphBase*) new AdjList(ro_opts, conn)
                : (GraphBase*) new SplitEdgeKey(ro_opts, conn);

#pragma omp parallel for num_threads(n_threads) schedule(static,1)
        for (int t = 0; t < n_threads; t++) {
            node_id_t start_id = MAKE_TYPED_ID(VT_POST, (node_id_t)t * chunk);
            node_id_t end_id   = MAKE_TYPED_ID(VT_POST,
                                     std::min((node_id_t)(t + 1) * chunk, total_posts));

            WT_CURSOR *cg = p1_handles[t]->open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
            cg->set_key(cg, (uint64_t)start_id);
            int cmp = 0;
            bool ok = (cg->search_near(cg, &cmp) == 0);
            if (ok && cmp < 0) ok = (cg->next(cg) == 0);
            while (ok) {
                uint64_t vid, cDate; int32_t length;
                cg->get_key(cg, &vid);
                if ((node_id_t)vid >= end_id) break;
                cg->get_value(cg, &cDate, &length);
                if ((int64_t)cDate <= max_date && length >= min_length)
                    local_q[t].insert((node_id_t)vid);
                ok = (cg->next(cg) == 0);
            }
            cg->close(cg);
        }
        for (int t = 0; t < n_threads; t++)
            p1_handles[t]->close(false);

        qualifying.reserve(total_posts);
        for (int t = 0; t < n_threads; t++)
            qualifying.merge(local_q[t]);
    }

    // Pass 2 (parallel): scan OUT_EDGES for VT_POST sources, one slice per thread.
    // Each thread accumulates a local creator_count map; maps are summed at the end.
    std::vector<std::unordered_map<node_id_t, int64_t>> local_counts(n_threads);

    // Construct handles directly from the existing connection.
    std::vector<GraphBase*> p2_handles(n_threads);
    for (int t = 0; t < n_threads; t++)
        p2_handles[t] = (ro_opts.type == GraphType::Adj)
            ? (GraphBase*) new AdjList(ro_opts, conn)
            : (GraphBase*) new SplitEdgeKey(ro_opts, conn);

#pragma omp parallel for num_threads(n_threads) schedule(static,1)
    for (int t = 0; t < n_threads; t++) {
        node_id_t start_src = MAKE_TYPED_ID(VT_POST, (node_id_t)t * chunk);
        node_id_t end_src   = MAKE_TYPED_ID(VT_POST,
                                  std::min((node_id_t)(t + 1) * chunk, total_posts));

        EdgeCursor *ec = p2_handles[t]->get_edge_iter();

        // Seek to the first VT_POST edge in this thread's src range.
        // dst=1 satisfies the set_key_range "non-empty" condition while still
        // landing before any real edge (real dst IDs are VT_PERSON with high bits set).
        ec->set_key_range({{start_src, 1}, {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});

        edge found;
        ec->next(&found);
        while (found.src_id != OutOfBand_ID_MAX) {
            if (VTYPE_OF(found.src_id) != VT_POST) break;
            if (found.src_id >= end_src) break;  // past this thread's slice
            if (VTYPE_OF(found.dst_id) == VT_PERSON) {
                if (no_filter || qualifying.count(found.src_id))
                    local_counts[t][found.dst_id]++;
            }
            ec->next(&found);
        }
        delete ec;
    }
    for (int t = 0; t < n_threads; t++)
        p2_handles[t]->close(false);

    // Merge thread-local maps by summing counts for each creator.
    std::unordered_map<node_id_t, int64_t> creator_count;
    for (int t = 0; t < n_threads; t++) {
        for (auto &[pid, cnt] : local_counts[t])
            creator_count[pid] += cnt;
    }

    if (out_ms) { TIME_END_CAP(bi12_message_distribution_fast_parallel, *out_ms) }
    else        { TIME_END(bi12_message_distribution_fast_parallel) }

    std::vector<std::pair<node_id_t, int64_t>> ranked(creator_count.begin(), creator_count.end());
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) {
        if (a.second != b.second) return a.second > b.second;
        return a.first < b.first;
    });
    fprintf(stderr, "  BI-12-fast-parallel (%llu posts, %d threads, %zu creators):\n",
            (unsigned long long)total_posts, n_threads, ranked.size());
    int shown = 0;
    for (auto &[pid, cnt] : ranked) {
        if (shown++ >= 10) { fprintf(stderr, "  ... (showing top 10)\n"); break; }
        fprintf(stderr, "  person %llu: %lld posts\n",
                (unsigned long long)VCOUNTER_OF(pid), (long long)cnt);
    }
}

// ============================================================
// C1: Comment-type queries (IC-7, IC-9, IC-8, BI-2)
// ============================================================

// IC-7: For Person P, find who liked P's messages (Posts + Comments).
// Algorithm: get P's messages via hasCreator in-edges → for each message get its
// Person in-neighbors (likes) → batch-seek likes_props:temporal sorted by (person,msg).
// Demonstrates: likes_props:temporal colgroup point-seek batch (CP-4 partial).
// Returns up to `limit` (liker_id, message_id, like_date) sorted by like_date desc.
static std::vector<std::tuple<node_id_t, node_id_t, int64_t>>
ic7_message_likes(GraphBase &graph, node_id_t pid,
                  int limit, bool has_props, double *out_ms = nullptr)
{
    SEP();
    TIME_START(ic7_message_likes)

    // Step 1: P's messages (in-edges of P where src is Post or Comment via hasCreator)
    std::vector<node_id_t> messages;
    for (node_id_t m : graph.get_in_nodes_id(pid)) {
        uint8_t vt = VTYPE_OF(m);
        if (vt == VT_POST || vt == VT_COMMENT) messages.push_back(m);
    }

    // Step 2: for each message, collect likers (in-edges where src is Person via likes)
    std::vector<std::pair<node_id_t, node_id_t>> liker_msg;  // (person_id, msg_id)
    for (node_id_t msg : messages)
        for (node_id_t n : graph.get_in_nodes_id(msg))
            if (VTYPE_OF(n) == VT_PERSON) liker_msg.emplace_back(n, msg);

    // Step 3: batch-seek likes_props:temporal sorted by (person_id, msg_id)
    // Sorted order turns random seeks into monotone cursor advances.
    std::sort(liker_msg.begin(), liker_msg.end());
    std::vector<std::tuple<node_id_t, node_id_t, int64_t>> result;

    if (has_props && !liker_msg.empty()) {
        WT_CURSOR *cg = graph.open_colgroup_cursor(LIKES_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[liker, msg] : liker_msg) {
            cg->set_key(cg, (uint64_t)liker, (uint64_t)msg);
            if (cg->search(cg) != 0) continue;
            uint64_t cDate;
            cg->get_value(cg, &cDate);
            result.emplace_back(liker, msg, (int64_t)cDate);
        }
        cg->close(cg);
    } else {
        for (auto &[liker, msg] : liker_msg)
            result.emplace_back(liker, msg, 0LL);
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return std::get<2>(a) > std::get<2>(b); });
    if ((int)result.size() > limit) result.resize(limit);

    if (out_ms) { TIME_END_CAP(ic7_message_likes, *out_ms) }
    else        { TIME_END(ic7_message_likes) }
    fprintf(stderr, "  IC-7 Person %llu: %zu message likes\n",
            (unsigned long long)pid, result.size());
    int shown = 0;
    for (auto &[liker, msg, cd] : result) {
        fprintf(stderr, "    liker=%llu msg=%llu date=%lld\n",
                (unsigned long long)liker, (unsigned long long)msg, (long long)cd);
        if (++shown >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// IC-9: Posts and Comments by Person P's friends created before cutoff_ms.
// Algorithm: get friends → collect (msg, creator) from each friend's hasCreator in-edges
// → sort msg IDs → batch-seek post_props:temporal + comment_props:temporal.
// Demonstrates: dual colgroup batch scan over two message type tables (CP-4).
// Returns up to `limit` (msg_id, creationDate, creator_id) sorted by date desc.
static std::vector<std::tuple<node_id_t, int64_t, node_id_t>>
ic9_friends_messages_before(GraphBase &graph, node_id_t pid,
                             int64_t cutoff_ms, int limit,
                             bool has_props, double *out_ms = nullptr)
{
    SEP();
    TIME_START(ic9_friends_messages_before)

    // Step 1: get P's friends
    std::vector<node_id_t> friends;
    for (node_id_t f : graph.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends.push_back(f);

    // Step 2: collect all (msg_id, creator_id) from friends' hasCreator in-edges
    std::vector<std::pair<node_id_t, node_id_t>> msg_creator;
    for (node_id_t f : friends)
        for (node_id_t m : graph.get_in_nodes_id(f)) {
            uint8_t vt = VTYPE_OF(m);
            if (vt == VT_POST || vt == VT_COMMENT) msg_creator.emplace_back(m, f);
        }

    // Step 3: sort by msg_id for monotone colgroup seeks
    std::sort(msg_creator.begin(), msg_creator.end());
    std::vector<std::tuple<node_id_t, int64_t, node_id_t>> result;

    if (has_props && !msg_creator.empty()) {
        WT_CURSOR *post_cg    = graph.open_colgroup_cursor(POST_PROPS_TABLE,    CG_TEMPORAL);
        WT_CURSOR *comment_cg = graph.open_colgroup_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[msg, creator] : msg_creator) {
            WT_CURSOR *cg = (VTYPE_OF(msg) == VT_POST) ? post_cg : comment_cg;
            cg->set_key(cg, (uint64_t)msg);
            if (cg->search(cg) != 0) continue;
            uint64_t cDate; int32_t length;
            cg->get_value(cg, &cDate, &length);
            if ((int64_t)cDate < cutoff_ms)
                result.emplace_back(msg, (int64_t)cDate, creator);
        }
        post_cg->close(post_cg);
        comment_cg->close(comment_cg);
    } else {
        for (auto &[msg, creator] : msg_creator)
            result.emplace_back(msg, 0LL, creator);
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return std::get<1>(a) > std::get<1>(b); });
    if ((int)result.size() > limit) result.resize(limit);

    if (out_ms) { TIME_END_CAP(ic9_friends_messages_before, *out_ms) }
    else        { TIME_END(ic9_friends_messages_before) }
    fprintf(stderr, "  IC-9 Person %llu (before %lld): %zu messages\n",
            (unsigned long long)pid, (long long)cutoff_ms, result.size());
    int shown = 0;
    for (auto &[msg, cd, creator] : result) {
        fprintf(stderr, "    msg=%llu date=%lld creator=%llu\n",
                (unsigned long long)msg, (long long)cd, (unsigned long long)creator);
        if (++shown >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// IC-8: Latest replies to Person P's posts and comments.
// Algorithm: get P's messages via hasCreator in-edges → for each message get
// Comment in-neighbors (replyOf edges Comment→Post/Comment) → sort reply IDs
// → batch-seek comment_props:temporal.
// Demonstrates: replyOf chain traversal — CP-1 chokepoint.
// Returns up to `limit` (reply_id, creationDate, reply_creator_id) sorted by date desc.
static std::vector<std::tuple<node_id_t, int64_t, node_id_t>>
ic8_latest_replies(GraphBase &graph, node_id_t pid,
                   int limit, bool has_props, double *out_ms = nullptr)
{
    SEP();
    TIME_START(ic8_latest_replies)

    // Step 1: get P's messages (hasCreator: Post/Comment → Person)
    std::vector<node_id_t> messages;
    for (node_id_t m : graph.get_in_nodes_id(pid)) {
        uint8_t vt = VTYPE_OF(m);
        if (vt == VT_POST || vt == VT_COMMENT) messages.push_back(m);
    }

    // Step 2: for each message, collect replying comments
    // replyOf: Comment→Post and Comment→Comment; so in-edges of msg where src is Comment
    std::vector<node_id_t> replies;
    for (node_id_t msg : messages)
        for (node_id_t n : graph.get_in_nodes_id(msg))
            if (VTYPE_OF(n) == VT_COMMENT) replies.push_back(n);

    // Step 3: sort reply IDs, batch-fetch creationDate from comment_props:temporal
    std::sort(replies.begin(), replies.end());
    std::vector<std::tuple<node_id_t, int64_t, node_id_t>> result;

    if (has_props && !replies.empty()) {
        WT_CURSOR *cg = graph.open_colgroup_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t cid : replies) {
            cg->set_key(cg, (uint64_t)cid);
            if (cg->search(cg) != 0) continue;
            uint64_t cDate; int32_t length;
            cg->get_value(cg, &cDate, &length);
            result.emplace_back(cid, (int64_t)cDate, (node_id_t)0);
        }
        cg->close(cg);
    } else {
        for (node_id_t cid : replies) result.emplace_back(cid, 0LL, (node_id_t)0);
    }

    // Sort by date desc, keep top `limit`
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return std::get<1>(a) > std::get<1>(b); });
    if ((int)result.size() > limit) result.resize(limit);

    // Fetch creator for top results only (out-edges of comment where dst is Person)
    for (auto &[cid, cdate, creator] : result)
        for (node_id_t n : graph.get_out_nodes_id(cid))
            if (VTYPE_OF(n) == VT_PERSON) { creator = n; break; }

    if (out_ms) { TIME_END_CAP(ic8_latest_replies, *out_ms) }
    else        { TIME_END(ic8_latest_replies) }
    fprintf(stderr, "  IC-8 Person %llu: %zu replies (replyOf-chain CP-1)\n",
            (unsigned long long)pid, result.size());
    int shown = 0;
    for (auto &[cid, cd, creator] : result) {
        fprintf(stderr, "    reply=%llu date=%lld creator=%llu\n",
                (unsigned long long)cid, (long long)cd, (unsigned long long)creator);
        if (++shown >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// BI-2 (simplified): Count Posts and Comments in two separate date windows.
// Full BI-2 groups by tag, but this simplified version counts by message type per window.
// Demonstrates: dual sequential colgroup scan (post_props:temporal + comment_props:temporal)
// with a date filter — CP-4 (dimension FK join omitted; see IC-3 for FK join variant).
static void bi2_message_count_two_windows(GraphBase &graph,
                                           int64_t lo1, int64_t hi1,
                                           int64_t lo2, int64_t hi2,
                                           double *out_ms = nullptr)
{
    TIME_START(bi2_message_count_two_windows)

    int64_t posts_w1=0, posts_w2=0, comments_w1=0, comments_w2=0;

    // Scan post_props:temporal
    {
        WT_CURSOR *cur = graph.open_colgroup_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        while (cur->next(cur) == 0) {
            uint64_t vid, cDate; int32_t length;
            cur->get_key(cur, &vid);
            cur->get_value(cur, &cDate, &length);
            int64_t t = (int64_t)cDate;
            if (t >= lo1 && t < hi1) posts_w1++;
            if (t >= lo2 && t < hi2) posts_w2++;
        }
        cur->close(cur);
    }
    // Scan comment_props:temporal
    {
        WT_CURSOR *cur = graph.open_colgroup_cursor(COMMENT_PROPS_TABLE, CG_TEMPORAL);
        while (cur->next(cur) == 0) {
            uint64_t vid, cDate; int32_t length;
            cur->get_key(cur, &vid);
            cur->get_value(cur, &cDate, &length);
            int64_t t = (int64_t)cDate;
            if (t >= lo1 && t < hi1) comments_w1++;
            if (t >= lo2 && t < hi2) comments_w2++;
        }
        cur->close(cur);
    }

    if (out_ms) { TIME_END_CAP(bi2_message_count_two_windows, *out_ms) }
    else        { TIME_END(bi2_message_count_two_windows) }

    fprintf(stderr, "  BI-2 message counts:\n");
    fprintf(stderr, "    W1 [%lld,%lld): posts=%lld comments=%lld total=%lld\n",
            (long long)lo1,(long long)hi1,(long long)posts_w1,(long long)comments_w1,
            (long long)(posts_w1+comments_w1));
    fprintf(stderr, "    W2 [%lld,%lld): posts=%lld comments=%lld total=%lld\n",
            (long long)lo2,(long long)hi2,(long long)posts_w2,(long long)comments_w2,
            (long long)(posts_w2+comments_w2));
}

// ============================================================
// C2: Forum-type queries (IC-5)
// ============================================================

// IC-5: Forums where Person P's friends joined since `since_ms`.
// Algorithm: friends → in-edges of each friend where src is Forum (hasMember edges)
// → sort (forum_id, friend_id) → batch-seek hasmember_props:temporal for date filter
// → group by forum, count qualifying members.
// Demonstrates: high-degree Forum fan-out — CP-3 chokepoint.
// Returns up to `limit` (forum_id, member_count) sorted by member_count desc.
static std::vector<std::pair<node_id_t, int32_t>>
ic5_forums_by_friend_membership(GraphBase &graph, node_id_t pid,
                                  int64_t since_ms, int limit,
                                  bool has_props, double *out_ms = nullptr)
{
    SEP();
    TIME_START(ic5_forums_by_friend_membership)

    // Step 1: get P's friends
    std::vector<node_id_t> friends;
    for (node_id_t f : graph.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends.push_back(f);

    // Step 2: for each friend, find forums via hasMember in-edges
    // hasMember: Forum→Person; so in-edges of person where src is Forum
    std::vector<std::pair<node_id_t, node_id_t>> forum_member;  // (forum_id, friend_id)
    for (node_id_t f : friends)
        for (node_id_t n : graph.get_in_nodes_id(f))
            if (VTYPE_OF(n) == VT_FORUM) forum_member.emplace_back(n, f);

    // Step 3: sort and batch-seek hasmember_props:temporal for date filter
    std::sort(forum_member.begin(), forum_member.end());
    std::unordered_map<node_id_t, int32_t> forum_count;

    if (has_props && !forum_member.empty()) {
        WT_CURSOR *cg = graph.open_colgroup_cursor(HASMEMBER_PROPS_TABLE, CG_TEMPORAL);
        for (auto &[fid, mid] : forum_member) {
            cg->set_key(cg, (uint64_t)fid, (uint64_t)mid);
            if (cg->search(cg) != 0) continue;
            uint64_t cDate;
            cg->get_value(cg, &cDate);
            if ((int64_t)cDate >= since_ms) forum_count[fid]++;
        }
        cg->close(cg);
    } else {
        for (auto &[fid, mid] : forum_member) forum_count[fid]++;
    }

    std::vector<std::pair<node_id_t, int32_t>> result(forum_count.begin(), forum_count.end());
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if ((int)result.size() > limit) result.resize(limit);

    if (out_ms) { TIME_END_CAP(ic5_forums_by_friend_membership, *out_ms) }
    else        { TIME_END(ic5_forums_by_friend_membership) }
    fprintf(stderr, "  IC-5 Person %llu since %lld: %zu forums (Forum fan-out CP-3)\n",
            (unsigned long long)pid, (long long)since_ms, result.size());
    int shown = 0;
    for (auto &[fid, cnt] : result) {
        fprintf(stderr, "    forum=%llu members=%d\n", (unsigned long long)fid, cnt);
        if (++shown >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// ============================================================
// C4: Place/Organisation queries (IC-3)
// ============================================================

// Resolve a Person's country of residence: Person→City (isLocatedIn)→Country (isPartOf).
// Returns the Country node_id, or ID_NOT_FOUND if the location chain is absent.
static node_id_t resolve_person_country(GraphBase &graph, node_id_t person_id)
{
    for (node_id_t city : graph.get_out_nodes_id(person_id)) {
        if (VTYPE_OF(city) != VT_CITY) continue;
        for (node_id_t country : graph.get_out_nodes_id(city))
            if (VTYPE_OF(country) == VT_COUNTRY) return country;
    }
    return ID_NOT_FOUND;
}

// IC-3: Friends-of-friends of Person P who are located in countryX but not countryY.
// Algorithm: BFS 2-hop knows expansion → exclude P and direct friends → for each FoF
// candidate resolve country via isLocatedIn+isPartOf chain → filter by countryX/not-Y
// → count common friends with P → sort by common friends desc.
// Demonstrates: 3-hop cross-type join (knows×2 + isLocatedIn + isPartOf) — CP-2 chokepoint.
// Returns up to `limit` (candidate_id, common_friends_count) sorted by count desc.
static std::vector<std::pair<node_id_t, int32_t>>
ic3_fof_by_country(GraphBase &graph, node_id_t pid,
                   node_id_t country_x, node_id_t country_y,
                   int limit, double *out_ms = nullptr)
{
    SEP();
    TIME_START(ic3_fof_by_country)

    // Step 1: get P's direct friends
    std::unordered_set<node_id_t> friends_set;
    for (node_id_t f : graph.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends_set.insert(f);

    // Step 2: 2-hop expansion — friends of friends, count shared friends with P
    std::unordered_map<node_id_t, int32_t> fof_common;
    for (node_id_t f : friends_set)
        for (node_id_t ff : graph.get_out_nodes_id(f)) {
            if (VTYPE_OF(ff) != VT_PERSON) continue;
            if (ff == pid || friends_set.count(ff)) continue;
            fof_common[ff]++;
        }

    // Step 3: filter by location — keep FoF in countryX but not countryY
    std::vector<std::pair<node_id_t, int32_t>> result;
    for (auto &[candidate, common] : fof_common) {
        node_id_t c = resolve_person_country(graph, candidate);
        if (c == ID_NOT_FOUND) continue;
        if (country_x != ID_NOT_FOUND && c != country_x) continue;
        if (country_y != ID_NOT_FOUND && c == country_y) continue;
        result.emplace_back(candidate, common);
    }

    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });
    if ((int)result.size() > limit) result.resize(limit);

    if (out_ms) { TIME_END_CAP(ic3_fof_by_country, *out_ms) }
    else        { TIME_END(ic3_fof_by_country) }
    fprintf(stderr, "  IC-3 Person %llu FoF in country %llu not %llu: %zu found (CP-2)\n",
            (unsigned long long)pid,
            country_x == ID_NOT_FOUND ? 0ULL : (unsigned long long)country_x,
            country_y == ID_NOT_FOUND ? 0ULL : (unsigned long long)country_y,
            result.size());
    int shown = 0;
    for (auto &[cand, common] : result) {
        fprintf(stderr, "    candidate=%llu common_friends=%d\n",
                (unsigned long long)cand, common);
        if (++shown >= 5) { fprintf(stderr, "    ...\n"); break; }
    }
    return result;
}

// ============================================================
// main
// ============================================================

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <data_dir> [graph_type] [--log <file>] [--dry-run] [--all]\n", argv[0]);
        fprintf(stderr, "  data_dir    directory containing LDBC SNB dynamic/ CSV files\n");
        fprintf(stderr, "  graph_type  adj | splitekey  (default: splitekey)\n");
        fprintf(stderr, "  --log <f>   write every NODE/EDGE insertion to <f> (converted IDs)\n");
        fprintf(stderr, "  --dry-run   parse CSVs and write log without inserting into the DB\n");
        fprintf(stderr, "  --all       also run Pass 1 (EMBEDDED baseline) for comparison\n");
        return 1;
    }

    std::string data_dir = argv[1];
    // Normalize path: remove trailing slash
    while (!data_dir.empty() && data_dir.back() == '/')
        data_dir.pop_back();

    std::string dyn = data_dir + "/dynamic";

    // ---- Parse optional flags ----
    GraphType graph_type = GraphType::SplitEKey;
    std::string graph_type_str = "splitekey";
    std::string insertion_log_path;
    bool dry_run = false;
    bool run_pass1 = false;  // Pass 1 (EMBEDDED baseline) only runs with --all

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "adj") {
            graph_type = GraphType::Adj;
            graph_type_str = "adj";
        } else if (arg == "splitekey") {
            graph_type = GraphType::SplitEKey;
            graph_type_str = "splitekey";
        }
        else if (arg == "--log" && i + 1 < argc)
            insertion_log_path = argv[++i];
        else if (arg == "--dry-run")
            dry_run = true;
        else if (arg == "--all")
            run_pass1 = true;
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
    // Skipped by default; enabled with --all.
    if (!dry_run && run_pass1) {
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
        emb_loader.load_person_emails(dyn + "/person_email_emailaddress_0_0.csv");
        emb_loader.load_person_speaks(dyn + "/person_speaks_language_0_0.csv");

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

    std::string sta = data_dir + "/static";

    // Helper: load a CSV, silently skip if the file is missing.
    auto safe_load = [](const char *label, auto fn) {
        fprintf(stderr, "[LOAD] %-52s ... ", label);
        try { fn(); fprintf(stderr, "done\n"); }
        catch (const std::exception &e) { fprintf(stderr, "skip (%s)\n", e.what()); }
    };

    // ---- Phase 1: load all vertices (add_node) ----
    // Static dimension types first (no edge dependencies).
    safe_load("places (city/country/continent)",
              [&]{ loader.load_places(sta + "/place_0_0.csv"); });
    safe_load("organisations (company/university)",
              [&]{ loader.load_organisations(sta + "/organisation_0_0.csv"); });
    safe_load("tags",
              [&]{ loader.load_tags(sta + "/tag_0_0.csv"); });
    safe_load("tagclasses",
              [&]{ loader.load_tagclasses(sta + "/tagclass_0_0.csv"); });
    // Dynamic vertex types
    safe_load("persons",
              [&]{ loader.load_persons(dyn + "/person_0_0.csv"); });
    safe_load("posts",
              [&]{ loader.load_posts(dyn + "/post_0_0.csv"); });
    safe_load("comments",
              [&]{ loader.load_comments(dyn + "/comment_0_0.csv"); });
    {
        // Build moderator map before loading forums so moderator_id can be embedded.
        std::unordered_map<int64_t, int64_t> mod_map;
        try { LDBCLoader::load_has_moderator_map(
                  dyn + "/forum_hasModerator_person_0_0.csv", mod_map); }
        catch (...) {}
        safe_load("forums",
                  [&]{ loader.load_forums(dyn + "/forum_0_0.csv", &mod_map); });
    }

    // ---- Phase 2: load all edges (add_edge) ----
    // Static structural edges (dimension graph)
    safe_load("place_isPartOf_place",
              [&]{ loader.load_is_part_of(sta + "/place_isPartOf_place_0_0.csv"); });
    safe_load("organisation_isLocatedIn_place",
              [&]{ loader.load_org_is_located_in(sta + "/organisation_isLocatedIn_place_0_0.csv"); });
    safe_load("tag_hasType_tagclass",
              [&]{ loader.load_has_type(sta + "/tag_hasType_tagclass_0_0.csv"); });
    safe_load("tagclass_isSubclassOf_tagclass",
              [&]{ loader.load_is_subclass_of(sta + "/tagclass_isSubclassOf_tagclass_0_0.csv"); });
    // Dynamic edges
    safe_load("person_knows_person",
              [&]{ loader.load_knows(dyn + "/person_knows_person_0_0.csv"); });
    safe_load("post_hasCreator_person",
              [&]{ loader.load_has_creator(dyn + "/post_hasCreator_person_0_0.csv"); });
    safe_load("comment_hasCreator_person",
              [&]{ loader.load_comment_has_creator(dyn + "/comment_hasCreator_person_0_0.csv"); });
    safe_load("comment_replyOf_post",
              [&]{ loader.load_reply_of_post(dyn + "/comment_replyOf_post_0_0.csv"); });
    safe_load("comment_replyOf_comment",
              [&]{ loader.load_reply_of_comment(dyn + "/comment_replyOf_comment_0_0.csv"); });
    safe_load("forum_containerOf_post",
              [&]{ loader.load_container_of(dyn + "/forum_containerOf_post_0_0.csv"); });
    safe_load("forum_hasMember_person",
              [&]{ loader.load_has_member(dyn + "/forum_hasMember_person_0_0.csv"); });
    safe_load("person_likes_post",
              [&]{ loader.load_likes(dyn + "/person_likes_post_0_0.csv"); });
    safe_load("person_likes_comment",
              [&]{ loader.load_likes_comment(dyn + "/person_likes_comment_0_0.csv"); });
    safe_load("post_hasTag_tag",
              [&]{ loader.load_post_has_tag(dyn + "/post_hasTag_tag_0_0.csv"); });
    safe_load("comment_hasTag_tag",
              [&]{ loader.load_comment_has_tag(dyn + "/comment_hasTag_tag_0_0.csv"); });
    safe_load("forum_hasTag_tag",
              [&]{ loader.load_forum_has_tag(dyn + "/forum_hasTag_tag_0_0.csv"); });
    safe_load("person_hasInterest_tag",
              [&]{ loader.load_has_interest(dyn + "/person_hasInterest_tag_0_0.csv"); });
    safe_load("person_isLocatedIn_place",
              [&]{ loader.load_person_is_located_in(dyn + "/person_isLocatedIn_place_0_0.csv"); });
    safe_load("post_isLocatedIn_place",
              [&]{ loader.load_post_is_located_in(dyn + "/post_isLocatedIn_place_0_0.csv"); });
    safe_load("comment_isLocatedIn_place",
              [&]{ loader.load_comment_is_located_in(dyn + "/comment_isLocatedIn_place_0_0.csv"); });
    safe_load("person_studyAt_organisation",
              [&]{ loader.load_study_at(dyn + "/person_studyAt_organisation_0_0.csv"); });
    safe_load("person_workAt_organisation",
              [&]{ loader.load_work_at(dyn + "/person_workAt_organisation_0_0.csv"); });

    // ---- Phase 3: flush node + edge properties ----
    safe_load("flush node props", [&]{ loader.flush_node_props(); });
    safe_load("flush edge props", [&]{ loader.flush_edge_props(); });

    // ---- Phase 4: secondary multi-valued tables ----
    safe_load("person emails",
              [&]{ loader.load_person_emails(dyn + "/person_email_emailaddress_0_0.csv"); });
    safe_load("person languages",
              [&]{ loader.load_person_speaks(dyn + "/person_speaks_language_0_0.csv"); });

    node_id_t person_count    = loader.person_count;
    node_id_t post_count      = loader.post_count;
    node_id_t comment_count   = loader.comment_count;
    node_id_t forum_count_n   = loader.forum_count;
    node_id_t tag_count_n     = loader.tag_count;
    node_id_t tagclass_count_n= loader.tagclass_count;
    node_id_t place_count_n   = loader.city_count + loader.country_count + loader.continent_count;
    node_id_t org_count_n     = loader.company_count + loader.university_count;

    fprintf(stderr, "\n=== Graph loaded: %llu persons, %llu posts, %llu comments,"
                    " %llu forums, %llu tags, %llu tagclasses,"
                    " %llu places, %llu orgs ===\n\n",
            (unsigned long long)person_count,
            (unsigned long long)post_count,
            (unsigned long long)comment_count,
            (unsigned long long)forum_count_n,
            (unsigned long long)tag_count_n,
            (unsigned long long)tagclass_count_n,
            (unsigned long long)place_count_n,
            (unsigned long long)org_count_n);

    // Checkpoint after load so that create_ro_graph_handle() (used by the
    // parallel BI queries) can open consistent read-only sessions.
    std::string chkpt;
    if (!dry_run)
        chkpt = engine.make_checkpoint();

    // ---- Node count verification ----
    // Cross-check three sources:
    //   (a) loader counters  — what we tried to insert
    //   (b) get_num_nodes()  — in-memory atomic counter incremented by add_node
    //   (c) get_nodes()      — full WT cursor scan (ground truth in the table)
    // Also breaks down (c) by vertex type to catch ID-partitioning bugs where
    // Post IDs lose their type bits and collide with Person IDs.
    if (!dry_run) {
        node_id_t expected_nodes = person_count + post_count + comment_count
                                 + forum_count_n + tag_count_n + tagclass_count_n
                                 + place_count_n + org_count_n;
        node_id_t atomic_count   = graph.get_num_nodes();

        fprintf(stderr, "=== Node count verification ===\n");
        fprintf(stderr, "  loader inserted:   %llu persons + %llu posts + %llu comments"
                        " + %llu forums + %llu tags + %llu tagclasses"
                        " + %llu places + %llu orgs = %llu total\n",
                (unsigned long long)person_count,
                (unsigned long long)post_count,
                (unsigned long long)comment_count,
                (unsigned long long)forum_count_n,
                (unsigned long long)tag_count_n,
                (unsigned long long)tagclass_count_n,
                (unsigned long long)place_count_n,
                (unsigned long long)org_count_n,
                (unsigned long long)expected_nodes);
        fprintf(stderr, "  get_num_nodes():   %llu  (in-memory atomic counter)\n",
                (unsigned long long)atomic_count);

        // Full WT scan — counts what is actually stored
        std::vector<node> all_nodes = graph.get_nodes();
        node_id_t scanned_persons=0, scanned_posts=0, scanned_comments=0;
        node_id_t scanned_forums=0, scanned_tags=0, scanned_tagclasses=0;
        node_id_t scanned_places=0, scanned_orgs=0, scanned_unknown=0;
        for (const node &nd : all_nodes) {
            uint8_t vt = VTYPE_OF(nd.id);
            if      (vt == VT_PERSON)                            scanned_persons++;
            else if (vt == VT_POST)                              scanned_posts++;
            else if (vt == VT_COMMENT)                           scanned_comments++;
            else if (vt == VT_FORUM)                             scanned_forums++;
            else if (vt == VT_TAG)                               scanned_tags++;
            else if (vt == VT_TAGCLASS)                          scanned_tagclasses++;
            else if (vt==VT_CITY||vt==VT_COUNTRY||vt==VT_CONTINENT) scanned_places++;
            else if (vt==VT_COMPANY||vt==VT_UNIVERSITY)          scanned_orgs++;
            else                                                 scanned_unknown++;
        }
        node_id_t scanned_total = (node_id_t)all_nodes.size();

        fprintf(stderr, "  get_nodes() scan:  %llu total"
                " (%llu persons, %llu posts, %llu comments,"
                " %llu forums, %llu tags, %llu tagclasses,"
                " %llu places, %llu orgs, %llu unknown)\n",
                (unsigned long long)scanned_total,
                (unsigned long long)scanned_persons,
                (unsigned long long)scanned_posts,
                (unsigned long long)scanned_comments,
                (unsigned long long)scanned_forums,
                (unsigned long long)scanned_tags,
                (unsigned long long)scanned_tagclasses,
                (unsigned long long)scanned_places,
                (unsigned long long)scanned_orgs,
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
            fprintf(stderr, "  MISMATCH: scanned %llu post nodes != inserted %llu\n",
                    (unsigned long long)scanned_posts, (unsigned long long)post_count);
        if (scanned_comments != comment_count)
            fprintf(stderr, "  MISMATCH: scanned %llu comment nodes != inserted %llu\n",
                    (unsigned long long)scanned_comments, (unsigned long long)comment_count);
        if (atomic_count == expected_nodes && scanned_total == expected_nodes)
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
    node_id_t sample_comment0 = (comment_count > 0)   ? MAKE_TYPED_ID(VT_COMMENT, 0)
                                                       : OutOfBand_ID_MAX;
    (void)sample_comment0;  // reserved for future comment-specific queries
    // Use first two countries for IC-3; ID_NOT_FOUND means "any country" (skip filter)
    node_id_t country_x = (loader.country_count > 0) ? MAKE_TYPED_ID(VT_COUNTRY, 0) : ID_NOT_FOUND;
    node_id_t country_y = (loader.country_count > 1) ? MAKE_TYPED_ID(VT_COUNTRY, 1) : ID_NOT_FOUND;

    fprintf(stderr, "=== WRITE QUERIES ===\n");

    // W1: insert a new person
    w1_insert_person(graph, person_count, opts.has_node_props, &col_times.w1);  // person_count used as counter only

    // W2: insert knows edge 0→1 (may already exist, but demonstrates the API)
    if (person_count >= 2) {
        w2_insert_knows(graph, sample_person0, sample_person1,
                        1700000001000LL, opts.has_edge_props, &col_times.w2);
    }

    // W3: insert a new post + hasCreator edge
    node_id_t next_post_id = MAKE_TYPED_ID(VT_POST, post_count);
    w3_insert_post_with_creator(graph, next_post_id, sample_person0,
                                1700000002000LL, 42, opts.has_node_props, &col_times.w3);

    fprintf(stderr, "\n=== SINGLE-TYPE READ QUERIES ===\n");

    // R1: person profile
    r1_person_profile(graph, sample_person0, &col_times.r1);

    // R2: friends sorted by date
    r2_friends_sorted_by_date(graph, sample_person0, opts.has_edge_props,
                               opts.prop_mode, &col_times.r2);

    // R3: BFS shortest path (may be slow for large graphs — demo only)
    if (person_count >= 2)
        r3_bfs_shortest_path(graph, sample_person0, sample_person1, &col_times.r3);

    fprintf(stderr, "\n=== CROSS-TYPE READ QUERIES ===\n");

    // X1: post profile
    if (sample_post0 != OutOfBand_ID_MAX)
        x1_post_profile(graph, sample_post0, &col_times.x1);

    // X2: post → author
    if (sample_post0 != OutOfBand_ID_MAX)
        x2_post_author(graph, sample_post0, &col_times.x2);

    // X3: IC-2 friends' recent posts
    {
        int64_t cutoff = INT64_MAX;  // all posts before "now"
        x3_ic2_friends_recent_posts(graph, sample_person0, cutoff, 10,
                                    opts.has_node_props, &col_times.x3);
    }

    // X4: insert a likes edge
    if (sample_post0 != OutOfBand_ID_MAX && sample_person1 != sample_person0) {
        x4_insert_likes(graph, sample_person1, sample_post0,
                        1700000003000LL, opts.has_edge_props, &col_times.x4);
    }

    // X5: count likes in date range
    if (sample_post0 != OutOfBand_ID_MAX) {
        x5_count_likes_in_range(graph, sample_post0,
                                0LL, INT64_MAX,
                                opts.has_edge_props, &col_times.x5);
    }

    fprintf(stderr, "\n=== AGGREGATE QUERIES ===\n");

    // A1: degree count
    a1_degree_count(graph, sample_person0, &col_times.a1);

    // A2: knows edges in date range for person 0
    a2_knows_in_date_range(graph, sample_person0,
                           0LL, INT64_MAX,
                           opts.has_edge_props, opts.prop_mode, &col_times.a2);

    // A3: posts liked in date range for person 1
    if (sample_person1 != sample_person0)
        a3_posts_liked_in_range(graph, sample_person1,
                                0LL, INT64_MAX,
                                opts.has_edge_props, opts.prop_mode, &col_times.a3);

    fprintf(stderr, "\n=== NEW QUERIES (C1: Comment, C2: Forum, C4: Place) ===\n");

    // IC-7: who liked P's messages
    ic7_message_likes(graph, sample_person0, 20, opts.has_edge_props, &col_times.ic7_message_likes);

    // IC-9: friends' posts and comments before cutoff
    ic9_friends_messages_before(graph, sample_person0, INT64_MAX, 20,
                                opts.has_node_props, &col_times.ic9_friends_messages);

    // IC-8: latest replies to P's messages (replyOf chain — CP-1)
    ic8_latest_replies(graph, sample_person0, 20,
                       opts.has_node_props, &col_times.ic8_latest_replies);

    // BI-2: message counts in two date windows (CP-4 colgroup scan demo)
    if (opts.prop_mode == COLUMNAR && !dry_run) {
        // Window 1: first half of 2012; Window 2: first half of 2013
        int64_t w1_lo = 1325376000000LL;  // 2012-01-01
        int64_t w1_hi = 1341100800000LL;  // 2012-07-01
        int64_t w2_lo = 1356998400000LL;  // 2013-01-01
        int64_t w2_hi = 1372636800000LL;  // 2013-07-01
        bi2_message_count_two_windows(graph, w1_lo, w1_hi, w2_lo, w2_hi,
                                      &col_times.bi2_two_windows);
    }

    // IC-5: forums where friends joined since epoch 0 (all time, CP-3 forum fan-out)
    ic5_forums_by_friend_membership(graph, sample_person0, 0LL, 20,
                                    opts.has_edge_props, &col_times.ic5_forums);

    // IC-3: FoF by country (CP-2 3-hop cross-type join)
    ic3_fof_by_country(graph, sample_person0, country_x, country_y, 10,
                       &col_times.ic3_fof_country);

    if (opts.prop_mode == COLUMNAR && !dry_run) {
        const int N_THREADS = omp_get_max_threads();
        fprintf(stderr, "\n=== BI QUERIES (COLUMNAR mode) ===\n");
        bi1_posting_summary(graph, post_count, &col_times.bi1);
        bi12_message_distribution(graph, post_count, &col_times.bi12);
        bi12_message_distribution_fast(graph, post_count, INT64_MAX, 0, &col_times.bi12_fast);

        // Build read-only opts for the parallel handles.
        // WiredTiger allows only one connection per DB, so we reuse the
        // existing connection via engine.get_connection() and construct graph
        // handles directly (new SplitEdgeKey / AdjList), bypassing GraphEngine.
        graph_opts ro_opts        = opts;
        ro_opts.read_only         = true;
        ro_opts.create_new        = false;
        ro_opts.checkpoint_name   = chkpt;
        WT_CONNECTION *conn       = engine.get_connection();

        fprintf(stderr, "\n=== BI QUERIES PARALLEL (%d threads) ===\n", N_THREADS);
        bi1_posting_summary_parallel(conn, ro_opts, post_count,
                                     N_THREADS, &col_times.bi1_par);
        bi12_message_distribution_fast_parallel(conn, ro_opts, post_count,
                                                N_THREADS, INT64_MAX, 0,
                                                &col_times.bi12_fast_par);
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

    // ---- CSV timing output (for comparison script) ----
    {
        const char *sep_line = "============================================================";
        std::string sys_name = std::string("flexograph_") + graph_type_str + "_"
                             + (opts.prop_mode == COLUMNAR ? "columnar" : "embedded");

        fprintf(stderr, "\n%s\n=== CSV (for comparison script) ===\n%s\n",
                sep_line, sep_line);
        fprintf(stderr, "system,query,ms\n");

        struct { const char *name; double ms; } all_times[] = {
            {"w1_insert_person",            col_times.w1},
            {"w2_insert_knows",             col_times.w2},
            {"w3_insert_post_hasCreator",   col_times.w3},
            {"r1_person_profile",           col_times.r1},
            {"r2_friends_sorted_by_date",   col_times.r2},
            {"r3_bfs_shortest_path",        col_times.r3},
            {"x1_post_profile",             col_times.x1},
            {"x2_post_author",              col_times.x2},
            {"x3_ic2_friends_recent_posts", col_times.x3},
            {"x4_insert_likes",             col_times.x4},
            {"x5_count_likes_in_range",     col_times.x5},
            {"a1_degree_count",             col_times.a1},
            {"a2_knows_in_date_range",      col_times.a2},
            {"a3_posts_liked_in_range",     col_times.a3},
            {"bi1_posting_summary",         col_times.bi1},
            {"bi12_message_distribution",   col_times.bi12},
            {"bi12_message_distribution_fast", col_times.bi12_fast},
            {"bi1_posting_summary_parallel",            col_times.bi1_par},
            {"bi12_message_distribution_fast_parallel", col_times.bi12_fast_par},
            {"ic7_message_likes",            col_times.ic7_message_likes},
            {"ic9_friends_messages_before",  col_times.ic9_friends_messages},
            {"ic8_latest_replies",           col_times.ic8_latest_replies},
            {"bi2_message_count_two_windows",col_times.bi2_two_windows},
            {"ic5_forums_by_friend_membership",col_times.ic5_forums},
            {"ic3_fof_by_country",           col_times.ic3_fof_country},
        };
        for (auto &t : all_times) {
            if (t.ms >= 0.0)
                fprintf(stderr, "%s,%s,%.3f\n", sys_name.c_str(), t.name, t.ms);
        }
    }

    graph_ptr->close(false);
    engine.close_graph();
    return 0;
}
