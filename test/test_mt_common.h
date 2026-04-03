// test_mt_common.h
//
// Shared multi-threaded test logic.
// Included by test_mt_adj.cpp and test_mt_ekey.cpp.
//
// Tests:
//   T4.1  concurrent add_vertex with distinct IDs — final count exact
//   T4.2  concurrent add_edge with distinct edges — final count exact
//   T4.3  concurrent add_edge with overlapping node IDs — no corruption
//   T4.4  mixed read/write — readers never crash or see torn state
//
// These tests exercise the thread_local set_key fix: without it, concurrent
// calls to CommonUtil::set_key overwrite each other's stack buffer and cause
// corrupted keys / crashes / wrong results.

#pragma once

#include <atomic>
#include <cassert>
#include <cstdio>
#include <set>
#include <thread>
#include <vector>

#include "common_defs.h"
#include "graph.h"
#include "graph_engine.h"

#ifndef PASS
#define PASS() fprintf(stderr, "%s: PASSED\n", __FUNCTION__)
#define INFO() fprintf(stderr, "--- %s\n", __FUNCTION__)
#endif

static const int MT_THREADS        = 4;
static const int MT_NODES_PER_THREAD  = 100;
static const int MT_EDGES_PER_THREAD  = 200;

// Retry add_edge until it either succeeds (0) or gets a definitive
// non-transient result (DUPLICATE_KEY).  Returns the final ret code.
static int add_edge_with_retry(GraphBase &g, node_id_t src, node_id_t dst)
{
    edge e{}; e.src_id = src; e.dst_id = dst;
    for (;;)
    {
        int ret = g.add_edge(e, false);
        if (ret == 0 || ret == WT_DUPLICATE_KEY) return ret;
        // WT_ROLLBACK or similar transient: retry immediately
    }
}

static int add_node_with_retry(GraphBase &g, node_id_t id)
{
    node n{}; n.id = id;
    for (;;)
    {
        int ret = g.add_node(n, false);
        if (ret == 0 || ret == WT_DUPLICATE_KEY) return ret;
    }
}

// ---------------------------------------------------------------------------
// T4.1 — concurrent add_vertex: distinct IDs, final count exactly N*T
// ---------------------------------------------------------------------------
static void test_mt_add_vertex(GraphEngine &engine, const char *label)
{
    INFO();
    // Node ID layout: thread t owns IDs [t*1000+10000 .. t*1000+10000+N)
    // (offset 10000 avoids clashing with single-threaded tests)
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;
    uint64_t nodes_before = 0;
    {
        GraphBase *tmp = engine.create_graph_handle();
        nodes_before = tmp->get_num_nodes();
        tmp->close(false);
    }

    for (int t = 0; t < MT_THREADS; t++)
    {
        threads.emplace_back([&engine, t, &errors]()
        {
            GraphBase *g = engine.create_graph_handle();
            node_id_t base = (node_id_t)(10000 + t * 1000);
            for (int i = 0; i < MT_NODES_PER_THREAD; i++)
            {
                int ret = add_node_with_retry(*g, base + (node_id_t)i);
                if (ret != 0 && ret != WT_DUPLICATE_KEY)
                    errors.fetch_add(1, std::memory_order_relaxed);
            }
            g->close(false);
        });
    }
    for (auto &th : threads) th.join();

    assert(errors.load() == 0);

    GraphBase *g = engine.create_graph_handle();
    uint64_t added = g->get_num_nodes() - nodes_before;
    g->close(false);

    assert(added == (uint64_t)(MT_THREADS * MT_NODES_PER_THREAD));
    fprintf(stderr, "  [%s] T4.1: %llu nodes added across %d threads\n",
            label, (unsigned long long)added, MT_THREADS);
    PASS();
}

// ---------------------------------------------------------------------------
// T4.2 — concurrent add_edge: distinct edges, final count exactly N*T
// ---------------------------------------------------------------------------
static void test_mt_add_edge_distinct(GraphEngine &engine, const char *label)
{
    INFO();
    // Edge layout: thread t inserts edges (src=20000+t*10000+i, dst=src+1)
    // All src/dst pairs are globally unique.
    std::atomic<int> errors{0};
    std::vector<std::thread> threads;

    uint64_t edges_before = 0;
    {
        GraphBase *tmp = engine.create_graph_handle();
        edges_before = tmp->get_num_edges();
        tmp->close(false);
    }

    for (int t = 0; t < MT_THREADS; t++)
    {
        threads.emplace_back([&engine, t, &errors]()
        {
            GraphBase *g = engine.create_graph_handle();
            node_id_t base = (node_id_t)(20000 + t * 10000);
            for (int i = 0; i < MT_EDGES_PER_THREAD; i++)
            {
                node_id_t src = base + (node_id_t)(i * 2);
                node_id_t dst = src + 1;
                int ret = add_edge_with_retry(*g, src, dst);
                if (ret != 0 && ret != WT_DUPLICATE_KEY)
                    errors.fetch_add(1, std::memory_order_relaxed);
            }
            g->close(false);
        });
    }
    for (auto &th : threads) th.join();

    assert(errors.load() == 0);

    GraphBase *g = engine.create_graph_handle();
    uint64_t added = g->get_num_edges() - edges_before;
    g->close(false);

    assert(added == (uint64_t)(MT_THREADS * MT_EDGES_PER_THREAD));
    fprintf(stderr, "  [%s] T4.2: %llu edges added across %d threads\n",
            label, (unsigned long long)added, MT_THREADS);
    PASS();
}

// ---------------------------------------------------------------------------
// T4.3 — concurrent add_edge with overlapping destination nodes
//         (many threads write edges pointing to the same N "hub" nodes).
//         After joining: edge count is exact; hub adjacency lists are intact.
// ---------------------------------------------------------------------------
static void test_mt_add_edge_shared_dst(GraphEngine &engine, const char *label)
{
    INFO();
    // 4 hub nodes shared across all threads.
    // IDs chosen beyond T4.2's range (T4.2 thread 3 reaches ~50400).
    static const int N_HUBS = 4;
    node_id_t hubs[N_HUBS];
    int hub_baseline_in[N_HUBS] = {};
    {
        GraphBase *g = engine.create_graph_handle();
        for (int h = 0; h < N_HUBS; h++)
        {
            hubs[h] = (node_id_t)(60000 + h);
            add_node_with_retry(*g, hubs[h]);
            hub_baseline_in[h] = (int)g->get_in_nodes_id(hubs[h]).size();
        }
        g->close(false);
    }

    // Each thread inserts MT_EDGES_PER_THREAD edges:
    //   src = 61000 + t * 10000 + i  (globally unique, beyond hub IDs)
    //   dst = hubs[i % N_HUBS]        (shared)
    std::atomic<int> errors{0};
    std::atomic<int> successful_inserts{0};
    std::vector<std::thread> threads;
    uint64_t edges_before = 0;
    {
        GraphBase *tmp = engine.create_graph_handle();
        edges_before = tmp->get_num_edges();
        tmp->close(false);
    }

    for (int t = 0; t < MT_THREADS; t++)
    {
        threads.emplace_back([&engine, t, &hubs, &errors, &successful_inserts]()
        {
            GraphBase *g = engine.create_graph_handle();
            node_id_t base = (node_id_t)(61000 + t * 10000);
            for (int i = 0; i < MT_EDGES_PER_THREAD; i++)
            {
                node_id_t src = base + (node_id_t)i;
                node_id_t dst = hubs[i % N_HUBS];
                int ret = add_edge_with_retry(*g, src, dst);
                if (ret == 0)
                    successful_inserts.fetch_add(1, std::memory_order_relaxed);
                else if (ret != WT_DUPLICATE_KEY)
                    errors.fetch_add(1, std::memory_order_relaxed);
            }
            g->close(false);
        });
    }
    for (auto &th : threads) th.join();

    assert(errors.load() == 0);

    GraphBase *g = engine.create_graph_handle();
    uint64_t added = g->get_num_edges() - edges_before;

    // All edges have unique src, so no duplicates: added must equal inserted
    assert(added == (uint64_t)successful_inserts.load());
    assert(added == (uint64_t)(MT_THREADS * MT_EDGES_PER_THREAD));

    // Each hub must have exactly MT_THREADS * (MT_EDGES_PER_THREAD / N_HUBS)
    // in-edges.  Verify none are missing (no torn writes).
    int expected_in_delta = MT_THREADS * (MT_EDGES_PER_THREAD / N_HUBS);
    for (int h = 0; h < N_HUBS; h++)
    {
        auto in = g->get_in_nodes_id(hubs[h]);
        int actual_delta = (int)in.size() - hub_baseline_in[h];
        assert(actual_delta == expected_in_delta);
    }
    g->close(false);

    fprintf(stderr, "  [%s] T4.3: %llu edges, hub in-degree delta = %d each\n",
            label, (unsigned long long)added, expected_in_delta);
    PASS();
}

// ---------------------------------------------------------------------------
// T4.4 — mixed read/write: readers call get_out_nodes_id on a node that
//         writers are simultaneously extending.  Verifies no crash or
//         assertion failure under concurrent load.
// ---------------------------------------------------------------------------
static void test_mt_mixed_read_write(GraphEngine &engine, const char *label)
{
    INFO();
    // Seed a source node that readers will query
    node_id_t src = 40000;
    {
        GraphBase *g = engine.create_graph_handle();
        add_node_with_retry(*g, src);
        g->close(false);
    }

    std::atomic<bool> stop{false};
    std::atomic<int> read_errors{0};
    std::vector<std::thread> threads;

    // 2 writer threads: each appends 50 edges from src
    for (int t = 0; t < 2; t++)
    {
        threads.emplace_back([&engine, src, t, &stop]()
        {
            GraphBase *g = engine.create_graph_handle();
            node_id_t base = (node_id_t)(40100 + t * 1000);
            for (int i = 0; i < 50; i++)
                add_edge_with_retry(*g, src, base + (node_id_t)i);
            g->close(false);
            (void)stop;
        });
    }

    // 2 reader threads: repeatedly read src's out-neighbors until writers done
    for (int t = 0; t < 2; t++)
    {
        threads.emplace_back([&engine, src, &stop, &read_errors]()
        {
            GraphBase *g = engine.create_graph_handle();
            int iters = 0;
            while (!stop.load(std::memory_order_relaxed) && iters < 500)
            {
                // Reading must not crash; result may be any prefix of the
                // final list (snapshot isolation), but must not throw.
                try {
                    auto nbrs = g->get_out_nodes_id(src);
                    (void)nbrs;
                } catch (...) {
                    read_errors.fetch_add(1, std::memory_order_relaxed);
                }
                iters++;
            }
            g->close(false);
        });
    }

    for (auto &th : threads) th.join();
    stop = true;

    assert(read_errors.load() == 0);
    fprintf(stderr, "  [%s] T4.4: mixed read/write completed without errors\n",
            label);
    PASS();
}

// ---------------------------------------------------------------------------
// Convenience runner
// ---------------------------------------------------------------------------

static void run_mt_tests(GraphEngine &engine, const char *label)
{
    test_mt_add_vertex(engine, label);
    test_mt_add_edge_distinct(engine, label);
    test_mt_add_edge_shared_dst(engine, label);
    test_mt_mixed_read_write(engine, label);
}
