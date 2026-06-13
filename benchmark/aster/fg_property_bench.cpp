/**
 * fg_property_bench.cpp -- FlexoGraph property CRUD benchmark.
 *
 * Bulk-loads data from .vertex/.edge files, builds WT index tables,
 * then benchmarks 8 property operations (search/update/insert/remove
 * for both vertex and edge properties).
 *
 * Usage:
 *   fg_property_bench -g <adj|split_ekey> -p <db_dir> -m <db_name>
 *                     --data_dir=<path> --dataset=<ldbc|freebase>
 *                     [--cache_size=<MB>] [--num_ops=<N>]
 *
 * Output: "Time of <op>: <N>ns" lines matching reproduce_script.sh awk.
 */

#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "graph_engine.h"
#include "prop_bench_loader.h"
#include "prop_bench_ops.h"

using namespace std;

// -- Config -------------------------------------------------------------------

struct Config {
  string data_dir;
  string dataset;      // "ldbc" or "freebase"
  string db_dir;
  string db_name;
  string graph_type_str = "adj";
  int cache_size_mb = 256;
  int num_ops = 10;
  bool no_index = true;   // use full scan instead of index for search
};

static Config parse_config(int argc, char *argv[])
{
  Config cfg;
  for (int i = 1; i < argc; i++) {
    string a(argv[i]);
    if (a.rfind("--data_dir=", 0) == 0) cfg.data_dir = a.substr(11);
    else if (a.rfind("--dataset=", 0) == 0) cfg.dataset = a.substr(10);
    else if (a.rfind("--cache_size=", 0) == 0) cfg.cache_size_mb = stoi(a.substr(13));
    else if (a.rfind("--num_ops=", 0) == 0) cfg.num_ops = stoi(a.substr(10));
    else if (a == "-g" && i + 1 < argc) cfg.graph_type_str = argv[++i];
    else if (a == "-p" && i + 1 < argc) cfg.db_dir = argv[++i];
    else if (a == "-m" && i + 1 < argc) cfg.db_name = argv[++i];
    else if (a == "--no_index") cfg.no_index = true;
    else if (a == "--use_index") cfg.no_index = false;
  }
  if (cfg.data_dir.empty() || cfg.dataset.empty()) {
    cerr << "Usage: fg_property_bench -g <adj|split_ekey> -p <db_dir> -m <db_name>\n"
         << "       --data_dir=<path> --dataset=<ldbc|freebase>\n"
         << "       [--cache_size=<MB>] [--num_ops=<N>] [--use_index]\n";
    exit(1);
  }
  if (cfg.db_dir.empty()) cfg.db_dir = cfg.data_dir;
  if (cfg.db_name.empty()) cfg.db_name = "fg_props_" + cfg.dataset;
  return cfg;
}

// -- Main ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
  Config cfg = parse_config(argc, argv);

  GraphType gtype = GraphType::Adj;
  if (cfg.graph_type_str == "split_ekey" || cfg.graph_type_str == "splitekey")
    gtype = GraphType::SplitEKey;

  // Search targets (matches AsterDB's load_with_properties.groovy)
  string vertex_target, edge_target;
  if (cfg.dataset == "freebase") {
    vertex_target = "484848485248";
    edge_target = "/american_football/football_coach/coaching_history";
  } else {
    vertex_target = "post";
    edge_target = "hasCreator";
  }

  // Data file paths
  string vertex_file, edge_file;
  if (cfg.dataset == "ldbc") {
    vertex_file = cfg.data_dir + "/ldbc.json2.vertex";
    edge_file = cfg.data_dir + "/ldbc.json2.edge";
  } else {
    vertex_file = cfg.data_dir + "/freebase_large.json2.vertex";
    edge_file = cfg.data_dir + "/freebase_large.json2.edge";
  }

  string conn_config = "cache_size=" + to_string(cfg.cache_size_mb) + "M";
  string db_path = cfg.db_dir + "/" + cfg.db_name;

  // Check if DB already exists
  bool db_exists = false;
  {
    ifstream test(db_path + "/WiredTiger");
    db_exists = test.good();
  }

  vector<node_id_t> node_ids;
  vector<EdgeRecord> edges;
  node_id_t max_node_id = 0;

  // Bulk load if DB doesn't exist
  if (!db_exists) {
    // Parse data files once
    vector<VertexRecord> verts;
    vector<ParsedEdge> pedges;
    set<node_id_t> all_node_ids;
    parse_vertex_file(vertex_file, verts, all_node_ids);
    parse_edge_file(edge_file, pedges, all_node_ids);

    auto t0 = chrono::high_resolution_clock::now();
    if (gtype == GraphType::SplitEKey) {
      bulk_load_split_ekey(db_path, conn_config, /*is_directed=*/false,
                            verts, pedges, all_node_ids, max_node_id);
    } else {
      bulk_load_adj(db_path, conn_config, /*is_directed=*/false,
                     verts, pedges, all_node_ids, max_node_id);
    }
    auto t1 = chrono::high_resolution_clock::now();
    cerr << "Bulk load done in "
         << chrono::duration<double>(t1 - t0).count() << "s" << endl;

    // Populate node_ids and edges from parsed data
    for (node_id_t id : all_node_ids)
      node_ids.push_back(id);
    for (auto &e : pedges) {
      if (e.src == e.dst) continue;
      edges.push_back({e.src, e.dst});
    }

    db_exists = true;
  }

  // Open GraphEngine on the (now existing) DB
  graph_opts opts;
  opts.create_new = false;
  opts.is_directed = false;
  opts.read_optimize = true;
  opts.has_node_props = true;
  opts.has_edge_props = true;
  opts.prop_mode = EMBEDDED;
  opts.type = gtype;
  opts.db_name = cfg.db_name;
  opts.db_dir = cfg.db_dir;
  opts.conn_config = conn_config;
  opts.num_nodes = node_ids.size();
  opts.num_edges = edges.size();

  GraphEngine engine(1, opts);
  GraphBase *gp = engine.create_graph_handle();
  WT_CONNECTION *conn = engine.get_connection();

  // If we didn't just bulk load, collect IDs from existing DB
  if (node_ids.empty()) {
    cerr << "Data already loaded, collecting IDs..." << endl;
    collect_existing_data(gp, node_ids, edges, max_node_id);

    // Verify index tables exist
    WT_SESSION *chk_session;
    check_wt(conn->open_session(conn, nullptr, nullptr, &chk_session),
             "open check session");
    WT_CURSOR *chk_cur;
    int ret = chk_session->open_cursor(chk_session, "table:fg_vprop_idx",
                                       nullptr, nullptr, &chk_cur);
    if (ret != 0) {
      cerr << "WARNING: Index tables not found. Re-run with fresh DB." << endl;
      chk_session->close(chk_session, nullptr);
      return 1;
    }
    chk_cur->close(chk_cur);
    chk_session->close(chk_session, nullptr);
  }

  // Run benchmarks
  cerr << "\n=== Property CRUD Benchmark ===" << endl;
  cerr << "Dataset: " << cfg.dataset << endl;
  cerr << "Graph type: " << cfg.graph_type_str << endl;
  cerr << "Vertex search target: " << vertex_target << endl;
  cerr << "Edge search target: " << edge_target << endl;
  cerr << "Num ops (update/insert/remove): " << cfg.num_ops << endl;
  cerr << "Search mode: " << (cfg.no_index ? "full scan" : "indexed") << endl;
  cerr << endl;

  bool is_split_ekey = (gtype == GraphType::SplitEKey);
  string engine_name = is_split_ekey ? "flexograph-ekey" : "flexograph-adj";

  // Output CSV: engine,script_name,value_us
  // Script names match graph-baselines groovy filenames for parse_property_crud_data.py
  auto emit = [&](const char *script, int64_t ns) {
    cout << engine_name << "," << script << "," << fixed << setprecision(2)
         << (ns / 1000.0) << endl;
  };

  int64_t ns;

  if (cfg.no_index) {
    ns = bench_vertex_property_scan(conn, vertex_target, is_split_ekey);
  } else {
    ns = bench_vertex_property_search(conn, vertex_target);
  }
  emit("node-property-search.groovy", ns);

  if (cfg.no_index) {
    ns = bench_edge_property_scan(conn, edge_target, is_split_ekey);
  } else {
    ns = bench_edge_property_search(conn, edge_target);
  }
  emit("edge-specific-property-search.groovy", ns);

  ns = bench_update_vertex_property(gp, node_ids, cfg.num_ops);
  emit("update-node-property.groovy", ns);

  ns = bench_update_edge_property(gp, edges, cfg.num_ops);
  emit("update-edge-property.groovy", ns);

  ns = bench_insert_vertex_property(gp, node_ids, cfg.num_ops);
  emit("insert-node-property.groovy", ns);

  ns = bench_insert_edge_property(gp, edges, cfg.num_ops);
  emit("insert-edge-property.groovy", ns);

  ns = bench_remove_vertex_property(gp, node_ids, cfg.num_ops);
  emit("delete-node-property.groovy", ns);

  ns = bench_remove_edge_property(gp, edges, cfg.num_ops);
  emit("delete-edge-property.groovy", ns);

  engine.close_graph();
  return 0;
}
