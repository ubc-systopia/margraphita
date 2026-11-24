// test_linkpred_candidates.cpp — tiny DB test for linkpred_candidates.h.
// Graph (directed): 1→2 1→3 1→4 2→4 3→4 4→5 5→6, node 7 isolated.
#undef NDEBUG
#include <cassert>
#include <cstdio>
#include "graph_engine.h"
#include "linkpred_candidates.h"
#include "linkpred_pipeline.h"
#include "linkpred_fof.h"
#include "linkpred_khop.h"
using namespace linkpred;

int main()
{
    graph_opts opts;
    opts.create_new = true; opts.is_directed = true; opts.read_optimize = true;
    opts.type = GraphType::Adj; opts.db_dir = "./lp_cand_db"; opts.db_name = "t";
    GraphEngine engine(1, opts);
    GraphBase *g = engine.create_graph_handle();
    for (node_id_t i = 1; i <= 7; ++i) g->add_node({.id = i}, false);
    for (auto [s, d] : std::vector<std::pair<node_id_t, node_id_t>>{
             {1,2},{1,3},{1,4},{2,4},{3,4},{4,5},{5,6}})
        g->add_edge({.src_id = s, .dst_id = d}, false);

    assert(exhaustive_all_pairs(g).size() == 14);          // 21 pairs - 7 edges
    assert(exhaustive_all_pairs(g, false).size() == 21);
    assert(exhaustive_all_pairs(g, true).size() == 14);

    CandidateList fof = {{1,5},{2,5},{3,5},{4,6}};
    assert(friends_of_friends(g) == fof);
    assert(friends_of_friends(g, false).size() == 5);      // + (1,4)
    assert(distance_bounded_candidates(g, 2) == fof);
    assert(distance_bounded_candidates(g, 3).size() == 7);
    assert(uniform_random_sampling(g, 50, 1) == uniform_random_sampling(g, 50, 1));

    // per-u exhaustive: same pairs as the whole-graph version (edges kept)
    {
        ExhaustiveCandidates ex(g);
        CandidateList per_u;
        for (node_id_t u = 1; u <= 7; ++u)
            ex.for_each(u, [&](node_id_t v) { per_u.push_back({u, v}); return true; });
        assert(per_u == exhaustive_all_pairs(g, false));
        std::vector<node_id_t> first;                      // stop early after 2
        ex.for_each(1, [&](node_id_t v) { first.push_back(v); return first.size() < 2; });
        assert((first == std::vector<node_id_t>{2, 3}));
    }

    // per-u FoF / k-hop: same pairs as the whole-graph versions
    auto collect = [&](auto &gen) {
        CandidateList out;
        for (node_id_t u = 1; u <= 7; ++u)
            gen.for_each(u, [&](node_id_t v) { out.push_back({u, v}); return true; });
        return out;
    };
    FofCandidates fof_keep(g), fof_skip(g, true);
    assert(collect(fof_skip) == fof);
    assert(collect(fof_keep) == friends_of_friends(g, false));
    KHopCandidates k2(g, 2, true), k3(g, 3, true), k2_keep(g, 2);
    assert(collect(k2) == fof);
    assert(collect(k3) == distance_bounded_candidates(g, 3));
    assert(collect(k2_keep) == distance_bounded_candidates(g, 2, false));

    // pipeline: ExhaustiveCandidates must match the hand-written exhaustive run
    g_thread_handles = {g};
    for (const char *m : {"cn", "jaccard"}) {
        Options o; o.method = m; o.topk = 5;
        RunResult a = run_direct_exhaustive_structural_cursor(g, o);
        RunResult b = run_pipeline_structural(g, o, [](GraphBase *h) { return ExhaustiveCandidates(h); });
        assert(a.pairs_scored == 21 && a.pairs_scored == b.pairs_scored);
        assert(a.true_edges == 7 && a.true_edges == b.true_edges);
        assert(a.top_k.size() == b.top_k.size());
        for (size_t i = 0; i < a.top_k.size(); ++i)
            assert(a.top_k[i].u == b.top_k[i].u && a.top_k[i].v == b.top_k[i].v);
        RunResult f = run_pipeline_structural(g, o, [](GraphBase *h) { return FofCandidates(h); });
        assert(f.pairs_scored == 5);                       // fof pairs incl. (1,4)

        // --cursor-scope=block gives the same result as thread scope
        Options ob = o; ob.cursor_scope = "block";
        RunResult bb = run_pipeline_structural(g, ob, [](GraphBase *h) { return ExhaustiveCandidates(h); });
        assert(bb.pairs_scored == b.pairs_scored && bb.true_edges == b.true_edges);
        for (size_t i = 0; i < b.top_k.size(); ++i)
            assert(bb.top_k[i].u == b.top_k[i].u && bb.top_k[i].v == b.top_k[i].v);

        // hand-written baselines (--impl=direct) == pipeline, pair for pair
        auto same = [](const RunResult &x, const RunResult &y) {
            if (x.pairs_scored != y.pairs_scored || x.true_edges != y.true_edges) return false;
            if (x.top_k.size() != y.top_k.size()) return false;
            for (size_t i = 0; i < x.top_k.size(); ++i)
                if (x.top_k[i].u != y.top_k[i].u || x.top_k[i].v != y.top_k[i].v ||
                    x.top_k[i].score != y.top_k[i].score) return false;
            return true;
        };
        assert(same(run_direct_fof_structural_cursor(g, o), f));
        for (int k : {2, 3}) {
            Options ok = o; ok.k_hops = k;
            RunResult pk = run_pipeline_structural(g, ok, [&](GraphBase *h) { return KHopCandidates(h, k); });
            assert(same(run_direct_khop_structural_cursor_bfs(g, ok), pk));
        }

        // single-threaded pipeline == threaded pipeline (same generator)
        RunResult s1 = run_pipeline_structural_single(g, o, [](GraphBase *h) { return ExhaustiveCandidates(h); });
        assert(s1.pairs_scored == b.pairs_scored && s1.true_edges == b.true_edges);
        for (size_t i = 0; i < b.top_k.size(); ++i)
            assert(s1.top_k[i].u == b.top_k[i].u && s1.top_k[i].v == b.top_k[i].v);
    }

    g->close(false);
    engine.close_graph();
    std::printf("all linkpred candidate tests passed\n");
}
