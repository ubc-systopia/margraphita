/**
 * fg_property_bench.cpp -- FlexoGraph property CRUD benchmark.
 *
 * Uses GraphEngine in EMBEDDED mode + separate WT index tables on the same
 * connection (paralleling AsterDB's createIndex / NebulaGraph's tag index).
 *
 * Data loaded from preprocessed .vertex and .edge files (same format as
 * AsterDB's process_property_ldbc.py / process_property_freebase.py output).
 *
 * Usage:
 *   fg_property_bench -g <adj|split_ekey> -p <db_dir> -m <db_name>
 *                     --data_dir=<path> --dataset=<ldbc|freebase>
 *                     [--cache_size=<MB>] [--num_ops=<N>]
 *
 * Output: "Time of <op>: <N>ns" lines matching reproduce_script.sh awk.
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "graph_engine.h"

using namespace std;
using Clock = chrono::high_resolution_clock;

// -- Config -------------------------------------------------------------------

struct Config {
  string data_dir;
  string dataset;      // "ldbc" or "freebase"
  string db_dir;
  string db_name;
  string graph_type_str = "adj";   // "adj" or "split_ekey"
  int cache_size_mb = 256;
  int num_ops = 10;
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
  }
  if (cfg.data_dir.empty() || cfg.dataset.empty()) {
    cerr << "Usage: fg_property_bench -g <adj|split_ekey> -p <db_dir> -m <db_name>\n"
         << "       --data_dir=<path> --dataset=<ldbc|freebase>\n"
         << "       [--cache_size=<MB>] [--num_ops=<N>]\n";
    exit(1);
  }
  if (cfg.db_dir.empty()) cfg.db_dir = cfg.data_dir;
  if (cfg.db_name.empty()) cfg.db_name = "fg_props_" + cfg.dataset;
  return cfg;
}

// -- Data structures ----------------------------------------------------------

struct EdgeRecord {
  uint64_t src;
  uint64_t dst;
};

// -- WT helper ----------------------------------------------------------------

static void check_wt(int ret, const char *msg)
{
  if (ret != 0) {
    cerr << msg << ": " << wiredtiger_strerror(ret) << endl;
    exit(1);
  }
}

// -- Load property data into GraphEngine + build index tables -----------------

static void load_vertex_props(GraphBase *gp, const string &vertex_file,
                              vector<node_id_t> &node_ids, node_id_t &max_node_id)
{
  ifstream vf(vertex_file);
  if (!vf.is_open()) {
    cerr << "Cannot open vertex file: " << vertex_file << endl;
    exit(1);
  }

  string line;
  uint64_t vcnt = 0;
  max_node_id = 0;
  while (getline(vf, line)) {
    if (line.empty()) continue;
    // Format: <id> <key>:<value> [...]
    size_t sp1 = line.find(' ');
    if (sp1 == string::npos) continue;
    node_id_t id = stoull(line.substr(0, sp1));

    size_t colon = line.find(':', sp1);
    if (colon == string::npos) continue;
    size_t val_start = colon + 1;
    size_t val_end = line.find(' ', val_start);
    string prop_value = (val_end == string::npos)
                            ? line.substr(val_start)
                            : line.substr(val_start, val_end - val_start);

    // Ensure node exists
    if (!gp->has_node(id)) {
      node n;
      n.id = id;
      gp->add_node(n, false);
    }

    // Store property as EMBEDDED blob
    gp->set_node_properties(id, (const uint8_t *)prop_value.data(), prop_value.size());

    node_ids.push_back(id);
    if (id > max_node_id) max_node_id = id;
    vcnt++;
    if (vcnt % 1000000 == 0)
      cerr << "  loaded " << vcnt << " vertices" << endl;
  }
  cerr << "Loaded " << vcnt << " vertices (max_id=" << max_node_id << ")" << endl;
}

static void load_edge_props(GraphBase *gp, const string &edge_file,
                            vector<EdgeRecord> &edges)
{
  ifstream ef(edge_file);
  if (!ef.is_open()) {
    cerr << "Cannot open edge file: " << edge_file << endl;
    exit(1);
  }

  string line;
  uint64_t ecnt = 0;
  while (getline(ef, line)) {
    if (line.empty()) continue;
    // Format: <src> <dst> <key>:<value>
    size_t sp1 = line.find(' ');
    if (sp1 == string::npos) continue;
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == string::npos) continue;

    node_id_t src = stoull(line.substr(0, sp1));
    node_id_t dst = stoull(line.substr(sp1 + 1, sp2 - sp1 - 1));

    size_t colon = line.find(':', sp2);
    if (colon == string::npos) continue;
    string prop_value = line.substr(colon + 1);

    // Ensure both nodes exist
    if (!gp->has_node(src)) {
      node n; n.id = src;
      gp->add_node(n, false);
    }
    if (!gp->has_node(dst)) {
      node n; n.id = dst;
      gp->add_node(n, false);
    }

    // Ensure edge exists
    edge e;
    e.src_id = src;
    e.dst_id = dst;
    gp->add_edge(e, false);

    // Store edge property as EMBEDDED blob
    gp->set_edge_properties(src, dst, (const uint8_t *)prop_value.data(), prop_value.size());

    edges.push_back({src, dst});
    ecnt++;
    if (ecnt % 1000000 == 0)
      cerr << "  loaded " << ecnt << " edges" << endl;
  }
  cerr << "Loaded " << ecnt << " edges" << endl;
}

static void build_index_tables(WT_CONNECTION *conn,
                               GraphBase *gp,
                               const vector<node_id_t> &node_ids,
                               const vector<EdgeRecord> &edges)
{
  WT_SESSION *idx_session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &idx_session),
           "open index session");

  // Create vertex property index table
  check_wt(idx_session->create(idx_session, "table:fg_vprop_idx",
                               "key_format=S,value_format=Q,"
                               "columns=(prop_value,node_id)"),
           "create fg_vprop_idx");

  // Create edge property index table
  check_wt(idx_session->create(idx_session, "table:fg_eprop_idx",
                               "key_format=S,value_format=QQ,"
                               "columns=(prop_value,src,dst)"),
           "create fg_eprop_idx");

  // Populate vertex index
  WT_CURSOR *vcur;
  check_wt(idx_session->open_cursor(idx_session, "table:fg_vprop_idx",
                                     nullptr, nullptr, &vcur),
           "open vprop_idx cursor");

  for (node_id_t id : node_ids) {
    prop_blob pb = gp->get_node_properties(id);
    if (pb.data && pb.size > 0) {
      string val((const char *)pb.data, pb.size);
      vcur->set_key(vcur, val.c_str());
      vcur->set_value(vcur, (uint64_t)id);
      vcur->insert(vcur);
    }
  }
  vcur->close(vcur);

  // Populate edge index
  WT_CURSOR *ecur;
  check_wt(idx_session->open_cursor(idx_session, "table:fg_eprop_idx",
                                     nullptr, nullptr, &ecur),
           "open eprop_idx cursor");

  for (auto &e : edges) {
    prop_blob pb = gp->get_edge_properties(e.src, e.dst);
    if (pb.data && pb.size > 0) {
      string val((const char *)pb.data, pb.size);
      ecur->set_key(ecur, val.c_str());
      ecur->set_value(ecur, (uint64_t)e.src, (uint64_t)e.dst);
      ecur->insert(ecur);
    }
  }
  ecur->close(ecur);

  check_wt(idx_session->checkpoint(idx_session, nullptr), "checkpoint indices");
  idx_session->close(idx_session, nullptr);
  cerr << "Index tables built and checkpointed" << endl;
}

// -- Collect existing data (when already loaded) ------------------------------

static void collect_existing_data(GraphBase *gp,
                                  vector<node_id_t> &node_ids,
                                  vector<EdgeRecord> &edges,
                                  node_id_t &max_node_id)
{
  max_node_id = gp->get_max_node_id();
  node_ids.reserve(max_node_id);
  for (node_id_t i = 0; i <= max_node_id; i++) {
    if (gp->has_node(i)) node_ids.push_back(i);
  }

  // Collect edge samples from first N node's adjacency lists
  for (size_t ni = 0; ni < node_ids.size() && edges.size() < 1000000; ni++) {
    node_id_t src = node_ids[ni];
    vector<node_id_t> nbrs = gp->get_out_nodes_id(src);
    for (node_id_t dst : nbrs) {
      edges.push_back({src, dst});
      if (edges.size() >= 1000000) break;
    }
  }
  cerr << "Collected " << node_ids.size() << " node IDs, "
       << edges.size() << " edge samples" << endl;
}

// -- Benchmark ops ------------------------------------------------------------

static int64_t bench_vertex_property_search(WT_CONNECTION *conn,
                                            const string &target)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session),
           "open search session");
  WT_CURSOR *icur;
  check_wt(session->open_cursor(session, "table:fg_vprop_idx",
                                nullptr, nullptr, &icur),
           "open vprop_idx cursor");

  auto t0 = Clock::now();
  icur->set_key(icur, target.c_str());
  int exact = 0;
  int ret = icur->search_near(icur, &exact);
  uint64_t count = 0;
  if (ret == 0) {
    if (exact < 0) ret = icur->next(icur);
    while (ret == 0) {
      const char *val;
      icur->get_key(icur, &val);
      if (strcmp(val, target.c_str()) != 0) break;
      count++;
      ret = icur->next(icur);
    }
  }
  auto t1 = Clock::now();
  int64_t ns = chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();

  cerr << "vertex property search count: " << count << endl;
  icur->close(icur);
  session->close(session, nullptr);
  return ns;
}

static int64_t bench_edge_property_search(WT_CONNECTION *conn,
                                          const string &target)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session),
           "open search session");
  WT_CURSOR *icur;
  check_wt(session->open_cursor(session, "table:fg_eprop_idx",
                                nullptr, nullptr, &icur),
           "open eprop_idx cursor");

  auto t0 = Clock::now();
  icur->set_key(icur, target.c_str());
  int exact = 0;
  int ret = icur->search_near(icur, &exact);
  uint64_t count = 0;
  if (ret == 0) {
    if (exact < 0) ret = icur->next(icur);
    while (ret == 0) {
      const char *val;
      icur->get_key(icur, &val);
      if (strcmp(val, target.c_str()) != 0) break;
      count++;
      ret = icur->next(icur);
    }
  }
  auto t1 = Clock::now();
  int64_t ns = chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();

  cerr << "edge property search count: " << count << endl;
  icur->close(icur);
  session->close(session, nullptr);
  return ns;
}

static int64_t bench_update_vertex_property(GraphBase *gp,
                                            const vector<node_id_t> &node_ids,
                                            int num_ops)
{
  mt19937_64 rng(42);
  const string new_val = "new-property";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    node_id_t id = node_ids[rng() % node_ids.size()];
    auto t0 = Clock::now();
    gp->set_node_properties(id, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_update_edge_property(GraphBase *gp,
                                          const vector<EdgeRecord> &edges,
                                          int num_ops)
{
  mt19937_64 rng(42);
  const string new_val = "new-label";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    auto t0 = Clock::now();
    gp->set_edge_properties(e.src, e.dst, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_insert_vertex_property(GraphBase *gp,
                                            const vector<node_id_t> &node_ids,
                                            int num_ops)
{
  mt19937_64 rng(43);
  const string new_val = "inserted-property";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    node_id_t id = node_ids[rng() % node_ids.size()];
    auto t0 = Clock::now();
    gp->set_node_properties(id, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_insert_edge_property(GraphBase *gp,
                                          const vector<EdgeRecord> &edges,
                                          int num_ops)
{
  mt19937_64 rng(43);
  const string new_val = "inserted-label";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    auto t0 = Clock::now();
    gp->set_edge_properties(e.src, e.dst, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_remove_vertex_property(GraphBase *gp,
                                            const vector<node_id_t> &node_ids,
                                            int num_ops)
{
  mt19937_64 rng(44);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    node_id_t id = node_ids[rng() % node_ids.size()];
    auto t0 = Clock::now();
    gp->set_node_properties(id, (const uint8_t *)"", 0);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_remove_edge_property(GraphBase *gp,
                                          const vector<EdgeRecord> &edges,
                                          int num_ops)
{
  mt19937_64 rng(44);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    auto t0 = Clock::now();
    gp->set_edge_properties(e.src, e.dst, (const uint8_t *)"", 0);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

// -- Main ---------------------------------------------------------------------

int main(int argc, char *argv[])
{
  Config cfg = parse_config(argc, argv);

  // Determine graph type
  GraphType gtype = GraphType::Adj;
  if (cfg.graph_type_str == "split_ekey" || cfg.graph_type_str == "splitekey")
    gtype = GraphType::SplitEKey;

  // Determine search targets (matches AsterDB's load_with_properties.groovy)
  string vertex_target, edge_target;
  if (cfg.dataset == "freebase") {
    vertex_target = "484848485248";
    edge_target = "/american_football/football_coach/coaching_history";
  } else {
    vertex_target = "post";
    edge_target = "hasCreator";
  }

  // Determine file paths
  string vertex_file, edge_file;
  if (cfg.dataset == "ldbc") {
    vertex_file = cfg.data_dir + "/ldbc.json2.vertex";
    edge_file = cfg.data_dir + "/ldbc.json2.edge";
  } else {
    vertex_file = cfg.data_dir + "/freebase_large.json2.vertex";
    edge_file = cfg.data_dir + "/freebase_large.json2.edge";
  }

  // Open GraphEngine with EMBEDDED property mode
  graph_opts opts;
  opts.create_new = true;
  opts.is_directed = false;
  opts.read_optimize = true;
  opts.has_node_props = true;
  opts.has_edge_props = true;
  opts.prop_mode = EMBEDDED;
  opts.type = gtype;
  opts.db_name = cfg.db_name;
  opts.db_dir = cfg.db_dir;
  opts.conn_config = "cache_size=" + to_string(cfg.cache_size_mb) + "M";
  opts.stat_log = "./";

  // Check if DB already exists
  string db_path = cfg.db_dir + "/" + cfg.db_name;
  bool db_exists = false;
  {
    ifstream test(db_path + "/WiredTiger");
    db_exists = test.good();
  }
  if (db_exists) opts.create_new = false;

  GraphEngine engine(1, opts);
  GraphBase *gp = engine.create_graph_handle();

  vector<node_id_t> node_ids;
  vector<EdgeRecord> edges;
  node_id_t max_node_id = 0;

  WT_CONNECTION *conn = engine.get_connection();

  if (!db_exists) {
    cerr << "Loading data from " << cfg.data_dir << " (" << cfg.dataset << ")" << endl;
    auto t0 = Clock::now();
    load_vertex_props(gp, vertex_file, node_ids, max_node_id);
    load_edge_props(gp, edge_file, edges);
    auto t1 = Clock::now();
    double secs = chrono::duration<double>(t1 - t0).count();
    cerr << "Data loaded in " << secs << "s" << endl;

    // Build index tables
    build_index_tables(conn, gp, node_ids, edges);
  } else {
    cerr << "Data already loaded, collecting IDs..." << endl;
    collect_existing_data(gp, node_ids, edges, max_node_id);

    // Check if index tables exist, build if not
    WT_SESSION *chk_session;
    check_wt(conn->open_session(conn, nullptr, nullptr, &chk_session),
             "open check session");
    WT_CURSOR *chk_cur;
    int ret = chk_session->open_cursor(chk_session, "table:fg_vprop_idx",
                                       nullptr, nullptr, &chk_cur);
    if (ret != 0) {
      cerr << "Index tables not found, rebuilding..." << endl;
      chk_session->close(chk_session, nullptr);
      build_index_tables(conn, gp, node_ids, edges);
    } else {
      chk_cur->close(chk_cur);
      chk_session->close(chk_session, nullptr);
    }
  }

  cerr << "\n=== Property CRUD Benchmark ===" << endl;
  cerr << "Dataset: " << cfg.dataset << endl;
  cerr << "Graph type: " << cfg.graph_type_str << endl;
  cerr << "Vertex search target: " << vertex_target << endl;
  cerr << "Edge search target: " << edge_target << endl;
  cerr << "Num ops (update/insert/remove): " << cfg.num_ops << endl;
  cerr << endl;

  int64_t ns;

  // 1. Vertex property search
  ns = bench_vertex_property_search(conn, vertex_target);
  cout << "Time of vertex property search: " << ns << "ns" << endl;

  // 2. Edge property search
  ns = bench_edge_property_search(conn, edge_target);
  cout << "Time of edge property search: " << ns << "ns" << endl;

  // 3. Update vertex property
  ns = bench_update_vertex_property(gp, node_ids, cfg.num_ops);
  cout << "Time of update vertex property: " << ns << "ns" << endl;

  // 4. Update edge property
  ns = bench_update_edge_property(gp, edges, cfg.num_ops);
  cout << "Time of update edge property: " << ns << "ns" << endl;

  // 5. Insert vertex property
  ns = bench_insert_vertex_property(gp, node_ids, cfg.num_ops);
  cout << "Time of insert vertex property: " << ns << "ns" << endl;

  // 6. Insert edge property
  ns = bench_insert_edge_property(gp, edges, cfg.num_ops);
  cout << "Time of insert edge property: " << ns << "ns" << endl;

  // 7. Remove vertex property
  ns = bench_remove_vertex_property(gp, node_ids, cfg.num_ops);
  cout << "Time of remove vertex property: " << ns << "ns" << endl;

  // 8. Remove edge property
  ns = bench_remove_edge_property(gp, edges, cfg.num_ops);
  cout << "Time of remove edge property: " << ns << "ns" << endl;

  engine.close_graph();
  return 0;
}
