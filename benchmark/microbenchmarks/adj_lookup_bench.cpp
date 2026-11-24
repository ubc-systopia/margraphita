// adj_lookup_bench: per-call cost of AdjList neighbour lookups by vertex ID.
// Calls get_out_nodes_id / get_in_nodes_id / get_out_nodes / get_in_nodes for every
// vertex plus 1000 IDs past the max (missing records). The first pass is a warm-up.
// Prints ns per call and a total/hash of the results, so two builds can be compared
// for speed and for identical output. Opens the DB read-only.
//
// Written for the change that removed the has_node() guards from these calls
// (2026-11-24): web-Stanford get_out_nodes_id 683 -> 443 ns, get_in_nodes_id 684 -> 411 ns
// (+410 in-edges of sinks that the guard hid), s17 get_out_nodes_id 866 -> 604 ns.
//
//   adj_lookup_bench -m adj_rd_web-Stanford -p <db dir> -g adj -r -d [--reps=3]
#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include "command_line.h"
#include "graph_engine.h"

using Clock = std::chrono::steady_clock;

int main(int argc, char** argv)
{
    int reps = 3;
    std::vector<char*> rest{argv[0]};
    for (int i = 1; i < argc; ++i) {
        std::string a(argv[i]);
        if (a.rfind("--reps=", 0) == 0) reps = std::stoi(a.substr(7));
        else rest.push_back(argv[i]);
    }
    CmdLineApp cli(static_cast<int>(rest.size()), rest.data());
    if (!cli.parse_args()) return 1;
    auto opts = cli.get_parsed_opts();
    if (opts.type != GraphType::Adj) { std::cerr << "adj_lookup_bench needs -g adj\n"; return 1; }
    opts.create_new = false; opts.read_only = true;
    opts.has_node_props = false; opts.has_edge_props = false;

    GraphEngine eng(1, opts);
    GraphBase* g = eng.create_graph_handle();
    std::vector<node_id_t> ids;
    auto* nc = g->get_node_iter();
    nc->set_key_range({0, OutOfBand_ID_MAX});
    node n; nc->next(&n);
    while (n.id != OutOfBand_ID_MAX) { ids.push_back(n.id); nc->next(&n); }
    nc->close(); delete nc;
    const node_id_t mx = ids.back();
    for (int i = 1; i <= 1000; ++i) ids.push_back(mx + i);   // missing records

    auto run = [&](const char* name, auto f) {
        for (int r = 0; r <= reps; ++r) {
            uint64_t total = 0, h = 1469598103934665603ULL;   // FNV-1a over results
            auto t = Clock::now();
            for (auto v : ids) {
                auto x = f(v); total += x.size();
                for (auto y : x) h = (h ^ y) * 1099511628211ULL;
            }
            double s = std::chrono::duration<double>(Clock::now() - t).count();
            if (r) std::cout << name << " rep=" << r << " s=" << s << " ns_per_call=" << s * 1e9 / ids.size()
                             << " total_nbrs=" << total << " hash=" << h << '\n';
        }
    };
    // get_*_nodes return node structs: hash id and degrees together
    auto pack = [](const std::vector<node>& ns) {
        std::vector<node_id_t> r;
        for (auto& x : ns) r.push_back(x.id ^ ((node_id_t)x.in_degree << 40) ^ ((node_id_t)x.out_degree << 20));
        return r;
    };
    run("get_out_nodes_id", [&](node_id_t v) { return g->get_out_nodes_id(v); });
    if (opts.is_directed) run("get_in_nodes_id", [&](node_id_t v) { return g->get_in_nodes_id(v); });
    run("get_out_nodes", [&](node_id_t v) { return pack(g->get_out_nodes(v)); });
    if (opts.is_directed) run("get_in_nodes", [&](node_id_t v) { return pack(g->get_in_nodes(v)); });
    std::cout << "calls_per_rep=" << ids.size() << '\n';
    g->close(false); eng.close_graph();
    return 0;
}
