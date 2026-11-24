#include "graph_engine.h"
#include "ml/ml_sampling.h"
#include <stdexcept>
#include <iostream>
#include <set>
#include <filesystem>
#include <chrono>
using namespace ml_sampling;
static void check(bool ok) { if (!ok) throw std::runtime_error("sampling assertion failed"); }
using Pair = std::pair<node_id_t,node_id_t>;
static std::multiset<Pair> edges(const SampledGraph& b) {
    std::multiset<Pair> result;
    for (auto e : b.edges) result.emplace(b.node_ids.at(e.src), b.node_ids.at(e.dst));
    return result;
}
int main() {
    graph_opts opts;
    opts.create_new=true; opts.optimize_create=false; opts.is_directed=true;
    opts.read_optimize=true; opts.is_weighted=false; opts.has_node_props=false;
    opts.has_edge_props=false; opts.type=GraphType::Adj;
    opts.db_dir="./db_test/ml_sampling_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    opts.db_name="sampling"; opts.conn_config="cache_size=256MB"; opts.stat_log="./";
    GraphEngine engine(1,opts);
    auto* g=engine.create_graph_handle();
    for (node_id_t id : {1, 7, 20, 50, 99}) { node n{}; n.id=id; g->add_node(n,false); }
    // Relabelled so every edge has src < dst (OrderNodes builds store min->max): 50 seed, 7/20 hop 1, 1 hop 2.
    for (Pair p : {Pair{7,50},Pair{20,50},Pair{1,7},Pair{1,20}}) {
        edge e{}; e.src_id=p.first; e.dst_id=p.second; g->add_edge(e,false);
    }
    g->close(true); // fresh reader after writes
    g=engine.create_graph_handle();
    auto b=sample_neighbors(*g,{50,50,99},{{-1,-1},Direction::Incoming,false,7});
    check(b.seed_indices==std::vector<std::size_t>{0,0,1});
    check(b.node_ids==std::vector<node_id_t>{50,99,7,20,1});
    check(edges(b)==std::multiset<Pair>{{7,50},{20,50},{1,7},{1,20}});
    check(b.hops.size()==2 && b.hops[1].frontier.size()==2);
    auto o=sample_neighbors(*g,{1},{{-1,-1},Direction::Outgoing,false,7});
    check(edges(o)==edges(b));
    auto s=sample_neighbors(*g,{50},{{1},Direction::Incoming,false,7});
    auto t=sample_neighbors(*g,{50},{{1},Direction::Incoming,false,7});
    check(s.edges.size()==1 && s.node_ids==t.node_ids && edges(s)==edges(t));
    check(sample_neighbors(*g,{50},{{5},Direction::Incoming,true,7}).edges.size()==5);
    check(sample_neighbors(*g,{50},{{5},Direction::Incoming,false,7}).edges.size()==2);
    check(sample_neighbors(*g,{50},{{0,-1},Direction::Incoming,false,7}).edges.empty());
    check(sample_neighbors(*g,{50},{{-1},Direction::Outgoing,false,7}).edges.empty());
    check(sample_neighbors(*g,{}).node_ids.empty());
    bool rejected=false;
    try { sample_neighbors(*g,{12345}); } catch(const std::invalid_argument&) { rejected=true; }
    check(rejected); rejected=false;
    try { sample_neighbors(*g,{50},{{-2}}); } catch(const std::invalid_argument&) { rejected=true; }
    check(rejected);
    g->close(false); engine.close_graph();
    std::cout << "ML sampling tests passed; scratch DB: " << opts.db_dir << '\n';
}
