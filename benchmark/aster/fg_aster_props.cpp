/**
 * fg_aster_props.cpp — FlexoGraph property CRUD benchmark for Fig 7C-D.
 *
 * Uses WiredTiger directly (like AsterDB uses RocksDB) to benchmark
 * property storage operations on LDBC and Freebase datasets.
 *
 * Tables:
 *   table:node_props  key_format=Q, value_format=S  columns=(id,prop_value)
 *   table:edge_props  key_format=QQ, value_format=S columns=(src,dst,prop_value)
 *   index:node_props:by_value  columns=(prop_value)
 *   index:edge_props:by_value  columns=(prop_value)
 *
 * Data loaded from preprocessed .vertex and .edge files (same format as
 * AsterDB's process_property_ldbc.py / process_property_freebase.py output).
 *
 * Usage:
 *   fg_aster_props --data_dir=<path> --dataset=<ldbc|freebase>
 *                  [--db_dir=<wt_db_path>] [--cache_size=<MB>]
 *
 * Output: "Time of <op>: <N>ns" lines matching reproduce_script.sh awk patterns.
 */

#include <wiredtiger.h>

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

using namespace std;
using Clock = chrono::high_resolution_clock;

struct Config {
  string data_dir;     // directory containing .vertex and .edge files
  string dataset;      // "ldbc" or "freebase"
  string db_dir;       // WiredTiger database directory
  int cache_size_mb = 256;
  int num_ops = 10;    // number of ops for update/insert/remove (matches AsterDB)
};

static void check_wt(int ret, const char *msg)
{
  if (ret != 0) {
    cerr << msg << ": " << wiredtiger_strerror(ret) << endl;
    exit(1);
  }
}

// ── Data Loading ────────────────────────────────────────────────────────────

struct EdgeRecord {
  uint64_t src;
  uint64_t dst;
};

static void load_data(WT_SESSION *session, const Config &cfg,
                      uint64_t &max_node_id,
                      vector<uint64_t> &node_ids,
                      vector<EdgeRecord> &edges)
{
  // Determine file paths
  string vertex_file, edge_file;
  if (cfg.dataset == "ldbc") {
    vertex_file = cfg.data_dir + "/ldbc.json2.vertex";
    edge_file = cfg.data_dir + "/ldbc.json2.edge";
  } else {
    vertex_file = cfg.data_dir + "/freebase_large.json2.vertex";
    edge_file = cfg.data_dir + "/freebase_large.json2.edge";
  }

  // Load vertices
  WT_CURSOR *ncur;
  check_wt(session->open_cursor(session, "table:node_props", nullptr, nullptr, &ncur),
            "open node_props cursor");

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
    // Format: <id> <key>:<value> [<key2>:<value2> ...]
    // We only care about the first property value
    size_t sp1 = line.find(' ');
    if (sp1 == string::npos) continue;

    uint64_t id = stoull(line.substr(0, sp1));

    // Extract first property value after the colon
    size_t colon = line.find(':', sp1);
    if (colon == string::npos) continue;
    size_t val_start = colon + 1;
    size_t val_end = line.find(' ', val_start);
    string prop_value = (val_end == string::npos)
                            ? line.substr(val_start)
                            : line.substr(val_start, val_end - val_start);

    ncur->set_key(ncur, id);
    ncur->set_value(ncur, prop_value.c_str());
    int ret = ncur->insert(ncur);
    if (ret != 0 && ret != WT_DUPLICATE_KEY) {
      check_wt(ret, "insert node_props");
    }

    node_ids.push_back(id);
    if (id > max_node_id) max_node_id = id;
    vcnt++;
    if (vcnt % 1000000 == 0)
      cout << "  loaded " << vcnt << " vertices" << endl;
  }
  vf.close();
  ncur->close(ncur);
  cout << "Loaded " << vcnt << " vertices (max_id=" << max_node_id << ")" << endl;

  // Load edges
  WT_CURSOR *ecur;
  check_wt(session->open_cursor(session, "table:edge_props", nullptr, nullptr, &ecur),
            "open edge_props cursor");

  ifstream ef(edge_file);
  if (!ef.is_open()) {
    cerr << "Cannot open edge file: " << edge_file << endl;
    exit(1);
  }

  uint64_t ecnt = 0;
  while (getline(ef, line)) {
    if (line.empty()) continue;
    // Format: <src> <dst> <key>:<value>
    size_t sp1 = line.find(' ');
    if (sp1 == string::npos) continue;
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == string::npos) continue;

    uint64_t src = stoull(line.substr(0, sp1));
    uint64_t dst = stoull(line.substr(sp1 + 1, sp2 - sp1 - 1));

    size_t colon = line.find(':', sp2);
    if (colon == string::npos) continue;
    string prop_value = line.substr(colon + 1);

    ecur->set_key(ecur, src, dst);
    ecur->set_value(ecur, prop_value.c_str());
    int ret = ecur->insert(ecur);
    if (ret != 0 && ret != WT_DUPLICATE_KEY) {
      check_wt(ret, "insert edge_props");
    }

    edges.push_back({src, dst});
    ecnt++;
    if (ecnt % 1000000 == 0)
      cout << "  loaded " << ecnt << " edges" << endl;
  }
  ef.close();
  ecur->close(ecur);
  cout << "Loaded " << ecnt << " edges" << endl;
}

// ── Benchmark Operations ────────────────────────────────────────────────────

static int64_t bench_vertex_property_search(WT_SESSION *session,
                                             const string &target)
{
  WT_CURSOR *icur;
  check_wt(session->open_cursor(session, "index:node_props:by_value",
                                  nullptr, nullptr, &icur),
            "open vertex index cursor");

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

  cout << "vertex property search count: " << count << endl;
  icur->close(icur);
  return ns;
}

static int64_t bench_edge_property_search(WT_SESSION *session,
                                           const string &target)
{
  WT_CURSOR *icur;
  check_wt(session->open_cursor(session, "index:edge_props:by_value",
                                  nullptr, nullptr, &icur),
            "open edge index cursor");

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

  cout << "edge property search count: " << count << endl;
  icur->close(icur);
  return ns;
}

static int64_t bench_update_vertex_property(WT_SESSION *session,
                                             const vector<uint64_t> &node_ids,
                                             int num_ops)
{
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, "table:node_props", nullptr, nullptr, &cur),
            "open node_props cursor for update");

  mt19937_64 rng(42);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    uint64_t id = node_ids[rng() % node_ids.size()];
    cur->set_key(cur, id);
    if (cur->search(cur) != 0) continue;

    auto t0 = Clock::now();
    cur->set_key(cur, id);
    cur->set_value(cur, "new-property");
    cur->update(cur);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  cur->close(cur);
  return total_ns / num_ops;
}

static int64_t bench_update_edge_property(WT_SESSION *session,
                                           const vector<EdgeRecord> &edges,
                                           int num_ops)
{
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, "table:edge_props", nullptr, nullptr, &cur),
            "open edge_props cursor for update");

  mt19937_64 rng(42);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    cur->set_key(cur, e.src, e.dst);
    if (cur->search(cur) != 0) continue;

    auto t0 = Clock::now();
    cur->set_key(cur, e.src, e.dst);
    cur->set_value(cur, "new-label");
    cur->update(cur);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  cur->close(cur);
  return total_ns / num_ops;
}

static int64_t bench_insert_vertex_property(WT_SESSION *session,
                                             const vector<uint64_t> &node_ids,
                                             int num_ops)
{
  // "Insert" a new property = update the value to something new.
  // AsterDB: v.property('new-key', 'new-property') — adds a new key.
  // In our fixed-schema approach, this is equivalent to updating the column.
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, "table:node_props", nullptr, nullptr, &cur),
            "open node_props cursor for insert");

  mt19937_64 rng(43);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    uint64_t id = node_ids[rng() % node_ids.size()];

    auto t0 = Clock::now();
    cur->set_key(cur, id);
    cur->set_value(cur, "new-property");
    cur->update(cur);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  cur->close(cur);
  return total_ns / num_ops;
}

static int64_t bench_insert_edge_property(WT_SESSION *session,
                                           const vector<EdgeRecord> &edges,
                                           int num_ops)
{
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, "table:edge_props", nullptr, nullptr, &cur),
            "open edge_props cursor for edge insert");

  mt19937_64 rng(43);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];

    auto t0 = Clock::now();
    cur->set_key(cur, e.src, e.dst);
    cur->set_value(cur, "new-label");
    cur->update(cur);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  cur->close(cur);
  return total_ns / num_ops;
}

static int64_t bench_remove_vertex_property(WT_SESSION *session,
                                             const vector<uint64_t> &node_ids,
                                             int num_ops)
{
  // "Remove" = set property to empty string (matches AsterDB: v.property(key, ''))
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, "table:node_props", nullptr, nullptr, &cur),
            "open node_props cursor for remove");

  mt19937_64 rng(44);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    uint64_t id = node_ids[rng() % node_ids.size()];
    cur->set_key(cur, id);
    if (cur->search(cur) != 0) continue;

    auto t0 = Clock::now();
    cur->set_key(cur, id);
    cur->set_value(cur, "");
    cur->update(cur);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  cur->close(cur);
  return total_ns / num_ops;
}

static int64_t bench_remove_edge_property(WT_SESSION *session,
                                           const vector<EdgeRecord> &edges,
                                           int num_ops)
{
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, "table:edge_props", nullptr, nullptr, &cur),
            "open edge_props cursor for edge remove");

  mt19937_64 rng(44);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    cur->set_key(cur, e.src, e.dst);
    if (cur->search(cur) != 0) continue;

    auto t0 = Clock::now();
    cur->set_key(cur, e.src, e.dst);
    cur->set_value(cur, "");
    cur->update(cur);
    auto t1 = Clock::now();
    total_ns += chrono::duration_cast<chrono::nanoseconds>(t1 - t0).count();
  }
  cur->close(cur);
  return total_ns / num_ops;
}

// ── Main ────────────────────────────────────────────────────────────────────

static Config parse_args(int argc, char *argv[])
{
  Config cfg;
  for (int i = 1; i < argc; i++) {
    string arg(argv[i]);
    if (arg.rfind("--data_dir=", 0) == 0)
      cfg.data_dir = arg.substr(11);
    else if (arg.rfind("--dataset=", 0) == 0)
      cfg.dataset = arg.substr(10);
    else if (arg.rfind("--db_dir=", 0) == 0)
      cfg.db_dir = arg.substr(9);
    else if (arg.rfind("--cache_size=", 0) == 0)
      cfg.cache_size_mb = stoi(arg.substr(13));
    else if (arg.rfind("--num_ops=", 0) == 0)
      cfg.num_ops = stoi(arg.substr(10));
  }

  if (cfg.data_dir.empty() || cfg.dataset.empty()) {
    cerr << "Usage: fg_aster_props --data_dir=<path> --dataset=<ldbc|freebase>"
         << " [--db_dir=<path>] [--cache_size=<MB>] [--num_ops=<N>]" << endl;
    exit(1);
  }

  if (cfg.db_dir.empty())
    cfg.db_dir = cfg.data_dir + "/fg_props_" + cfg.dataset;

  return cfg;
}

int main(int argc, char *argv[])
{
  Config cfg = parse_args(argc, argv);

  // Determine search targets (matches AsterDB's load_with_properties.groovy)
  string vertex_target, edge_target;
  if (cfg.dataset == "freebase") {
    vertex_target = "484848485248";
    edge_target = "/american_football/football_coach/coaching_history";
  } else {
    vertex_target = "post";
    edge_target = "hasCreator";
  }

  // Create DB directory
  string mkdir_cmd = "mkdir -p " + cfg.db_dir;
  system(mkdir_cmd.c_str());

  // Open WiredTiger connection
  string conn_config = "create,cache_size=" + to_string(cfg.cache_size_mb) + "M"
                        ",statistics=(fast)";
  WT_CONNECTION *conn;
  check_wt(wiredtiger_open(cfg.db_dir.c_str(), nullptr, conn_config.c_str(), &conn),
            "wiredtiger_open");

  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session),
            "open_session");

  // Create tables and indices
  check_wt(session->create(session, "table:node_props",
                            "key_format=Q,value_format=S,"
                            "columns=(id,prop_value)"),
            "create node_props table");
  check_wt(session->create(session, "table:edge_props",
                            "key_format=QQ,value_format=S,"
                            "columns=(src,dst,prop_value)"),
            "create edge_props table");
  check_wt(session->create(session, "index:node_props:by_value",
                            "columns=(prop_value)"),
            "create node_props index");
  check_wt(session->create(session, "index:edge_props:by_value",
                            "columns=(prop_value)"),
            "create edge_props index");

  // Check if data already loaded
  WT_CURSOR *check_cur;
  check_wt(session->open_cursor(session, "table:node_props", nullptr, nullptr, &check_cur),
            "open check cursor");
  bool need_load = (check_cur->next(check_cur) != 0);
  check_cur->close(check_cur);

  uint64_t max_node_id = 0;
  vector<uint64_t> node_ids;
  vector<EdgeRecord> edges;

  if (need_load) {
    cout << "Loading data from " << cfg.data_dir << " (" << cfg.dataset << ")" << endl;
    auto t0 = Clock::now();
    load_data(session, cfg, max_node_id, node_ids, edges);
    auto t1 = Clock::now();
    double secs = chrono::duration<double>(t1 - t0).count();
    cout << "Data loaded in " << secs << "s" << endl;

    // Checkpoint to persist
    check_wt(session->checkpoint(session, nullptr), "checkpoint after load");
  } else {
    cout << "Data already loaded, collecting IDs..." << endl;
    // Collect node IDs
    WT_CURSOR *ncur;
    check_wt(session->open_cursor(session, "table:node_props", nullptr, nullptr, &ncur),
              "open node_props scan cursor");
    while (ncur->next(ncur) == 0) {
      uint64_t id;
      ncur->get_key(ncur, &id);
      node_ids.push_back(id);
      if (id > max_node_id) max_node_id = id;
    }
    ncur->close(ncur);

    // Collect edge samples (first 1M for random selection)
    WT_CURSOR *ecur;
    check_wt(session->open_cursor(session, "table:edge_props", nullptr, nullptr, &ecur),
              "open edge_props scan cursor");
    uint64_t sampled = 0;
    while (ecur->next(ecur) == 0 && sampled < 1000000) {
      uint64_t s, d;
      ecur->get_key(ecur, &s, &d);
      edges.push_back({s, d});
      sampled++;
    }
    ecur->close(ecur);

    cout << "Collected " << node_ids.size() << " node IDs, "
         << edges.size() << " edge samples" << endl;
  }

  cout << "\n=== Property CRUD Benchmark ===" << endl;
  cout << "Dataset: " << cfg.dataset << endl;
  cout << "Vertex search target: " << vertex_target << endl;
  cout << "Edge search target: " << edge_target << endl;
  cout << "Num ops (update/insert/remove): " << cfg.num_ops << endl;
  cout << endl;

  // 1. Vertex property search
  int64_t ns = bench_vertex_property_search(session, vertex_target);
  cout << "Time of vertex property search: " << ns << "ns" << endl;

  // 2. Edge property search
  ns = bench_edge_property_search(session, edge_target);
  cout << "Time of edge property search: " << ns << "ns" << endl;

  // 3. Update vertex property
  ns = bench_update_vertex_property(session, node_ids, cfg.num_ops);
  cout << "Time of update vertex property: " << ns << "ns" << endl;

  // 4. Update edge property
  ns = bench_update_edge_property(session, edges, cfg.num_ops);
  cout << "Time of update edge property: " << ns << "ns" << endl;

  // 5. Insert vertex property
  ns = bench_insert_vertex_property(session, node_ids, cfg.num_ops);
  cout << "Time of insert vertex property: " << ns << "ns" << endl;

  // 6. Insert edge property
  ns = bench_insert_edge_property(session, edges, cfg.num_ops);
  cout << "Time of insert edge property: " << ns << "ns" << endl;

  // 7. Remove vertex property
  ns = bench_remove_vertex_property(session, node_ids, cfg.num_ops);
  cout << "Time of remove vertex property: " << ns << "ns" << endl;

  // 8. Remove edge property
  ns = bench_remove_edge_property(session, edges, cfg.num_ops);
  cout << "Time of remove edge property: " << ns << "ns" << endl;

  session->close(session, nullptr);
  conn->close(conn, nullptr);
  return 0;
}
