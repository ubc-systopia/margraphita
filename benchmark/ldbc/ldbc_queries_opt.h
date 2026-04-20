// ldbc_queries_opt.h — Section 9 performance-optimised query variants
//
// This file implements the four optimisations documented in
// impl_docs/spec_query_compatibility.md Section 9.  Each function is a
// drop-in replacement for the corresponding tq_* function in ldbc_queries.h,
// differing only in the hot-path implementation.  The original functions are
// left untouched so both can be timed side-by-side.
//
// Include this file AFTER ldbc_queries.h (depends on its macros and helpers).
//
// Optimisation summary
// ────────────────────
//  §9.1  tq_r3_bfs_shortest_path_bidir
//        Bidirectional BFS with two dense int16_t distance arrays.
//        Visits O(k^(d/2)) nodes vs O(k^d) for unidirectional BFS.
//        No schema change.  No DB reload.
//
//  §9.2  tq_ic3_fof_by_country_citycache
//        Per-query city→country cache eliminates repeated WiredTiger lookups
//        for the second hop when multiple fof candidates share the same city.
//        No schema change.  No DB reload.
//        (The full §9.2 optimisation — storing country_id as a CG_LOCATION
//        colgroup in SNBPersonSchema — additionally requires a schema change
//        and DB rebuild; see impl_docs for that plan.)
//
//  §9.3  tq_x3_ic2_friends_recent_posts_flat_scan
//        Replaces n_friends separate get_in_nodes_id() calls with a single
//        forward flat scan of the postHasCreator edge range.  Eliminates
//        reverse-index seeks that are expensive in SplitEdgeKey.
//        Trade-off: may regress AdjList (where per-friend seeks are cheap).
//        No schema change.  No DB reload.
//
//  §9.4  tq_bi12_message_distribution_fast_dense
//        Replaces std::unordered_map<node_id_t,int64_t> creator_count with a
//        dense std::vector<int32_t>[n_person] indexed by person counter.
//        Eliminates hash overhead across ~10M edge iterations on SF3.
//        NOTE: The typed-ID early-exit per post (skip tag/country edges) is
//        described in §9.4 but requires a seek() method on EdgeCursor that
//        does not currently exist.  Only the dense-array part is implemented.
//        No schema change.  No DB reload.

#pragma once
#include <climits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// ─── §9.1  r3 — bidirectional BFS ───────────────────────────────────────────
//
// Dense int16_t distance arrays (indexed by VCOUNTER_OF — for VT_PERSON=0 this
// equals the raw ID) replace the unordered_map<node_id_t,int> used in the
// original.  Both frontiers use get_out_nodes_id(): since `knows` is stored
// bidirectionally (both p→q and q→p), out-edges from dst correctly reach src's
// neighbourhood without needing get_in_nodes_id().
//
// Stopping condition: expand until a meeting node is found (best < INT_MAX).
// This is safe for unweighted undirected graphs (knows is undirected).
static void tq_r3_bfs_shortest_path_bidir(GraphBase &g,
                                           node_id_t src, node_id_t dst,
                                           size_t n_person,
                                           double *out_ms = nullptr)
{
    TQ_START(r3_bfs_shortest_path_bidir)
    if (src != dst) {
        std::vector<int16_t> d_fwd(n_person, -1);
        std::vector<int16_t> d_bwd(n_person, -1);
        d_fwd[VCOUNTER_OF(src)] = 0;
        d_bwd[VCOUNTER_OF(dst)] = 0;

        std::vector<node_id_t> front_fwd = {src};
        std::vector<node_id_t> front_bwd = {dst};
        std::vector<node_id_t> next;
        int best = INT_MAX;

        // Expand one complete level of `frontier`, updating d_cur.
        // If any newly-reached node is already in d_other, record meeting.
        auto expand = [&](std::vector<node_id_t> &frontier,
                          std::vector<int16_t>   &d_cur,
                          std::vector<int16_t>   &d_other) {
            next.clear();
            for (node_id_t u : frontier) {
                int16_t du = d_cur[VCOUNTER_OF(u)];
                for (node_id_t v : g.get_out_nodes_id(u)) {
                    if (!is_person(v)) continue;
                    uint64_t vi = VCOUNTER_OF(v);
                    if (vi >= n_person || d_cur[vi] >= 0) continue;
                    d_cur[vi] = du + 1;
                    if (d_other[vi] >= 0) {
                        int cand = (int)d_cur[vi] + (int)d_other[vi];
                        if (cand < best) best = cand;
                    }
                    next.push_back(v);
                }
            }
            frontier.swap(next);
        };

        while (best == INT_MAX && (!front_fwd.empty() || !front_bwd.empty())) {
            // Always expand the smaller frontier to keep both sides balanced.
            if (front_bwd.empty() ||
                (!front_fwd.empty() && front_fwd.size() <= front_bwd.size()))
                expand(front_fwd, d_fwd, d_bwd);
            else
                expand(front_bwd, d_bwd, d_fwd);
        }
        (void)best;
    }
    if (out_ms) { TQ_END_CAP(r3_bfs_shortest_path_bidir, *out_ms) }
    else        { TQ_END(r3_bfs_shortest_path_bidir) }
}

// ─── §9.2  ic3 — per-query city→country cache ───────────────────────────────
//
// resolve_person_country_graph() issues two WiredTiger cursor opens per fof
// candidate (person→city, city→country).  Within a single query execution,
// the social neighbourhood is geographically clustered, so many candidates
// share the same city.  A per-query unordered_map<city_id, country_id> amortises
// the second hop across the candidate list — the city→country lookup is paid
// once per unique city rather than once per candidate.
//
// The first hop (person→city via get_out_nodes_id) is still performed per
// candidate through WiredTiger.
static void tq_ic3_fof_by_country_citycache(GraphBase &g, node_id_t pid,
                                             node_id_t country_x, node_id_t country_y,
                                             double *out_ms = nullptr)
{
    TQ_START(ic3_fof_by_country_citycache)
    // 1-hop expansion: direct friends of pid (forward knows only, matches NeuG Cypher)
    std::unordered_set<node_id_t> friends_set;
    for (node_id_t f : g.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends_set.insert(f);

    // 2-hop expansion: fof candidates and common-friend counts
    std::unordered_map<node_id_t, int32_t> fof_common;
    for (node_id_t f : friends_set)
        for (node_id_t ff : g.get_out_nodes_id(f)) {
            if (VTYPE_OF(ff) != VT_PERSON) continue;
            if (ff == pid || friends_set.count(ff)) continue;
            fof_common[ff]++;
        }

    // Country filter with per-query city→country cache.
    // Key insight: social networks cluster geographically, so a high fraction
    // of candidates in the same fof neighbourhood share a city.
    std::unordered_map<node_id_t, node_id_t> city_country_cache;
    std::vector<std::pair<node_id_t, int32_t>> result;

    for (auto &[candidate, common] : fof_common) {
        // First hop: person → city (or directly to country)  — always via WiredTiger
        node_id_t country = ID_NOT_FOUND;
        for (node_id_t nb : g.get_out_nodes_id(candidate)) {
            if (VTYPE_OF(nb) == VT_COUNTRY) { country = nb; break; }
            if (VTYPE_OF(nb) != VT_CITY) continue;
            // Second hop: city → country — use cache to avoid repeated lookups
            auto it = city_country_cache.find(nb);
            if (it != city_country_cache.end()) {
                country = it->second;
            } else {
                for (node_id_t c : g.get_out_nodes_id(nb))
                    if (VTYPE_OF(c) == VT_COUNTRY) { country = c; break; }
                city_country_cache[nb] = country;  // cache even ID_NOT_FOUND
            }
            break;
        }
        if (country == ID_NOT_FOUND) continue;
        if (country_x != ID_NOT_FOUND && country != country_x) continue;
        if (country_y != ID_NOT_FOUND && country == country_y) continue;
        result.emplace_back(candidate, common);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(ic3_fof_by_country_citycache, *out_ms) }
    else        { TQ_END(ic3_fof_by_country_citycache) }
}

// ─── §9.2b ic3 — CG_LOCATION colgroup (single seek per candidate) ────────
//
// Full CG_LOCATION optimisation: country_id is stored directly in person_props
// as a colgroup column, so resolving a candidate's country is a single colgroup
// cursor seek instead of a two-hop WiredTiger traversal (person→city→country).
// Requires DB rebuilt with CG_LOCATION schema.
static void tq_ic3_fof_by_country_colgroup(GraphBase &g, node_id_t pid,
                                            node_id_t country_x, node_id_t country_y,
                                            double *out_ms = nullptr)
{
    TQ_START(ic3_fof_by_country_colgroup)
    // 1-hop expansion: direct friends of pid (forward knows only)
    std::unordered_set<node_id_t> friends_set;
    for (node_id_t f : g.get_out_nodes_id(pid))
        if (VTYPE_OF(f) == VT_PERSON) friends_set.insert(f);

    // 2-hop expansion: fof candidates and common-friend counts
    std::unordered_map<node_id_t, int32_t> fof_common;
    for (node_id_t f : friends_set)
        for (node_id_t ff : g.get_out_nodes_id(f)) {
            if (VTYPE_OF(ff) != VT_PERSON) continue;
            if (ff == pid || friends_set.count(ff)) continue;
            fof_common[ff]++;
        }

    // Country filter via CG_LOCATION colgroup — one seek per candidate
    auto loc_cur = g.get_node_prop_cursor(PERSON_PROPS_TABLE, CG_LOCATION);
    std::vector<std::pair<node_id_t, int32_t>> result;

    for (auto &[candidate, common] : fof_common) {
        if (!loc_cur->seek(candidate)) continue;
        node_id_t country = (node_id_t)loc_cur->get_uint64(0);
        if (country == 0) continue;  // no country_id stored
        if (country_x != ID_NOT_FOUND && country != country_x) continue;
        if (country_y != ID_NOT_FOUND && country == country_y) continue;
        result.emplace_back(candidate, common);
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(ic3_fof_by_country_colgroup, *out_ms) }
    else        { TQ_END(ic3_fof_by_country_colgroup) }
}

// ─── §9.3  x3 — flat postHasCreator scan for SplitEdgeKey ──────────────────
//
// Original: for each friend f, get_in_nodes_id(f) to find f's posts.
//   Cost: n_friends reverse-index seeks (expensive in SplitEdgeKey).
//
// This variant: one forward flat scan of the postHasCreator edge range,
// filtered by a friends_set hash lookup per edge.
//   Cost: ~total_post_edges next() calls — O(10M) on SF3 — but no seeks.
//
// Trade-off: AdjList per-friend in-edge lookup is fast (~O(degree) next() per
// friend), so this variant is expected to be SLOWER for adj and FASTER for
// ekey.  Use the benchmark to confirm which storage type benefits.
static void tq_x3_ic2_friends_recent_posts_flat_scan(GraphBase &g, node_id_t pid,
                                                      int64_t cutoff_ms, bool has_props,
                                                      double *out_ms = nullptr)
{
    TQ_START(x3_ic2_friends_recent_posts_flat_scan)
    // Phase 1: build friends set
    std::unordered_set<node_id_t> friends_set;
    for (node_id_t f : g.get_out_nodes_id(pid))
        if (is_person(f)) friends_set.insert(f);

    // Phase 2: single forward scan of all post out-edges
    // Find posts whose creator (hasCreator → Person dst) is in friends_set.
    std::vector<node_id_t> candidate_posts;
    {
        EdgeCursor *ec = g.get_edge_iter();
        ec->set_key_range({{MAKE_TYPED_ID(VT_POST, 0), 1},
                           {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
        edge found;
        ec->next(&found);
        while (found.src_id != OutOfBand_ID_MAX) {
            if (VTYPE_OF(found.src_id) != VT_POST) break;
            if (is_person(found.dst_id) && friends_set.count(found.dst_id))
                candidate_posts.push_back(found.src_id);
            ec->next(&found);
        }
        delete ec;
    }

    // Phase 3: temporal filter — identical to original tq_x3
    std::sort(candidate_posts.begin(), candidate_posts.end());
    candidate_posts.erase(std::unique(candidate_posts.begin(), candidate_posts.end()),
                          candidate_posts.end());
    std::vector<std::pair<node_id_t, int64_t>> result;
    if (has_props && !candidate_posts.empty()) {
        auto cg = g.get_node_prop_cursor(POST_PROPS_TABLE, CG_TEMPORAL);
        for (node_id_t p2 : candidate_posts) {
            if (!cg->seek(p2)) continue;
            uint64_t cDate = cg->get_uint64(0);
            if ((int64_t)cDate < cutoff_ms) result.emplace_back(p2, (int64_t)cDate);
        }
    } else {
        for (node_id_t p2 : candidate_posts) {
            prop_blob pb = g.get_node_properties(p2);
            DecodedPost dp = decode_post_blob(pb);
            if (dp.valid && dp.creationDate < cutoff_ms)
                result.emplace_back(p2, dp.creationDate);
        }
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b){ return a.second > b.second; });
    if (result.size() > 10) result.resize(10);
    if (out_ms) { TQ_END_CAP(x3_ic2_friends_recent_posts_flat_scan, *out_ms) }
    else        { TQ_END(x3_ic2_friends_recent_posts_flat_scan) }
}

// ─── §9.4  bi12 — dense creator_count array ─────────────────────────────────
//
// Original uses unordered_map<node_id_t, int64_t> creator_count.
// With n_person~35k on SF3 and ~10M post edges to process, the map has a high
// load factor producing many hash collisions and cache misses per increment.
//
// This variant uses a dense vector<int32_t>[n_person] indexed by person counter
// (VCOUNTER_OF).  Since VT_PERSON=0, person IDs equal their counters directly.
// Each increment is a single array write with no hashing.
//
// The typed-ID early-exit optimisation (seek past tag/country edges per post,
// §9.4) is NOT implemented here because EdgeCursor has no seek() method.
// Adding that optimisation would require extending the EdgeCursor interface.
static void tq_bi12_message_distribution_fast_dense(GraphBase &g,
                                                     node_id_t total_posts,
                                                     size_t n_person,
                                                     double *out_ms = nullptr)
{
    TQ_START(bi12_message_distribution_fast_dense)
    (void)total_posts;
    std::vector<int32_t> creator_count(n_person, 0);

    EdgeCursor *ec = g.get_edge_iter();
    ec->set_key_range({{MAKE_TYPED_ID(VT_POST, 0), 1},
                       {OutOfBand_ID_MAX, OutOfBand_ID_MAX}});
    edge found;
    ec->next(&found);
    while (found.src_id != OutOfBand_ID_MAX) {
        if (VTYPE_OF(found.src_id) != VT_POST) break;
        if (is_person(found.dst_id)) {
            uint64_t ci = VCOUNTER_OF(found.dst_id);
            if (ci < n_person) creator_count[ci]++;
        }
        ec->next(&found);
    }
    delete ec;
    (void)creator_count;
    if (out_ms) { TQ_END_CAP(bi12_message_distribution_fast_dense, *out_ms) }
    else        { TQ_END(bi12_message_distribution_fast_dense) }
}
