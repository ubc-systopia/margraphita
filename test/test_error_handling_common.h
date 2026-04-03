// test_error_handling_common.h
//
// Shared single-threaded error-handling and driver-compliance test logic.
// Included by test_error_handling_adj.cpp and test_error_handling_ekey.cpp.
//
// Each test function takes (GraphBase &g) so it is backend-agnostic.
// Directed-graph tests live in run_directed_tests(); undirected-graph tests
// need a fresh graph with is_directed=false and live in run_undirected_tests().
//
// Node-ID ranges per test group (no overlap so a shared DB stays clean):
//   T1.x  1001–1909
//   T2.x  2001–2303
//   T3.x  3001–3304   (AdjList-specific tests also call these)
//   Driver sim  4001–4202

#pragma once

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <set>
#include <vector>

#include "common_defs.h"
#include "graph.h"

#define PASS() fprintf(stderr, "%s: PASSED\n", __FUNCTION__)
#define INFO() fprintf(stderr, "--- %s\n", __FUNCTION__)

// ---------------------------------------------------------------------------
// Low-level helpers that return the raw WT error code
// ---------------------------------------------------------------------------

static int insert_node_ret(GraphBase &g, node_id_t id)
{
    node n{}; n.id = id;
    return g.add_node(n, false);
}

static int insert_edge_ret(GraphBase &g, node_id_t src, node_id_t dst)
{
    edge e{}; e.src_id = src; e.dst_id = dst;
    return g.add_edge(e, false);
}

// Insert and assert success (mirrors the existing test helpers but keeps
// the name distinct to avoid shadowing the helper in the including .cpp).
static void must_insert_node(GraphBase &g, node_id_t id)
{
    int ret = insert_node_ret(g, id);
    assert(ret == 0 || ret == WT_DUPLICATE_KEY);
}


static bool vec_contains(const std::vector<node_id_t> &v, node_id_t id)
{
    return std::find(v.begin(), v.end(), id) != v.end();
}

// ---------------------------------------------------------------------------
// T1.1 — new vertex: add_node returns 0
// ---------------------------------------------------------------------------
static void test_add_vertex_new(GraphBase &g)
{
    INFO();
    int ret = insert_node_ret(g, 1001);
    assert(ret == 0);
    assert(g.has_node(1001));
    PASS();
}

// ---------------------------------------------------------------------------
// T1.2 — duplicate vertex: add_node returns WT_DUPLICATE_KEY (no throw,
//         no infinite loop)
// ---------------------------------------------------------------------------
static void test_add_vertex_duplicate(GraphBase &g)
{
    INFO();
    int ret1 = insert_node_ret(g, 1002);
    assert(ret1 == 0);
    int ret2 = insert_node_ret(g, 1002);
    assert(ret2 == WT_DUPLICATE_KEY);
    PASS();
}

// ---------------------------------------------------------------------------
// T1.3 — count consistency: inserting N distinct nodes increments by N;
//         inserting a duplicate does not increment further
// ---------------------------------------------------------------------------
static void test_add_vertex_count(GraphBase &g)
{
    INFO();
    uint64_t before = g.get_num_nodes();
    for (node_id_t id = 1100; id < 1120; id++)
    {
        int ret = insert_node_ret(g, id);
        assert(ret == 0);
    }
    assert(g.get_num_nodes() == before + 20);

    // Duplicate must not increment the counter
    insert_node_ret(g, 1100);
    assert(g.get_num_nodes() == before + 20);
    PASS();
}

// ---------------------------------------------------------------------------
// T1.4 — directed edge: correct out/in adjacency; reverse not present
// ---------------------------------------------------------------------------
static void test_add_edge_directed(GraphBase &g)
{
    INFO();
    must_insert_node(g, 1200);
    must_insert_node(g, 1201);
    int ret = insert_edge_ret(g, 1200, 1201);
    assert(ret == 0);

    auto out_src = g.get_out_nodes_id(1200);
    assert(vec_contains(out_src, 1201));

    auto in_dst = g.get_in_nodes_id(1201);
    assert(vec_contains(in_dst, 1200));

    // 1201 has no outgoing edge to 1200
    auto out_dst = g.get_out_nodes_id(1201);
    assert(!vec_contains(out_dst, 1200));
    PASS();
}

// ---------------------------------------------------------------------------
// T1.5 — add_edge implicitly creates src and dst nodes
// ---------------------------------------------------------------------------
static void test_add_edge_implicit_nodes(GraphBase &g)
{
    INFO();
    // Do NOT pre-insert nodes 1300, 1301
    int ret = insert_edge_ret(g, 1300, 1301);
    assert(ret == 0);
    assert(g.has_node(1300));
    assert(g.has_node(1301));
    PASS();
}

// ---------------------------------------------------------------------------
// T1.6 — duplicate edge: returns WT_DUPLICATE_KEY; edge count not incremented
// ---------------------------------------------------------------------------
static void test_add_edge_duplicate(GraphBase &g)
{
    INFO();
    must_insert_node(g, 1400);
    must_insert_node(g, 1401);

    int ret1 = insert_edge_ret(g, 1400, 1401);
    assert(ret1 == 0);
    uint64_t edge_count = g.get_num_edges();

    int ret2 = insert_edge_ret(g, 1400, 1401);
    assert(ret2 == WT_DUPLICATE_KEY);
    assert(g.get_num_edges() == edge_count);  // count must not change
    PASS();
}

// ---------------------------------------------------------------------------
// T1.9 — node count stays stable when all edge endpoints are pre-existing
// ---------------------------------------------------------------------------
static void test_add_edge_node_count_stable(GraphBase &g)
{
    INFO();
    for (node_id_t id = 1900; id < 1905; id++)
        must_insert_node(g, id);

    uint64_t node_count = g.get_num_nodes();

    insert_edge_ret(g, 1900, 1901);
    insert_edge_ret(g, 1901, 1902);
    insert_edge_ret(g, 1902, 1903);

    assert(g.get_num_nodes() == node_count);
    PASS();
}

// ---------------------------------------------------------------------------
// T2.2 — edge count invariant after sequence with a duplicate in the middle
// ---------------------------------------------------------------------------
static void test_count_after_duplicate_edge(GraphBase &g)
{
    INFO();
    uint64_t e_before = g.get_num_edges();

    int r1 = insert_edge_ret(g, 2200, 2201);
    assert(r1 == 0);
    assert(g.get_num_edges() == e_before + 1);

    int r2 = insert_edge_ret(g, 2200, 2201);  // duplicate
    assert(r2 == WT_DUPLICATE_KEY);
    assert(g.get_num_edges() == e_before + 1);  // unchanged

    int r3 = insert_edge_ret(g, 2200, 2202);  // new
    assert(r3 == 0);
    assert(g.get_num_edges() == e_before + 2);
    PASS();
}

// ---------------------------------------------------------------------------
// T2.3 — adjacency list integrity after a duplicate edge (rollback must not
//         corrupt the next successful insert)
// ---------------------------------------------------------------------------
static void test_adjlist_integrity_after_rollback(GraphBase &g)
{
    INFO();
    must_insert_node(g, 2300);

    insert_edge_ret(g, 2300, 2301);
    insert_edge_ret(g, 2300, 2302);
    insert_edge_ret(g, 2300, 2301);  // duplicate — must roll back cleanly
    int ret = insert_edge_ret(g, 2300, 2303);  // must succeed after rollback
    assert(ret == 0);

    auto out = g.get_out_nodes_id(2300);
    std::set<node_id_t> out_set(out.begin(), out.end());
    assert(out_set.count(2301) == 1);
    assert(out_set.count(2302) == 1);
    assert(out_set.count(2303) == 1);
    assert(out.size() == 3);
    PASS();
}

// ---------------------------------------------------------------------------
// T3.1 — first edge to a previously-unseen destination node:
//         the destination's out-adjacency must be queryable (empty, no crash).
//         This directly catches the throw-on-not-found regression in AdjList.
// ---------------------------------------------------------------------------
static void test_new_dst_adjlist_queryable(GraphBase &g)
{
    INFO();
    // 3100 → 3101: node 3101 has never appeared before
    int ret = insert_edge_ret(g, 3100, 3101);
    assert(ret == 0);

    // 3101 has no out-edges — must return empty list without throwing
    auto out = g.get_out_nodes_id(3101);
    assert(!vec_contains(out, (node_id_t)3100));

    // A subsequent edge from 3101 must also succeed
    int ret2 = insert_edge_ret(g, 3101, 3102);
    assert(ret2 == 0);
    auto out2 = g.get_out_nodes_id(3101);
    assert(vec_contains(out2, (node_id_t)3102));
    PASS();
}

// ---------------------------------------------------------------------------
// T3.2 — node with multiple in-edges: all are present in the in-adjacency list
// ---------------------------------------------------------------------------
static void test_multiple_in_edges(GraphBase &g)
{
    INFO();
    must_insert_node(g, 3200);
    insert_edge_ret(g, 3201, 3200);
    insert_edge_ret(g, 3202, 3200);
    insert_edge_ret(g, 3203, 3200);

    auto in = g.get_in_nodes_id(3200);
    std::set<node_id_t> in_set(in.begin(), in.end());
    assert(in_set.count(3201) == 1);
    assert(in_set.count(3202) == 1);
    assert(in_set.count(3203) == 1);
    PASS();
}

// ---------------------------------------------------------------------------
// T (driver sim 1) — add_vertex retry loop: returns true on new vertex,
//   false on duplicate, never spins on WT_DUPLICATE_KEY
// ---------------------------------------------------------------------------
static void test_driver_add_vertex(GraphBase &g)
{
    INFO();

    auto driver_add_vertex = [&](node_id_t id) -> bool
    {
        node n{}; n.id = id;
        int attempts = 0;
        for (;;)
        {
            int ret = g.add_node(n, false);
            if (ret == 0)          return true;
            if (ret == WT_DUPLICATE_KEY) return false;
            // WT_ROLLBACK or other transient: retry
            assert(++attempts < 1000);  // guard against infinite loop
        }
    };

    assert(driver_add_vertex(4001) == true);
    assert(driver_add_vertex(4001) == false);  // duplicate
    assert(driver_add_vertex(4002) == true);
    assert(g.has_node(4001));
    assert(g.has_node(4002));
    PASS();
}

// ---------------------------------------------------------------------------
// T (driver sim 2) — add_edge_v2 retry loop: returns true on success,
//   false on duplicate, never spins on WT_DUPLICATE_KEY
// ---------------------------------------------------------------------------
static void test_driver_add_edge_v2(GraphBase &g)
{
    INFO();

    auto driver_add_edge_v2 = [&](node_id_t src, node_id_t dst) -> bool
    {
        edge e{}; e.src_id = src; e.dst_id = dst;
        int attempts = 0;
        for (;;)
        {
            int ret = g.add_edge(e, false);
            if (ret == 0)                return true;
            if (ret == WT_DUPLICATE_KEY) return false;
            assert(++attempts < 1000);
        }
    };

    assert(driver_add_edge_v2(4100, 4101) == true);
    assert(driver_add_edge_v2(4100, 4101) == false);  // duplicate
    assert(driver_add_edge_v2(4100, 4102) == true);

    auto out = g.get_out_nodes_id(4100);
    assert(vec_contains(out, (node_id_t)4101));
    assert(vec_contains(out, (node_id_t)4102));
    assert(out.size() == 2);
    PASS();
}

// ---------------------------------------------------------------------------
// T (driver sim 3) — add_edge (NOT v2): the raw driver implementation assigns
//   the int return to bool, giving done=false on success (infinite loop bug).
//   This test verifies the *correct* behaviour that should be used instead,
//   i.e. checking ret == 0 explicitly.  It also serves as a regression guard:
//   if add_edge ever changes to return non-zero on success, this fails.
// ---------------------------------------------------------------------------
static void test_add_edge_returns_zero_on_success(GraphBase &g)
{
    INFO();
    must_insert_node(g, 4200);
    must_insert_node(g, 4201);
    int ret = insert_edge_ret(g, 4200, 4201);
    // The driver's add_edge does: done = tl_graph->add_edge(...)
    // With int return, done = 0 = false → loop never exits.
    // Correct check is ret == 0.
    assert(ret == 0);  // confirms add_edge returns 0 on success
    PASS();
}

// ---------------------------------------------------------------------------
// Undirected graph tests (need a graph created with is_directed=false)
// ---------------------------------------------------------------------------

// T1.7 — undirected add_edge: both nodes see each other as neighbors;
//         edge count is 1 (not 2)
static void test_undirected_both_directions(GraphBase &g)
{
    INFO();
    must_insert_node(g, 1700);
    must_insert_node(g, 1701);

    int ret = insert_edge_ret(g, 1700, 1701);
    assert(ret == 0);

    auto out_a = g.get_out_nodes_id(1700);
    assert(vec_contains(out_a, (node_id_t)1701));

    auto out_b = g.get_out_nodes_id(1701);
    assert(vec_contains(out_b, (node_id_t)1700));
    PASS();
}

// T1.8 — undirected: inserting the reverse (dst, src) is a duplicate
static void test_undirected_reverse_is_duplicate(GraphBase &g)
{
    INFO();
    must_insert_node(g, 1800);
    must_insert_node(g, 1801);

    int ret1 = insert_edge_ret(g, 1800, 1801);
    assert(ret1 == 0);

    // In an undirected graph (1801, 1800) is the same edge
    int ret2 = insert_edge_ret(g, 1801, 1800);
    assert(ret2 == WT_DUPLICATE_KEY);
    PASS();
}

// ---------------------------------------------------------------------------
// Convenience runners
// ---------------------------------------------------------------------------

static void run_directed_tests(GraphBase &g)
{
    test_add_vertex_new(g);
    test_add_vertex_duplicate(g);
    test_add_vertex_count(g);
    test_add_edge_directed(g);
    test_add_edge_implicit_nodes(g);
#ifdef MK_NEDGES
    // Duplicate edge detection requires the edge table (MK_NEDGES only).
    test_add_edge_duplicate(g);
#endif
    test_add_edge_node_count_stable(g);
#ifdef MK_NEDGES
    test_count_after_duplicate_edge(g);
    test_adjlist_integrity_after_rollback(g);
#endif
    test_new_dst_adjlist_queryable(g);
    test_multiple_in_edges(g);
    test_driver_add_vertex(g);
#ifdef MK_NEDGES
    // driver_add_edge_v2 tests duplicate detection — requires edge table.
    test_driver_add_edge_v2(g);
#endif
    test_add_edge_returns_zero_on_success(g);
}

static void run_undirected_tests(GraphBase &g)
{
    test_undirected_both_directions(g);
#ifdef MK_NEDGES
    // Duplicate reverse-edge detection requires the edge table (MK_NEDGES only).
    test_undirected_reverse_is_duplicate(g);
#endif
}
