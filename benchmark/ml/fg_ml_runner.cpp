#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/resource.h>

#include "command_line.h"
#include "graph_engine.h"
#include "ml/algorithms/fastrp.h"
#include "ml/ml_access.h"
#include "linkpred_candidates.h"
#include "linkpred_scores.h"
#include "linkpred_topk.h"
#include "linkpred_structural_fof.h"
#include "linkpred_structural_khop.h"
#include "linkpred_structural_exhaustive.h"
#include "linkpred_embedding_fof.h"
#include "linkpred_embedding_khop.h"
#include "linkpred_embedding_exhaustive.h"

namespace {
using Clock = std::chrono::steady_clock;
double elapsed(Clock::time_point t) {
    return std::chrono::duration<double>(Clock::now() - t).count();
}
struct RunnerOptions {
    ml::FastRPOptions fr;
    std::string prediction = "embedding";
    std::string embedding = "stored", candidates = "fof", method = "cosine";
    std::string impl = "pipeline", variant = "cursor", direction = "out";
    std::string output = "ml_predictions.csv";
    size_t batch = 256, topk = 100, max_pairs = 0;
    int hops = 2, draws = 10000;
    ml_access::RetrievalStrategy strategy = ml_access::RetrievalStrategy::Direct;
    bool allow_write = false;
    bool count_fetches = false;   // --count-fetches: requested / unique vectors and node_props searches (pipeline)
    std::string source_selection = "all";   // all | uniform (pipeline; fof, khop, exhaustive)
    size_t source_count = 0;                // required for uniform
    uint64_t sampling_seed = 42;
    std::string source_out;                 // optional: write selected source IDs, one per line
};
size_t number(const std::string& s) {
    if (s.empty() || s.find_first_not_of("0123456789") != std::string::npos)
        throw std::runtime_error("Expected unsigned integer: " + s);
    return std::stoull(s);
}
int positive(const std::string& s) {
    auto n = number(s);
    if (!n || n > 2147483647) throw std::runtime_error("Invalid positive integer: " + s);
    return static_cast<int>(n);
}
RunnerOptions parse(int argc, char** argv, std::vector<char*>& rest) {
    RunnerOptions o; o.fr.threads = 4; o.fr.write_db = "after";
    rest.push_back(argv[0]);
    for (int i=1; i<argc; ++i) {
        std::string a = argv[i]; auto eq = a.find('=');
        auto key = a.substr(0,eq), v = eq == std::string::npos ? "" : a.substr(eq+1);
        if (key=="--prediction") o.prediction=v;
        else if (key=="--impl") o.impl=v;
        else if (key=="--variant") o.variant=v;
        else if (key=="--direction") o.direction=v;
        else if (key=="--embedding") o.embedding=v;
        else if (key=="--task") { if(v!="linkpred") throw std::runtime_error("Only linkpred supported"); }
        else if (key=="--candidates") o.candidates=v;
        else if (key=="--method") o.method=v;
        else if (key=="--dim") o.fr.dim=positive(v);
        else if (key=="--iters") o.fr.iters=positive(v);
        else if (key=="--threads") o.fr.threads=positive(v);
        else if (key=="--seed") o.fr.seed=number(v);
        else if (key=="--write-db") o.fr.write_db=v;
        else if (key=="--batch-size") o.batch=number(v);
        else if (key=="--top-k") o.topk=number(v);
        else if (key=="--max-pairs") o.max_pairs=number(v);
        else if (key=="--hops") o.hops=positive(v);
        else if (key=="--draws") o.draws=positive(v);
        else if (key=="--out") o.output=v;
        else if (key=="--allow-scratch-writes") o.allow_write=true;
        else if (key=="--count-fetches") o.count_fetches=true;
        else if (key=="--source-selection") o.source_selection=v;
        else if (key=="--source-count") o.source_count=number(v);
        else if (key=="--sampling-seed") o.sampling_seed=number(v);
        else if (key=="--source-out") o.source_out=v;
        else if (key=="--retrieval") {
            if(v=="direct") o.strategy=ml_access::RetrievalStrategy::Direct;
            else if(v=="sorted") o.strategy=ml_access::RetrievalStrategy::Sorted;
            else if(v=="dedup") o.strategy=ml_access::RetrievalStrategy::SortedDeduplicated;
            else throw std::runtime_error("Invalid retrieval strategy");
        } else rest.push_back(argv[i]);
    }
    if(o.embedding!="fastrp" && o.embedding!="stored") throw std::runtime_error("Invalid embedding mode");
    if(o.candidates!="fof" && o.candidates!="khop" && o.candidates!="exhaustive" && o.candidates!="uniform") throw std::runtime_error("Invalid candidate strategy");
    if(o.prediction!="embedding" && o.prediction!="structural") throw std::runtime_error("Invalid prediction type");
    if(o.prediction=="structural") {
        if(o.method!="cn" && o.method!="jaccard" && o.method!="cosine") throw std::runtime_error("Structural methods: cn, jaccard, cosine");
        if(o.embedding=="fastrp" || o.count_fetches || o.strategy!=ml_access::RetrievalStrategy::Direct) throw std::runtime_error("Structural mode does not use embedding generation, fetch counters or embedding retrieval strategies");
        if(o.impl=="direct" && o.method=="cosine") throw std::runtime_error("Structural cosine currently supported by pipeline only");
    } else if(o.method!="cosine" && o.method!="dot") throw std::runtime_error("Invalid embedding scoring method");
    if(!o.batch || !o.topk || o.output.empty()) throw std::runtime_error("Invalid batch, top-k or output");
    if(o.fr.write_db!="after" && o.fr.write_db!="fused") throw std::runtime_error("Expected after or fused");
    if(o.embedding=="fastrp" && !o.allow_write) throw std::runtime_error("Generation overwrites node_props: use a disposable copy and --allow-scratch-writes");
    if(o.impl!="pipeline" && o.impl!="direct") throw std::runtime_error("Expected pipeline or direct");
    if(o.direction!="out" && o.direction!="both") throw std::runtime_error("Expected out or both");
    if(o.impl=="pipeline" && (o.variant!="cursor" || o.direction!="out")) throw std::runtime_error("Pipeline supports cursor/out only");
    if(o.source_selection!="all" && o.source_selection!="uniform") throw std::runtime_error("Expected --source-selection=all|uniform");
    if(o.source_selection=="uniform") {
        if(!o.source_count) throw std::runtime_error("--source-selection=uniform needs --source-count>0");
        if(o.impl=="direct") throw std::runtime_error("Source selection is pipeline-only; direct always uses all vertices");
        if(o.candidates=="uniform") throw std::runtime_error("Source selection does not apply to uniform pair sampling");
    } else if(o.source_count || !o.source_out.empty()) throw std::runtime_error("--source-count/--source-out need --source-selection=uniform");
    if(o.impl=="direct") {
        if(o.max_pairs || o.candidates=="uniform") throw std::runtime_error("Direct requires full workload; no max-pairs or uniform");
        if(o.strategy!=ml_access::RetrievalStrategy::Direct) throw std::runtime_error("Direct uses legacy property retrieval");
        if(o.topk>2147483647) throw std::runtime_error("Direct top-k exceeds int range");
        if(o.variant!="cursor" && !(o.candidates=="khop" && o.hops==2 && o.variant=="outcursor_nested") && !(o.candidates=="exhaustive" && o.variant=="tiled")) throw std::runtime_error("Unsupported direct variant");
    }
    return o;
}
// Separate engine lifetime ensures the scoring stage sees committed properties.
void generate(graph_opts opts, const RunnerOptions& o) {
    opts.read_only=false;
    GraphEngine engine(o.fr.threads+1,opts);
    WT_CONNECTION* conn=engine.get_connection(); WT_SESSION* sess=nullptr;
    int ret=conn->open_session(conn,nullptr,nullptr,&sess);
    if(!ret) ret=sess->create(sess,(std::string("table:")+AdjList::NODE_PROPS_TABLE).c_str(),"key_format=u,value_format=u,leaf_page_max=64KB");
    if(sess) sess->close(sess,nullptr);
    if(ret) throw std::runtime_error(wiredtiger_strerror(ret));
    auto* g=engine.create_graph_handle();
    std::vector<GraphBase*> workers;
    for(int i=0;i<o.fr.threads;++i) workers.push_back(engine.create_graph_handle());
    auto t=Clock::now();
    { auto result=ml::fastrp(*g,workers,o.fr,opts.is_directed);
      std::cerr<<"embedding_rows="<<result.embeddings.rows<<" generation_and_write_s="<<elapsed(t)<<'\n'; }
    for(auto* w:workers) w->close(false);
    g->close(false); engine.close_graph();
}
// Reuse the specialized benchmark implementations without modifying them.
void predict_direct(graph_opts opts, const RunnerOptions& o) {
    opts.read_only=true;
    ::Options lp;
    lp.impl="direct"; lp.method=o.method; lp.candidates=o.candidates;
    lp.variant=o.variant; lp.direction=o.direction; lp.k_hops=o.hops;
    lp.threads=o.fr.threads; lp.topk=static_cast<int>(o.topk);
    g_graph_directed=opts.is_directed;
    auto t=Clock::now();
    GraphEngine engine(lp.threads+1,opts);
    std::string checkpoint="WiredTigerCheckpoint";
    auto* g=engine.create_ro_graph_handle(checkpoint);
    g_thread_handles.clear();
    for(int i=0;i<lp.threads;++i) g_thread_handles.push_back(engine.create_ro_graph_handle(checkpoint));
    std::cerr<<"prediction_open_s="<<elapsed(t)<<'\n';
    t=Clock::now();
    RunResult r;
    if(o.prediction=="structural") {
        if(o.candidates=="fof") r=run_direct_fof_structural_cursor(g,lp);
        else if(o.candidates=="khop") {
            if(o.variant=="outcursor_nested") r=run_direct_khop_structural_outcursor_nested(g,lp);
            else r=run_direct_khop_structural_cursor_bfs(g,lp);
        } else if(o.variant=="tiled") r=run_direct_exhaustive_structural_tiled(g,lp);
        else r=run_direct_exhaustive_structural_cursor(g,lp);
    } else if(o.candidates=="fof") r=run_direct_fof_embedding_cursor(g,lp);
    else if(o.candidates=="khop") {
        if(o.variant=="outcursor_nested") r=run_direct_khop_embedding_outcursor_nested(g,lp);
        else r=run_direct_khop_embedding_cursor_bfs(g,lp);
    } else if(o.variant=="tiled") r=run_direct_exhaustive_embedding_tiled(g,lp);
    else r=run_direct_exhaustive_embedding_cursor(g,lp);
    double workload_s=elapsed(t);
    for(auto* h:g_thread_handles) h->close(false);
    g_thread_handles.clear(); g->close(false); engine.close_graph();
    std::ofstream out(o.output);
    out<<"u,v,score,label\n"<<std::setprecision(17);
    for(const auto& p:r.top_k) out<<p.u<<','<<p.v<<','<<p.score<<','<<p.label<<'\n';
    out.close(); if(!out) throw std::runtime_error("Cannot write output: "+o.output);
    std::cerr<<"impl=direct variant="<<o.variant<<" pairs_scored="<<r.pairs_scored
             <<" positive_pairs="<<r.true_edges<<" top_k="<<r.top_k.size()<<" workload_s="<<workload_s<<'\n';
}
// Single-threaded orchestration baseline; FastRP itself remains parallel.
// --count-fetches: actual node_props searches from WiredTiger's per-table statistics.
long long node_props_searches(WT_SESSION* s) {
    if (!s) return -1;
    WT_CURSOR* c = nullptr;
    const std::string uri = std::string("statistics:table:") + AdjList::NODE_PROPS_TABLE;
    if (s->open_cursor(s, uri.c_str(), nullptr, nullptr, &c) != 0) return -1;
    c->set_key(c, WT_STAT_DSRC_CURSOR_SEARCH); long long v = -1;
    if (c->search(c) == 0) { const char *d, *pv; int64_t val; c->get_value(c, &d, &pv, &val); v = val; }
    c->close(c); return v;
}
void predict(graph_opts opts,const RunnerOptions& o) {
    opts.read_only=true;
    if (o.count_fetches) opts.conn_config += (opts.conn_config.empty() ? "" : ",") + std::string("statistics=(fast)");
    auto t=Clock::now(); GraphEngine engine(1,opts); auto* g=engine.create_graph_handle();
    WT_SESSION* stat_s=nullptr;
    if (o.count_fetches) engine.get_connection()->open_session(engine.get_connection(),nullptr,nullptr,&stat_s);
    size_t requested=0, unique_in_batches=0, batches=0;
    std::vector<node_id_t> uniq;
    std::cerr<<"prediction_open_s="<<elapsed(t)<<'\n';
    linkpred::TopK top(o.topk);
    ml_access::VectorRequest request{static_cast<size_t>(o.fr.dim),o.strategy};
    std::vector<linkpred::CandidatePair> pending;
    pending.reserve(o.batch);
    size_t scored=0, positives=0; double retrieval_s=0, scoring_s=0;
    auto flush=[&]() {
        if(pending.empty()) return;
        if(o.prediction=="structural") {
            for(auto [u,v]:pending) {
                auto start=Clock::now();
                auto a=g->get_out_nodes_id(u), b=g->get_out_nodes_id(v);
                std::sort(a.begin(),a.end()); std::sort(b.begin(),b.end());
                retrieval_s+=elapsed(start); start=Clock::now();
                double score;
                if(o.method=="cosine") {
                    double denom=std::sqrt(double(a.size())*double(b.size()));
                    score=denom>0 ? double(linkpred::common_neighbors(a,b))/denom : 0.0;
                } else score=linkpred::score_pair(o.method,a,b);
                int label=g->has_edge(u,v)?1:0;
                top.offer({u,v,score,label}); ++scored; positives+=label;
                scoring_s+=elapsed(start);
            }
            pending.clear(); return;
        }
        std::vector<node_id_t> ids; ids.reserve(2*pending.size());
        for(auto p:pending) { ids.push_back(p.first); ids.push_back(p.second); }
        if (o.count_fetches) {   // counted outside the timed retrieval
            requested+=ids.size(); ++batches;
            uniq.assign(ids.begin(),ids.end()); std::sort(uniq.begin(),uniq.end());
            unique_in_batches+=std::unique(uniq.begin(),uniq.end())-uniq.begin();
        }
        auto start=Clock::now(); auto vectors=ml_access::get_vertex_vectors(*g,ids,request);
        retrieval_s+=elapsed(start); start=Clock::now();
        for(size_t i=0;i<pending.size();++i) {
            auto [u,v]=pending[i]; auto* a=vectors.values.data()+2*i*request.dimension;
            auto* b=a+request.dimension;
            double score=o.method=="cosine" ? linkpred::cosine_from_ptrs(a,b,request.dimension) : linkpred::dot_from_ptrs(a,b,request.dimension);
            if(!std::isfinite(score)) throw std::runtime_error("Nonfinite score");
            int label=g->has_edge(u,v)?1:0;
            top.offer({u,v,score,label}); ++scored; positives+=label;
        }
        scoring_s+=elapsed(start); pending.clear();
    };
    size_t active_sources=0; node_id_t last_u=OutOfBand_ID_MAX;
    auto emit=[&](linkpred::CandidatePair p) {
        if(p.first!=last_u) { ++active_sources; last_u=p.first; }   // generators emit grouped by u
        pending.push_back(p);
        bool stop=o.max_pairs && scored+pending.size()>=o.max_pairs;
        if(pending.size()>=o.batch || stop) flush();
        return !stop;
    };
    // Source selection (header: linkpred::sample_source_vertices). nullptr = all vertices, unchanged path.
    std::vector<node_id_t> sources; const std::vector<node_id_t>* src=nullptr;
    linkpred::ReverseFilter rf; linkpred::ReverseFilter* rev=nullptr;   // undirected sampled sources only
    if(o.source_selection=="uniform") {
        auto ts=Clock::now();
        sources=linkpred::sample_source_vertices(g,o.source_count,o.sampling_seed); src=&sources;
        if(!opts.is_directed) { rf=linkpred::make_reverse_filter(sources); rev=&rf; }
        std::cerr<<"source_selection=uniform source_count_requested="<<o.source_count<<" sources_selected="<<sources.size()
                 <<" sampling_seed="<<o.sampling_seed<<" min_source="<<(sources.empty()?0:sources.front())
                 <<" max_source="<<(sources.empty()?0:sources.back())<<" sampling_s="<<elapsed(ts)<<'\n';
        if(sources.size()<o.source_count) std::cerr<<"WARNING: graph has fewer vertices than --source-count; using all "<<sources.size()<<'\n';
        if(!o.source_out.empty()) {
            std::ofstream so(o.source_out); for(auto id:sources) so<<id<<'\n';
            if(!so) throw std::runtime_error("Cannot write source IDs: "+o.source_out);
        }
    } else std::cerr<<"source_selection=all\n";
    const long long searches0 = o.count_fetches ? node_props_searches(stat_s) : -1;
    t=Clock::now();
    if(o.candidates=="fof") linkpred::friends_of_friends_each(g,emit,false,src,rev);
    else if(o.candidates=="khop") linkpred::distance_bounded_candidates_each(g,o.hops,emit,false,src,rev);
    else if(o.candidates=="uniform") linkpred::uniform_random_sampling_each(g,o.draws,static_cast<unsigned>(o.fr.seed),emit,false);
    else linkpred::exhaustive_all_pairs_each(g,emit,false,src,rev);
    flush(); double workload_s=elapsed(t);
    auto ranked=top.drain_sorted(); std::ofstream out(o.output);
    out<<"u,v,score,label\n"<<std::setprecision(17);
    for(const auto& p:ranked) out<<p.u<<','<<p.v<<','<<p.score<<','<<p.label<<'\n';
    out.close(); if(!out) throw std::runtime_error("Cannot write output: "+o.output);
    std::cerr<<"pairs_scored="<<scored<<" positive_pairs="<<positives<<" top_k="<<ranked.size()
             <<" retrieval_s="<<retrieval_s<<" scoring_label_ranking_s="<<scoring_s<<" workload_s="<<workload_s<<'\n';
    std::cerr<<"sources_with_candidates="<<active_sources<<" pairs_per_active_source="
             <<(active_sources?double(scored)/active_sources:0.0)<<'\n';
    if(src) std::cerr<<"reverse_duplicates_skipped="<<(rev?rf.skipped:0)
                     <<" reverse_filter="<<(rev?"undirected":"off_directed")<<'\n';
    if (o.count_fetches) {
        const long long searches1 = node_props_searches(stat_s);
        std::cerr<<"batch_size="<<o.batch<<" batches="<<batches<<" requested_vectors="<<requested
                 <<" unique_vectors_in_batches="<<unique_in_batches
                 <<" node_props_searches="<<(searches0>=0 && searches1>=0 ? searches1-searches0 : -1)<<'\n';
        if (stat_s) stat_s->close(stat_s,nullptr);
    }
    g->close(false); engine.close_graph();
}
} // namespace
int main(int argc,char** argv) {
    try {
        auto t=Clock::now(); std::vector<char*> rest; auto o=parse(argc,argv,rest);
        CmdLineApp cli(static_cast<int>(rest.size()),rest.data());
        if(!cli.parse_args()) return 1;
        auto opts=cli.get_parsed_opts();
        if(opts.type!=GraphType::Adj || !opts.read_optimize) throw std::runtime_error("First version requires -g adj -r");
        opts.create_new=false;
        if(o.prediction=="embedding") { opts.has_node_props=true; opts.prop_mode=EMBEDDED; }
        else { opts.has_node_props=false; }
        if(o.embedding=="fastrp") generate(opts,o);
        if(o.impl=="direct") predict_direct(opts,o);
        else predict(opts,o);
        rusage usage{}; getrusage(RUSAGE_SELF,&usage);
        std::cerr<<"total_s="<<elapsed(t)<<" peak_rss_mb="<<usage.ru_maxrss/1024.0<<'\n';
        return 0;
    } catch(const std::exception& e) { std::cerr<<"ERROR: "<<e.what()<<'\n'; return 1; }
