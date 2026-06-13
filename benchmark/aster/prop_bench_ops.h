#pragma once
/**
 * prop_bench_ops.h -- Benchmark operations for property CRUD.
 *
 * Each function returns the average latency in nanoseconds for one operation.
 */

#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "graph.h"
#include "prop_bench_loader.h"  // for EdgeRecord, check_wt

using Clock = std::chrono::high_resolution_clock;

// -- Search ops (use WT index tables directly) --------------------------------

static int64_t bench_vertex_property_search(WT_CONNECTION *conn,
                                            const std::string &target)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open search session");
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
  int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  std::cerr << "vertex property search count: " << count << std::endl;
  icur->close(icur);
  session->close(session, nullptr);
  return ns;
}

static int64_t bench_edge_property_search(WT_CONNECTION *conn,
                                          const std::string &target)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open search session");
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
  int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  std::cerr << "edge property search count: " << count << std::endl;
  icur->close(icur);
  session->close(session, nullptr);
  return ns;
}

// -- Scan-based search ops (full table scan, no index) ------------------------

static int64_t bench_vertex_property_scan(WT_CONNECTION *conn,
                                          const std::string &target,
                                          bool is_split_ekey)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open scan session");

  const char *table = is_split_ekey ? "table:edge_out" : "table:node_props";
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, table, nullptr, nullptr, &cur),
           "open scan cursor");

  uint64_t count = 0;
  auto t0 = Clock::now();

  if (is_split_ekey) {
    // split_ekey: node sentinels have dst=0 (second key item).
    // Value layout: [in_degree(4B)][out_degree(4B)][prop_bytes...]
    while (cur->next(cur) == 0) {
      WT_ITEM key1, key2;
      cur->get_key(cur, &key1, &key2);
      // Node sentinel: key2 size is sizeof(node_id_t) with value 0
      if (key2.size != sizeof(node_id_t)) continue;
      node_id_t dst_val;
      memcpy(&dst_val, key2.data, sizeof(node_id_t));
      if (dst_val != 0) continue;  // edge, not node sentinel
      WT_ITEM val;
      cur->get_value(cur, &val);
      size_t hdr = sizeof(degree_t) * 2;
      if (val.size <= hdr) continue;
      std::string prop((const char *)val.data + hdr, val.size - hdr);
      if (prop == target) count++;
    }
  } else {
    // adj: node_props table, key=u (node_id), value=u (prop blob)
    while (cur->next(cur) == 0) {
      WT_ITEM val;
      cur->get_value(cur, &val);
      if (val.size == 0) continue;
      if (val.size == target.size() &&
          memcmp(val.data, target.data(), target.size()) == 0)
        count++;
    }
  }

  auto t1 = Clock::now();
  int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  std::cerr << "vertex property scan count: " << count << std::endl;
  cur->close(cur);
  session->close(session, nullptr);
  return ns;
}

static int64_t bench_edge_property_scan(WT_CONNECTION *conn,
                                        const std::string &target,
                                        bool is_split_ekey)
{
  WT_SESSION *session;
  check_wt(conn->open_session(conn, nullptr, nullptr, &session), "open scan session");

  const char *table = is_split_ekey ? "table:edge_out" : "table:edge";
  WT_CURSOR *cur;
  check_wt(session->open_cursor(session, table, nullptr, nullptr, &cur),
           "open scan cursor");

  uint64_t count = 0;
  auto t0 = Clock::now();

  if (is_split_ekey) {
    // split_ekey: edges have dst != 0. Value is the prop blob directly.
    while (cur->next(cur) == 0) {
      WT_ITEM key1, key2;
      cur->get_key(cur, &key1, &key2);
      if (key2.size != sizeof(node_id_t)) continue;
      node_id_t dst_val;
      memcpy(&dst_val, key2.data, sizeof(node_id_t));
      if (dst_val == 0) continue;  // node sentinel, skip
      WT_ITEM val;
      cur->get_value(cur, &val);
      if (val.size == 0) continue;
      if (val.size == target.size() &&
          memcmp(val.data, target.data(), target.size()) == 0)
        count++;
    }
  } else {
    // adj: edge table, key=(src,dst), value=u (prop blob)
    while (cur->next(cur) == 0) {
      WT_ITEM val;
      cur->get_value(cur, &val);
      if (val.size == 0) continue;
      if (val.size == target.size() &&
          memcmp(val.data, target.data(), target.size()) == 0)
        count++;
    }
  }

  auto t1 = Clock::now();
  int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();

  std::cerr << "edge property scan count: " << count << std::endl;
  cur->close(cur);
  session->close(session, nullptr);
  return ns;
}

// -- Mutation ops (use GraphEngine API) ---------------------------------------

static int64_t bench_update_vertex_property(GraphBase *gp,
                                            const std::vector<node_id_t> &node_ids,
                                            int num_ops)
{
  std::mt19937_64 rng(42);
  const std::string new_val = "new-property";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    node_id_t id = node_ids[rng() % node_ids.size()];
    auto t0 = Clock::now();
    gp->set_node_properties(id, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_update_edge_property(GraphBase *gp,
                                          const std::vector<EdgeRecord> &edges,
                                          int num_ops)
{
  std::mt19937_64 rng(42);
  const std::string new_val = "new-label";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    auto t0 = Clock::now();
    gp->set_edge_properties(e.src, e.dst, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_insert_vertex_property(GraphBase *gp,
                                            const std::vector<node_id_t> &node_ids,
                                            int num_ops)
{
  std::mt19937_64 rng(43);
  const std::string new_val = "inserted-property";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    node_id_t id = node_ids[rng() % node_ids.size()];
    auto t0 = Clock::now();
    gp->set_node_properties(id, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_insert_edge_property(GraphBase *gp,
                                          const std::vector<EdgeRecord> &edges,
                                          int num_ops)
{
  std::mt19937_64 rng(43);
  const std::string new_val = "inserted-label";
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    auto t0 = Clock::now();
    gp->set_edge_properties(e.src, e.dst, (const uint8_t *)new_val.data(), new_val.size());
    auto t1 = Clock::now();
    total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_remove_vertex_property(GraphBase *gp,
                                            const std::vector<node_id_t> &node_ids,
                                            int num_ops)
{
  std::mt19937_64 rng(44);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    node_id_t id = node_ids[rng() % node_ids.size()];
    auto t0 = Clock::now();
    gp->set_node_properties(id, (const uint8_t *)"", 0);
    auto t1 = Clock::now();
    total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}

static int64_t bench_remove_edge_property(GraphBase *gp,
                                          const std::vector<EdgeRecord> &edges,
                                          int num_ops)
{
  std::mt19937_64 rng(44);
  int64_t total_ns = 0;
  for (int i = 0; i < num_ops; i++) {
    auto &e = edges[rng() % edges.size()];
    auto t0 = Clock::now();
    gp->set_edge_properties(e.src, e.dst, (const uint8_t *)"", 0);
    auto t1 = Clock::now();
    total_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
  }
  return total_ns / num_ops;
}
