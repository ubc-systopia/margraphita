#pragma once
/**
 * prop_bench_loader.h -- Bulk loaders for the property CRUD benchmark.
 *
 * Provides fast bulk loading for both adj and split_ekey graph types,
 * bypassing the slow per-element GraphEngine transaction path.
 */

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "common_util.h"
#include "graph_engine.h"

// -- Data structures ----------------------------------------------------------

struct VertexRecord {
  node_id_t id;
  std::string prop;
};

struct ParsedEdge {
  node_id_t src, dst;
  std::string prop;
};

struct EdgeRecord {
  uint64_t src;
  uint64_t dst;
};

// -- WT helper ----------------------------------------------------------------

static inline void check_wt(int ret, const char *msg)
{
  if (ret != 0) {
    std::cerr << msg << ": " << wiredtiger_strerror(ret) << std::endl;
    exit(1);
  }
}

// -- File parsers -------------------------------------------------------------

static void parse_vertex_file(const std::string &vertex_file,
                               std::vector<VertexRecord> &verts,
                               std::set<node_id_t> &all_node_ids)
{
  std::ifstream vf(vertex_file);
  if (!vf.is_open()) {
    std::cerr << "Cannot open vertex file: " << vertex_file << std::endl;
    exit(1);
  }
  std::string line;
  while (getline(vf, line)) {
    if (line.empty()) continue;
    size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos) continue;
    node_id_t id = stoull(line.substr(0, sp1));
    size_t colon = line.find(':', sp1);
    if (colon == std::string::npos) continue;
    size_t val_start = colon + 1;
    size_t val_end = line.find(' ', val_start);
    std::string prop_value = (val_end == std::string::npos)
                                 ? line.substr(val_start)
                                 : line.substr(val_start, val_end - val_start);
    verts.push_back({id, prop_value});
    all_node_ids.insert(id);
  }
  std::cerr << "Parsed " << verts.size() << " vertices" << std::endl;
}

static void parse_edge_file(const std::string &edge_file,
                             std::vector<ParsedEdge> &pedges,
                             std::set<node_id_t> &all_node_ids)
{
  std::ifstream ef(edge_file);
  if (!ef.is_open()) {
    std::cerr << "Cannot open edge file: " << edge_file << std::endl;
    exit(1);
  }
  std::string line;
  while (getline(ef, line)) {
    if (line.empty()) continue;
    size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos) continue;
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) continue;
    node_id_t src = stoull(line.substr(0, sp1));
    node_id_t dst = stoull(line.substr(sp1 + 1, sp2 - sp1 - 1));
    size_t colon = line.find(':', sp2);
    if (colon == std::string::npos) continue;
    std::string prop_value = line.substr(colon + 1);
    pedges.push_back({src, dst, prop_value});
    all_node_ids.insert(src);
    all_node_ids.insert(dst);
  }
  std::cerr << "Parsed " << pedges.size() << " edges" << std::endl;
}

// -- Index table builder (works with any graph type) --------------------------

static void build_index_tables_from_parsed(WT_CONNECTION *conn,
                                            const std::vector<VertexRecord> &verts,
                                            const std::vector<ParsedEdge> &pedges)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open index session");

  check_wt(session->create(session, "table:fg_vprop_idx",
      "key_format=S,value_format=Q,columns=(prop_value,node_id)"),
      "create fg_vprop_idx");
  check_wt(session->create(session, "table:fg_eprop_idx",
      "key_format=S,value_format=QQ,columns=(prop_value,src,dst)"),
      "create fg_eprop_idx");

  WT_CURSOR *vcur;
  check_wt(session->open_cursor(session, "table:fg_vprop_idx", nullptr, nullptr, &vcur),
           "open vprop_idx cursor");
  for (auto &v : verts) {
    vcur->set_key(vcur, v.prop.c_str());
    vcur->set_value(vcur, (uint64_t)v.id);
    vcur->insert(vcur);
  }
  vcur->close(vcur);

  WT_CURSOR *ecur;
  check_wt(session->open_cursor(session, "table:fg_eprop_idx", nullptr, nullptr, &ecur),
           "open eprop_idx cursor");
  for (auto &e : pedges) {
    ecur->set_key(ecur, e.prop.c_str());
    ecur->set_value(ecur, (uint64_t)e.src, (uint64_t)e.dst);
    ecur->insert(ecur);
  }
  ecur->close(ecur);

  check_wt(session->checkpoint(session, nullptr), "checkpoint indices");
  session->close(session, nullptr);
  std::cerr << "Index tables built: " << verts.size() << " vprop, "
            << pedges.size() << " eprop" << std::endl;
}

// -- Bulk loader: split_ekey --------------------------------------------------

static void bulk_load_split_ekey(const std::string &db_path,
                                  const std::string &conn_config,
                                  bool is_directed,
                                  const std::vector<VertexRecord> &verts,
                                  const std::vector<ParsedEdge> &pedges,
                                  const std::set<node_id_t> &all_node_ids,
                                  node_id_t &max_node_id)
{
  // Compute degrees
  std::map<node_id_t, std::string> vprop_map;
  for (auto &v : verts) vprop_map[v.id] = v.prop;

  std::map<node_id_t, degree_t> out_deg, in_deg;
  for (auto &e : pedges) {
    out_deg[e.src]++;
    if (is_directed)
      in_deg[e.dst]++;
    else
      out_deg[e.dst]++;
  }

  // Build sorted bulk entries
  struct BulkEntry {
    bool is_node;
    node_id_t id1, id2;
    std::vector<uint8_t> value;
  };
  std::vector<BulkEntry> entries;
  entries.reserve(all_node_ids.size() + pedges.size() * 2);

  max_node_id = 0;
  for (node_id_t id : all_node_ids) {
    if (id > max_node_id) max_node_id = id;
    degree_t ind = is_directed ? in_deg[id] : out_deg[id];
    degree_t outd = out_deg[id];
    auto it = vprop_map.find(id);
    size_t prop_sz = (it != vprop_map.end()) ? it->second.size() : 0;
    size_t total = sizeof(degree_t) * 2 + prop_sz;
    std::vector<uint8_t> val(total);
    memcpy(val.data(), &ind, sizeof(degree_t));
    memcpy(val.data() + sizeof(degree_t), &outd, sizeof(degree_t));
    if (prop_sz > 0)
      memcpy(val.data() + sizeof(degree_t) * 2, it->second.data(), prop_sz);
    entries.push_back({true, id, 0, std::move(val)});
  }

  for (auto &e : pedges) {
    std::vector<uint8_t> val(e.prop.begin(), e.prop.end());
    entries.push_back({false, e.src, e.dst, val});
    if (!is_directed)
      entries.push_back({false, e.dst, e.src, val});
  }

  std::sort(entries.begin(), entries.end(),
            [](const BulkEntry &a, const BulkEntry &b) {
              if (a.id1 != b.id1) return a.id1 < b.id1;
              if (a.is_node != b.is_node) return a.is_node;
              return a.id2 < b.id2;
            });

  // Deduplicate: keep last occurrence for same key
  {
    std::vector<BulkEntry> deduped;
    deduped.reserve(entries.size());
    for (size_t i = 0; i < entries.size(); i++) {
      if (i + 1 < entries.size() && entries[i].id1 == entries[i + 1].id1 &&
          entries[i].is_node == entries[i + 1].is_node &&
          entries[i].id2 == entries[i + 1].id2)
        continue;
      deduped.push_back(std::move(entries[i]));
    }
    entries = std::move(deduped);
  }

  std::cerr << "Bulk inserting " << entries.size() << " entries (split_ekey)..."
            << std::endl;

  // Open raw WT connection, create tables, bulk insert
  CommonUtil::create_dir(db_path);
  WT_CONNECTION *conn;
  std::string full_config = "create," + conn_config;
  check_wt(wiredtiger_open(db_path.c_str(), nullptr, full_config.c_str(), &conn),
           "open WT connection for bulk load");

  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open session");

  check_wt(session->create(session, "table:edge_out",
      "key_format=uu,value_format=u,"
      "columns=(src,dst,attr),"
      "leaf_page_max=64KB,internal_page_max=16KB,"
      "memory_page_max=32MB,split_pct=100"),
      "create edge_out table");

  check_wt(session->create(session, "table:metadata",
      "key_format=I,value_format=u"),
      "create metadata table");
  session->close(session, nullptr);

  // Bulk cursor insert
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open bulk session");
  WT_CURSOR *cursor;
  check_wt(session->open_cursor(session, "table:edge_out", nullptr, "bulk", &cursor),
           "open bulk cursor");

  uint64_t cnt = 0;
  for (auto &ent : entries) {
    if (ent.is_node)
      CommonUtil::ekey_set_node_key(cursor, ent.id1);
    else
      CommonUtil::ekey_set_edge_key(cursor, ent.id1, ent.id2);
    WT_ITEM item;
    item.data = ent.value.data();
    item.size = ent.value.size();
    cursor->set_value(cursor, &item);
    check_wt(cursor->insert(cursor), "bulk insert");
    cnt++;
    if (cnt % 100000 == 0)
      std::cerr << "  inserted " << cnt << " / " << entries.size() << std::endl;
  }
  cursor->close(cursor);

  // Write metadata
  WT_CURSOR *mcur;
  check_wt(session->open_cursor(session, "table:metadata", nullptr, nullptr, &mcur),
           "open metadata cursor");
  node_id_t num_nodes = all_node_ids.size();
  node_id_t num_edges = pedges.size();
  node_id_t min_id = *all_node_ids.begin();
  auto write_meta = [&](int key, void *val, size_t sz) {
    mcur->set_key(mcur, key);
    WT_ITEM it;
    it.data = val;
    it.size = sz;
    mcur->set_value(mcur, &it);
    mcur->insert(mcur);
  };
  write_meta(MetadataKey::min_node_id, &min_id, sizeof(min_id));
  write_meta(MetadataKey::max_node_id, &max_node_id, sizeof(max_node_id));
  write_meta(MetadataKey::num_nodes, &num_nodes, sizeof(num_nodes));
  write_meta(MetadataKey::num_edges, &num_edges, sizeof(num_edges));
  mcur->close(mcur);

  // Build index tables
  build_index_tables_from_parsed(conn, verts, pedges);

  check_wt(session->checkpoint(session, nullptr), "final checkpoint");
  session->close(session, nullptr);
  conn->close(conn, nullptr);
  std::cerr << "Bulk load complete (split_ekey): " << cnt << " entries" << std::endl;
}

// -- Bulk loader: adj ---------------------------------------------------------
//
// Adj tables:
//   table:adjlistout  key_format=u, value_format=u  -> [degree | id0 id1 ... ]
//   table:node_props  key_format=u, value_format=u  -> [prop_blob]
//   table:edge        key_format=uu, value_format=u -> [prop_blob]  (if has_edge_props)
//   table:metadata    key_format=I, value_format=u
//
// All keys use byte-swapped big-endian (CommonUtil::set_key), so ascending
// node_id order gives correct WT sort order. Bulk cursors require sorted keys.

static void bulk_load_adj(const std::string &db_path,
                           const std::string &conn_config,
                           bool is_directed,
                           const std::vector<VertexRecord> &verts,
                           const std::vector<ParsedEdge> &pedges,
                           const std::set<node_id_t> &all_node_ids,
                           node_id_t &max_node_id)
{
  // Build adjacency lists and edge properties in memory
  // For undirected: store both directions in out_adjlist
  std::map<node_id_t, std::vector<node_id_t>> adjlists;
  for (node_id_t id : all_node_ids)
    adjlists[id];  // ensure every node has an entry (even degree-0)

  for (auto &e : pedges) {
    if (e.src == e.dst) continue;  // skip self-loops
    adjlists[e.src].push_back(e.dst);
    if (!is_directed)
      adjlists[e.dst].push_back(e.src);
  }

  // Sort each adjacency list for consistency
  for (auto &[id, list] : adjlists)
    std::sort(list.begin(), list.end());

  // Vertex properties map
  std::map<node_id_t, std::string> vprop_map;
  for (auto &v : verts) vprop_map[v.id] = v.prop;

  // Edge properties: key=(src,dst) -> prop
  // For undirected, store both directions
  struct EdgeKey {
    node_id_t src, dst;
    bool operator<(const EdgeKey &o) const {
      return src < o.src || (src == o.src && dst < o.dst);
    }
  };
  std::map<EdgeKey, std::string> eprop_map;
  for (auto &e : pedges) {
    if (e.src == e.dst) continue;
    eprop_map[{e.src, e.dst}] = e.prop;
    if (!is_directed)
      eprop_map[{e.dst, e.src}] = e.prop;
  }

  max_node_id = all_node_ids.empty() ? 0 : *all_node_ids.rbegin();

  std::cerr << "Bulk loading adj: " << all_node_ids.size() << " nodes, "
            << eprop_map.size() << " edge entries..." << std::endl;

  // Open raw WT connection
  CommonUtil::create_dir(db_path);
  WT_CONNECTION *conn;
  std::string full_config = "create," + conn_config;
  check_wt(wiredtiger_open(db_path.c_str(), nullptr, full_config.c_str(), &conn),
           "open WT connection for adj bulk load");

  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open session");

  // Create tables (matching AdjList::create_wt_tables for undirected, read_optimize, EMBEDDED)
  std::string adjlist_config =
      "key_format=u,value_format=u,"
      "leaf_page_max=64KB,internal_page_max=16KB,"
      "memory_page_max=10MB,split_pct=90";
  check_wt(session->create(session, "table:adjlistout", adjlist_config.c_str()),
           "create adjlistout");

  if (is_directed)
    check_wt(session->create(session, "table:adjlistin", adjlist_config.c_str()),
             "create adjlistin");

  // Node table (read_optimize, undirected: key=u, value=I for out_degree)
  // With MK_NEDGES:
  std::string node_config = is_directed
      ? "key_format=u,value_format=II,columns=(id,in_degree,out_degree)"
      : "key_format=u,value_format=I,columns=(id,out_degree)";
  check_wt(session->create(session, "table:node", node_config.c_str()),
           "create node table");

  // Edge table (key=uu src,dst; value=u for props blob)
  check_wt(session->create(session, "table:edge",
      "key_format=uu,value_format=u,columns=(src,dst,props)"),
      "create edge table");

  // Node props table
  check_wt(session->create(session, "table:node_props",
      "key_format=u,value_format=u,leaf_page_max=64KB"),
      "create node_props table");

  // Metadata table
  check_wt(session->create(session, "table:metadata",
      "key_format=I,value_format=u"),
      "create metadata table");

  session->close(session, nullptr);

  // --- Bulk insert all tables ---
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open bulk session");

  // 1. adjlistout (sorted by node_id)
  {
    WT_CURSOR *cur;
    check_wt(session->open_cursor(session, "table:adjlistout", nullptr, "bulk", &cur),
             "open adjlistout bulk cursor");
    for (auto &[id, list] : adjlists) {
      CommonUtil::set_key(cur, id);
      size_t buf_sz = sizeof(degree_t) + list.size() * sizeof(node_id_t);
      std::vector<uint8_t> buf(buf_sz);
      degree_t deg = static_cast<degree_t>(list.size());
      memcpy(buf.data(), &deg, sizeof(degree_t));
      if (!list.empty())
        memcpy(buf.data() + sizeof(degree_t), list.data(),
               list.size() * sizeof(node_id_t));
      WT_ITEM item = {.data = buf.data(), .size = buf_sz};
      cur->set_value(cur, &item);
      check_wt(cur->insert(cur), "adjlistout bulk insert");
    }
    cur->close(cur);
    std::cerr << "  adjlistout: " << adjlists.size() << " entries" << std::endl;
  }

  // 2. node table (sorted by node_id)
  {
    WT_CURSOR *cur;
    check_wt(session->open_cursor(session, "table:node", nullptr, "bulk", &cur),
             "open node bulk cursor");
    for (auto &[id, list] : adjlists) {
      CommonUtil::set_key(cur, id);
      if (is_directed) {
        // would need in-degree too; for now undirected only
        cur->set_value(cur, (uint32_t)list.size(), (uint32_t)0);
      } else {
        cur->set_value(cur, (uint32_t)list.size());
      }
      check_wt(cur->insert(cur), "node bulk insert");
    }
    cur->close(cur);
    std::cerr << "  node table: " << adjlists.size() << " entries" << std::endl;
  }

  // 3. node_props (sorted by node_id)
  {
    WT_CURSOR *cur;
    check_wt(session->open_cursor(session, "table:node_props", nullptr, "bulk", &cur),
             "open node_props bulk cursor");
    // vprop_map is sorted (it's a std::map)
    // But we need entries for ALL nodes, not just those with props.
    // Only insert nodes that have properties.
    for (auto &[id, prop] : vprop_map) {
      CommonUtil::set_key(cur, id);
      WT_ITEM item;
      item.data = prop.data();
      item.size = prop.size();
      cur->set_value(cur, &item);
      check_wt(cur->insert(cur), "node_props bulk insert");
    }
    cur->close(cur);
    std::cerr << "  node_props: " << vprop_map.size() << " entries" << std::endl;
  }

  // 4. edge table (sorted by (src, dst))
  {
    WT_CURSOR *cur;
    check_wt(session->open_cursor(session, "table:edge", nullptr, "bulk", &cur),
             "open edge bulk cursor");
    // eprop_map is sorted by (src, dst) since EdgeKey has operator<
    for (auto &[ek, prop] : eprop_map) {
      CommonUtil::set_key(cur, ek.src, ek.dst);
      WT_ITEM item;
      item.data = prop.data();
      item.size = prop.size();
      cur->set_value(cur, &item);
      check_wt(cur->insert(cur), "edge bulk insert");
    }
    cur->close(cur);
    std::cerr << "  edge table: " << eprop_map.size() << " entries" << std::endl;
  }

  // 5. metadata
  {
    WT_CURSOR *mcur;
    check_wt(session->open_cursor(session, "table:metadata", nullptr, nullptr, &mcur),
             "open metadata cursor");
    node_id_t num_nodes = all_node_ids.size();
    node_id_t num_edges = pedges.size();
    node_id_t min_id = *all_node_ids.begin();
    auto write_meta = [&](int key, void *val, size_t sz) {
      mcur->set_key(mcur, key);
      WT_ITEM it;
      it.data = val;
      it.size = sz;
      mcur->set_value(mcur, &it);
      mcur->insert(mcur);
    };
    write_meta(MetadataKey::min_node_id, &min_id, sizeof(min_id));
    write_meta(MetadataKey::max_node_id, &max_node_id, sizeof(max_node_id));
    write_meta(MetadataKey::num_nodes, &num_nodes, sizeof(num_nodes));
    write_meta(MetadataKey::num_edges, &num_edges, sizeof(num_edges));
    mcur->close(mcur);
  }

  // 6. Property index tables
  build_index_tables_from_parsed(conn, verts, pedges);

  check_wt(session->checkpoint(session, nullptr), "final checkpoint");
  session->close(session, nullptr);
  conn->close(conn, nullptr);
  std::cerr << "Bulk load complete (adj): " << all_node_ids.size() << " nodes, "
            << eprop_map.size() << " edges" << std::endl;
}

// -- Collect existing data (when DB already loaded) ---------------------------

static void collect_existing_data(GraphBase *gp,
                                  std::vector<node_id_t> &node_ids,
                                  std::vector<EdgeRecord> &edges,
                                  node_id_t &max_node_id)
{
  max_node_id = gp->get_max_node_id();
  node_ids.reserve(max_node_id);
  for (node_id_t i = 0; i <= max_node_id; i++) {
    if (gp->has_node(i)) node_ids.push_back(i);
  }

  for (size_t ni = 0; ni < node_ids.size() && edges.size() < 1000000; ni++) {
    node_id_t src = node_ids[ni];
    std::vector<node_id_t> nbrs = gp->get_out_nodes_id(src);
    for (node_id_t dst : nbrs) {
      edges.push_back({src, dst});
      if (edges.size() >= 1000000) break;
    }
  }
  std::cerr << "Collected " << node_ids.size() << " node IDs, "
            << edges.size() << " edge samples" << std::endl;
}
