// LDBC SNB benchmark queries on Flexograph
//
// Usage: ./ldbc_snb_queries <data_dir>
//
// Expects LDBC SNB CSV files (pipe-delimited) under <data_dir>/dynamic/:
//   person_0_0.csv
//   post_0_0.csv
//   person_knows_person_0_0.csv
//   post_hasCreator_person_0_0.csv
//   person_likes_post_0_0.csv
//
// Implements 14 queries across 4 categories:
//   Writes          (3): insert Person, insert knows, insert Post+hasCreator
//   Single-type (3): person profile, friends list by date, BFS shortest path
//   Cross-type  (5): post profile, post→author, IC-2 friends' posts,
//                    insert likes, count likes in date range
//   Aggregates  (3): degree count, knows edges in date range, posts liked in range

#include <cassert>
#include <cstdio>
#include <cstring>
#include <deque>
#include <iostream>
#include <limits>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

#include "common_defs.h"
#include "edgekey_split.h"
#include "graph_engine.h"
#include "ldbc_snb_loader.h"
#include "prop_schema.h"

// ============================================================
// ID-space helpers
// ============================================================

static inline bool is_person(node_id_t id, node_id_t person_count)
{
    return id < person_count;
}

static inline bool is_post(node_id_t id, node_id_t person_count,
                           node_id_t post_count)
{
    return id >= person_count && id < person_count + post_count;
}

// ============================================================
// Property read helpers (decode blobs by ID range)
// ============================================================

struct PersonProps {
    int64_t creation_date = 0;
    int64_t birthday = 0;
    int8_t  gender = 0;
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
static void w1_insert_person(GraphBase &graph, node_id_t &person_count,
                             bool has_props)
{
    SEP();
    node_id_t new_id = person_count;  // append at end of person range
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

    person_count++;
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
    fprintf(stderr, "  Person %llu: creationDate=%lld birthday=%lld gender=%d\n",
            (unsigned long long)pid,
            (long long)p.creation_date,
            (long long)p.birthday,
            (int)p.gender);
    return p;
}

// R2: Friends list — get all out-neighbors of a person, sorted by knows.creationDate
static std::vector<std::pair<node_id_t, int64_t>>
r2_friends_sorted_by_date(GraphBase &graph, node_id_t pid, bool has_props)
{
    SEP();
    TIME_START(r2_friends_sorted_by_date)

    std::vector<node_id_t> neighbors = graph.get_out_nodes_id(pid);
    std::vector<std::pair<node_id_t, int64_t>> result;
    result.reserve(neighbors.size());

    for (node_id_t nb : neighbors) {
        int64_t cd = 0;
        if (has_props) {
            prop_blob pb = graph.get_edge_properties(pid, nb);
            cd = decode_knows(pb).creation_date;
        }
        result.emplace_back(nb, cd);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) { return a.second > b.second; });

    TIME_END(r2_friends_sorted_by_date)
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
                                node_id_t person_count)
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
            if (!is_person(v, person_count)) continue;
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
static node_id_t x2_post_author(GraphBase &graph, node_id_t post_id,
                                 node_id_t person_count)
{
    SEP();
    TIME_START(x2_post_author)

    std::vector<node_id_t> out = graph.get_out_nodes_id(post_id);
    node_id_t author = OutOfBand_ID_MAX;
    for (node_id_t nb : out) {
        if (is_person(nb, person_count)) {
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
                             node_id_t person_count, node_id_t post_count,
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
        if (!is_person(friend_id, person_count)) continue;

        std::vector<node_id_t> creators_in = graph.get_in_nodes_id(friend_id);
        for (node_id_t post_id : creators_in) {
            if (!is_post(post_id, person_count, post_count)) continue;

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
                                        node_id_t person_count, bool has_props)
{
    SEP();
    TIME_START(x5_count_likes_in_range)

    // Persons who liked this post are in-edges of the post from the person range
    std::vector<node_id_t> likers = graph.get_in_nodes_id(post_id);
    int64_t count = 0;
    for (node_id_t liker : likers) {
        if (!is_person(liker, person_count)) continue;
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
static int64_t a2_knows_in_date_range(GraphBase &graph, node_id_t pid,
                                       int64_t lo_ms, int64_t hi_ms,
                                       node_id_t person_count, bool has_props)
{
    SEP();
    TIME_START(a2_knows_in_date_range)

    std::vector<node_id_t> friends = graph.get_out_nodes_id(pid);
    int64_t count = 0;
    for (node_id_t nb : friends) {
        if (!is_person(nb, person_count)) continue;
        if (!has_props) { count++; continue; }
        prop_blob pb = graph.get_edge_properties(pid, nb);
        int64_t cd = decode_knows(pb).creation_date;
        if (cd >= lo_ms && cd <= hi_ms)
            count++;
    }

    TIME_END(a2_knows_in_date_range)
    fprintf(stderr, "  Person %llu knows edges in date range: %lld\n",
            (unsigned long long)pid, (long long)count);
    return count;
}

// A3: Count posts liked by a person within a date range
static int64_t a3_posts_liked_in_range(GraphBase &graph, node_id_t pid,
                                        int64_t lo_ms, int64_t hi_ms,
                                        node_id_t person_count, bool has_props)
{
    SEP();
    TIME_START(a3_posts_liked_in_range)

    std::vector<node_id_t> liked = graph.get_out_nodes_id(pid);
    int64_t count = 0;
    for (node_id_t post_id : liked) {
        if (is_person(post_id, person_count)) continue;  // skip knows edges
        if (!has_props) { count++; continue; }
        prop_blob pb = graph.get_edge_properties(pid, post_id);
        int64_t cd = decode_likes(pb).creation_date;
        if (cd >= lo_ms && cd <= hi_ms)
            count++;
    }

    TIME_END(a3_posts_liked_in_range)
    fprintf(stderr, "  Person %llu posts-liked in date range: %lld\n",
            (unsigned long long)pid, (long long)count);
    return count;
}

// ============================================================
// main
// ============================================================

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <data_dir>\n", argv[0]);
        fprintf(stderr, "  data_dir should contain LDBC SNB dynamic/ CSV files\n");
        return 1;
    }

    std::string data_dir = argv[1];
    // Normalize path: remove trailing slash
    while (!data_dir.empty() && data_dir.back() == '/')
        data_dir.pop_back();

    std::string dyn = data_dir + "/dynamic";

    // ---- Graph engine setup ----
    graph_opts opts;
    opts.create_new      = true;
    opts.optimize_create = false;
    opts.is_directed     = true;
    opts.read_optimize   = true;
    opts.is_weighted     = false;
    opts.has_node_props  = true;
    opts.has_edge_props  = true;
    opts.type            = GraphType::SplitEKey;
    opts.db_name         = "ldbc_snb_queries";
    opts.db_dir          = "./db";
    opts.conn_config     = "cache_size=2GB";
    opts.stat_log        = "./";

    GraphEngine engine(1, opts);
    WT_CONNECTION *conn = engine.get_connection();
    SplitEdgeKey graph(opts, conn);

    // ---- Load data ----
    fprintf(stderr, "=== Loading LDBC SNB data from %s ===\n", data_dir.c_str());

    LDBCLoader loader(&graph, opts);

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
    const node_id_t post_count   = loader.post_count;
    const node_id_t post_base    = person_count;  // first post ID

    fprintf(stderr, "\n=== Graph loaded: %llu persons, %llu posts ===\n\n",
            (unsigned long long)person_count,
            (unsigned long long)post_count);

    // ---- Pick sample IDs for queries ----
    // Use person 0 and person 1 as query subjects (always exist if SF > 0)
    node_id_t sample_person0 = 0;
    node_id_t sample_person1 = (person_count > 1) ? 1 : 0;
    node_id_t sample_post0   = (post_count > 0) ? post_base : OutOfBand_ID_MAX;

    fprintf(stderr, "=== WRITE QUERIES ===\n");

    // W1: insert a new person
    w1_insert_person(graph, person_count, opts.has_node_props);

    // W2: insert knows edge 0→1 (may already exist, but demonstrates the API)
    if (person_count >= 2) {
        w2_insert_knows(graph, sample_person0, sample_person1,
                        1700000001000LL, opts.has_edge_props);
    }

    // W3: insert a new post + hasCreator edge
    node_id_t next_post_id = post_base + post_count;
    w3_insert_post_with_creator(graph, next_post_id, sample_person0,
                                1700000002000LL, 42, opts.has_node_props);

    fprintf(stderr, "\n=== SINGLE-TYPE READ QUERIES ===\n");

    // R1: person profile
    r1_person_profile(graph, sample_person0);

    // R2: friends sorted by date
    r2_friends_sorted_by_date(graph, sample_person0, opts.has_edge_props);

    // R3: BFS shortest path (may be slow for large graphs — demo only)
    if (person_count >= 2)
        r3_bfs_shortest_path(graph, sample_person0, sample_person1, person_count);

    fprintf(stderr, "\n=== CROSS-TYPE READ QUERIES ===\n");

    // X1: post profile
    if (sample_post0 != OutOfBand_ID_MAX)
        x1_post_profile(graph, sample_post0);

    // X2: post → author
    if (sample_post0 != OutOfBand_ID_MAX)
        x2_post_author(graph, sample_post0, person_count);

    // X3: IC-2 friends' recent posts
    {
        int64_t cutoff = INT64_MAX;  // all posts before "now"
        x3_ic2_friends_recent_posts(graph, sample_person0, cutoff, 10,
                                    person_count, post_count,
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
                                person_count, opts.has_edge_props);
    }

    fprintf(stderr, "\n=== AGGREGATE QUERIES ===\n");

    // A1: degree count
    a1_degree_count(graph, sample_person0);

    // A2: knows edges in date range for person 0
    a2_knows_in_date_range(graph, sample_person0,
                           0LL, INT64_MAX,
                           person_count, opts.has_edge_props);

    // A3: posts liked in date range for person 1
    if (sample_person1 != sample_person0)
        a3_posts_liked_in_range(graph, sample_person1,
                                0LL, INT64_MAX,
                                person_count, opts.has_edge_props);

    fprintf(stderr, "\n=== All queries complete ===\n");

    graph.close(false);
    engine.close_graph();
    return 0;
}
