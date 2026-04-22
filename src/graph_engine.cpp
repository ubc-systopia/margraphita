#include "graph_engine.h"

#include <stdexcept>

GraphEngine::GraphEngine(int _num_threads, const graph_opts &engine_opts)
    : num_threads(_num_threads), opts(engine_opts)
{
  // std::cout <<" GraphEngine constructor called\n\n\n" << std::endl ;
  //  check_opts_valid();
  opts.print_config("");
  if (opts.create_new)
  {
    create_new_graph();
  }
  else
  {
    open_connection();
  }
}

GraphEngine::~GraphEngine() { close_connection(); }

std::string GraphEngine::make_checkpoint()
{
  WT_SESSION *session;
  int ret = conn->open_session(conn, nullptr, nullptr, &session);
  if (ret != 0)
  {
    throw GraphException("Failed to open session for checkpoint");
  }

  try
  {
    // Generate timestamp-based checkpoint name
    auto now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm localTime = *std::localtime(&now);
    char cpt_name_clean[27];
    std::strftime(cpt_name_clean, 27, "%Y_%m_%d_%H_%M_%S", &localTime);

    // Now create the actual WiredTiger checkpoint
    char checkpoint_config[64];
    std::snprintf(checkpoint_config,
                  sizeof(checkpoint_config),
                  "name=%s",
                  cpt_name_clean);

    ret = session->checkpoint(session, checkpoint_config);
    if (ret != 0)
    {
      session->close(session, nullptr);
      throw GraphException("Failed to create checkpoint");
    }

    last_checkpoint = cpt_name_clean;
    session->close(session, nullptr);

#ifdef DEBUG
    std::cout << "Successfully created checkpoint: " << last_checkpoint
              << std::endl;
#endif
    return last_checkpoint;
  }
  catch (...)
  {
    session->close(session, nullptr);
    throw;
  }
}

GraphBase *GraphEngine::create_ro_graph_handle(std::string &checkpoint_name)
{
#ifdef DEBUG
  LOG_MSG("Making read-only graph handle");
#endif
  GraphBase *ptr;

  if (checkpoint_name.empty())
  {
    if (this->last_checkpoint.empty())
    {
      checkpoint_name = make_checkpoint();
    }
    checkpoint_name = this->last_checkpoint;
  }
  opts.checkpoint_name = checkpoint_name;
  bool saved_read_only = opts.read_only;
  opts.read_only = true;  // RO handles must have read_only=true for set_ro_num_nodes

  if (opts.type == GraphType::Adj)
    ptr = new AdjList(opts, conn);
  else if (opts.type == GraphType::SplitEKey)
    ptr = new SplitEdgeKey(opts, conn);
  else
    throw GraphException("Failed to create graph object");

  if (opts.num_edges == 0)
  {
    // The metadata table is only synced on explicit force_metadata_sync calls,
    // so it may still be 0 after bulk insertions.  Fall back to the live atomic
    // counter — an overestimate (from concurrent writers in mixed mode) only
    // affects partition granularity (slightly fewer, larger partitions), which
    // is a benign load-balancing trade-off, not a correctness issue.
    WT_ITEM metadata_item;
    ptr->get_metadata(MetadataKey::num_edges, metadata_item, nullptr);
    opts.num_edges = *((uint64_t *)metadata_item.data);
    if (opts.num_edges == 0)
      opts.num_edges = GraphBase::get_atomic_nedges();
  }

  if (checkpoint_node_count != 0)
  {
    opts.num_nodes = checkpoint_node_count;
  }
  else
  {
    switch(partition_strategy)
    {
      case PartitionStrategy::NODE_COUNT:
        opts.num_nodes = compute_nodes_and_partition(num_threads, ptr);
        break;
      case PartitionStrategy::EDGE_AWARE:
        opts.num_nodes = new_parts(num_threads, ptr, partition_scale);
        break;
      case PartitionStrategy::EDGE_AWARE_ADJLIST:
        opts.num_nodes = new_parts_from_adjlist(num_threads, partition_scale);
        break;
      case PartitionStrategy::NODE_COUNT_FINE:
        opts.num_nodes = make_min_parts(num_threads, ptr, partition_scale);
        break;
      default:
        throw GraphException("Invalid partition strategy");
    }
    checkpoint_node_count = opts.num_nodes;
    ptr->set_ro_num_nodes(opts.num_nodes);
  }

  opts.read_only = saved_read_only;  // restore original read_only setting
  return ptr;
}

GraphBase *GraphEngine::create_graph_handle()
{
  GraphBase *ptr;
  if (opts.type == GraphType::Adj)
    ptr = new AdjList(opts, conn);
  else if (opts.type == GraphType::SplitEKey)
    ptr = new SplitEdgeKey(opts, conn);
  else
    throw GraphException("Failed to create graph object");

  return ptr;
}

void GraphEngine::create_indices()
{
  WT_SESSION *sess;
  CommonUtil::open_session(conn, &sess);
  if (opts.type == GraphType::SplitEKey)
  {
    // SplitEdgeKey::create_indices(sess);
  }
  else
  {
    throw GraphException("Failed to create graph object");
  }
}

void GraphEngine::calculate_thread_offsets(bool make_edge)
{
  // Create snapshot here first?
  GraphBase *graph_stats = create_ro_graph_handle(last_checkpoint);
  //_calculate_thread_offsets(num_threads, graph_stats);
  // _calculate_thread_offsets_fast(num_threads, graph_stats);
  // if (make_edge) _calculate_thread_offsets_edge(num_threads, graph_stats);

#ifdef DEBUG
  std::cout << "Using partition strategy: " << get_strategy_name() << std::endl;
#endif

  graph_stats->close(false);
}

/**
 * This function can be used to calculate the thread offsets if we keep account
 * of what's the min and max key the threads have seen so far.
 *
 * If we don't have this metadata recorded, we can extract this by using the
 * GraphAPI calls: get_min_node, get_max_node.
 * This function only works correctly if the max_node_id is the last node_id.
 */
node_id_t GraphEngine::_calculate_exact_node_count(GraphBase *graph_stats)
{
  NodeCursor *n_cur = graph_stats->get_node_iter();

  node_id_t num_nodes{0};
  node found;
  n_cur->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    num_nodes++;
    n_cur->next(&found);
  }
  n_cur->close();
#ifdef DEBUG
  std::cout << "The number of nodes is: " << num_nodes << std::endl;
#endif
  graph_stats->set_ro_num_nodes(num_nodes);

  return num_nodes;
}

/**
 * Computes the exact node count and creates balanced thread partitions in a
 * single traversal. Each thread gets approximately equal number of nodes.
 * @param thread_max The number of threads (partitions) to create
 * @param graph_stats The graph object to traverse
 * @return The exact number of nodes in the graph
 */
node_id_t GraphEngine::compute_nodes_and_partition(int thread_max,
                                                   GraphBase *graph_stats)
{
  NodeCursor *n_cur = graph_stats->get_node_iter();
  node found;
  n_cur->next(&found);

  // Collect all node IDs in a single pass
  std::vector<node_id_t> node_ids_temp(1000000);  // preallocate for efficiency
  size_t idx = 0;
  while (found.id != OutOfBand_ID_MAX)
  {
    if (idx >= node_ids_temp.size())
    {
      node_ids_temp.resize(node_ids_temp.size() * 2);  // double the size
    }
    node_ids_temp[idx++] = found.id;
    n_cur->next(&found);
  }
  node_ids_temp.resize(idx);
  n_cur->close();

  // Save all node IDs for chunk creation
  // all_node_ids = node_ids_temp;

  node_id_t num_nodes = node_ids_temp.size();
#ifdef DEBUG
  std::cout << "The number of nodes is: " << num_nodes << std::endl;
#endif
  graph_stats->set_ro_num_nodes(num_nodes);

  // check if node_ranges needs to be populated
  if (!node_ranges.empty())
  {
    return num_nodes;  // already populated; can return
  }
  // Create balanced partitions based on actual node count
  if (num_nodes > 0 && thread_max > 0)
  {
    node_id_t nodes_per_partition =
        (num_nodes + thread_max - 1) / thread_max;  // ceil division

    // First partition starts at first node
    node_ranges.push_back(node_ids_temp[0]);

    // Create partition boundaries at every nodes_per_partition interval
    for (int i = 1; i < thread_max; i++)
    {
      node_id_t idx = i * nodes_per_partition;
      if (idx < node_ids_temp.size())
      {
        node_ranges.push_back(node_ids_temp[idx]);
      }
    }

    // Last partition boundary is the last node
    node_ranges.push_back(node_ids_temp.back());

#ifdef DEBUG
    std::cout << "Balanced partitioning: " << num_nodes << " nodes across "
              << thread_max << " threads (" << nodes_per_partition
              << " nodes/partition)" << std::endl;
    for (int i = 0; i < thread_max; i++)
    {
      auto x = get_key_range(i);
      std::cout << "thread " << i << " [" << x.start << ", " << x.end << "]\n";
    }
#endif
  }

  return num_nodes;
}

/**
 * Creates balanced thread partitions based on outdegrees (edge work).
 * Each partition gets approximately equal sum of outdegrees.
 * This implementation uses a node iterator since the graph is (constructed to
 * be) read optimized. That implementation will be much faster than using edgeor
 * out nbd itrerators.
 * @param thread_max The number of threads (partitions) to create
 * @param graph_stats The graph object to traverse
 * @param mini_part_scale Scale factor to create more partitions than threads
 * for better balancing. We will create (thread_max * mini_part_scale)
 * partitions.
 * @return The exact number of nodes in the graph
 */
node_id_t GraphEngine::new_parts(int thread_max,
                                 GraphBase *graph_stats,
                                 int mini_part_scale)
{
  NodeCursor *node_cursor = graph_stats->get_node_iter();
  node found_node;

  // Get total edges from opts
  node_id_t total_edges = opts.num_edges;
  node_id_t edges_per_thread = total_edges / (thread_max * mini_part_scale);
  if (edges_per_thread == 0)
  {
    edges_per_thread = 1;  // at least 1 edge per partition
  }

#ifdef DEBUG
  std::cout << "Creating degree-balanced partitions with target "
            << edges_per_thread << " edges per thread" << std::endl;
#endif

  // Collect all node IDs and create partitions based on outdegree
  std::vector<node_id_t> node_ids_temp;
  node_ids_temp.reserve(1000000);

  node_cursor->next(&found_node);

  node_id_t accumulated_edges = 0;
  node_id_t num_nodes = 0;
  int partitions_created = 0;
  int min_parts = get_min_partitions();
  node_ranges.clear();

  // First partition starts at first node
  if (found_node.id != OutOfBand_ID_MAX)
  {
    node_ranges.push_back(found_node.id);
    partitions_created = 1;
  }

  while (found_node.id != OutOfBand_ID_MAX)
  {
    node_ids_temp.push_back(found_node.id);
    num_nodes++;

    // Get outdegree for this node
    accumulated_edges += found_node.out_degree;

    // Move to next node
    node_cursor->next(&found_node);

    // Create a new partition if:
    // 1. We've accumulated enough edges for this partition, AND
    // 2. There are more nodes to process
    if (accumulated_edges >= edges_per_thread &&
        found_node.id != OutOfBand_ID_MAX)
    {
      // Start new partition at the next node
      node_ranges.push_back(found_node.id);
      partitions_created++;
      accumulated_edges = 0;  // Reset for next partition
    }
  }

  // Last partition boundary is the last node
  if (!node_ids_temp.empty())
  {
    node_ranges.push_back(node_ids_temp.back());
  }

  node_cursor->close();
  delete node_cursor;

  // Save all node IDs for chunk creation
  all_node_ids = node_ids_temp;
#ifdef DEBUG
  std::cout << "Degree-balanced partitioning: " << num_nodes << " nodes across "
            << thread_max << " threads (target " << edges_per_thread
            << " edges/partition, created " << partitions_created
            << " partitions, min required: " << min_parts << ")" << std::endl;
  if (partitions_created < min_parts)
  {
    std::cout << "WARNING: Created fewer partitions (" << partitions_created
              << ") than minimum (" << min_parts << ")" << std::endl;
  }
  for (int i = 0; i < partitions_created; i++)
  {
    auto x = get_key_range(i);
    std::cout << "partition " << i << " [" << x.start << ", " << x.end << "]\n";
  }
#endif

  graph_stats->set_ro_num_nodes(num_nodes);
  return num_nodes;
}

/**
 * Degree-balanced partitioning that reads degrees directly from the OUT_ADJLIST
 * table.  Unlike new_parts() which relies on the node cursor's out_degree
 * (broken when MK_NEDGES is defined and read_optimize is false), this function
 * reads the raw adjlist value where the degree is always stored as the first
 * sizeof(degree_t) bytes.
 */
node_id_t GraphEngine::new_parts_from_adjlist(int thread_max,
                                              int mini_part_scale)
{
  WT_SESSION *part_session = nullptr;
  conn->open_session(conn, nullptr, nullptr, &part_session);
  WT_CURSOR *cursor = nullptr;
  GraphBase::_get_table_cursor(OUT_ADJLIST, &cursor, part_session,
                               false, false, opts.checkpoint_name);

  node_id_t total_edges = opts.num_edges;
  node_id_t edges_per_thread = total_edges / (thread_max * mini_part_scale);
  if (edges_per_thread == 0)
    edges_per_thread = 1;

  std::vector<node_id_t> node_ids_temp;
  node_ids_temp.reserve(1000000);

  node_id_t accumulated_edges = 0;
  uint64_t total_edges_counted = 0;
  node_id_t num_nodes = 0;
  int partitions_created = 0;
  node_ranges.clear();

  while (cursor->next(cursor) == 0)
  {
    node_id_t node_id;
    CommonUtil::get_key(cursor, &node_id);

    WT_ITEM item;
    cursor->get_value(cursor, &item);
    degree_t degree = 0;
    if (item.size >= sizeof(degree_t))
      memcpy(&degree, item.data, sizeof(degree_t));

    node_ids_temp.push_back(node_id);
    num_nodes++;
    total_edges_counted += degree;

    if (partitions_created == 0)
    {
      node_ranges.push_back(node_id);
      partitions_created = 1;
    }

    accumulated_edges += degree;

    if (accumulated_edges >= edges_per_thread)
    {
      // Peek ahead — boundary goes at the *next* node
      int peek = cursor->next(cursor);
      if (peek == 0)
      {
        node_id_t next_id;
        CommonUtil::get_key(cursor, &next_id);
        node_ranges.push_back(next_id);
        partitions_created++;
        accumulated_edges = 0;

        // Process the peeked node
        WT_ITEM next_item;
        cursor->get_value(cursor, &next_item);
        degree_t next_degree = 0;
        if (next_item.size >= sizeof(degree_t))
          memcpy(&next_degree, next_item.data, sizeof(degree_t));
        node_ids_temp.push_back(next_id);
        num_nodes++;
        total_edges_counted += next_degree;
        accumulated_edges += next_degree;
      }
      else
      {
        break;  // no more nodes
      }
    }
  }

  if (!node_ids_temp.empty())
    node_ranges.push_back(node_ids_temp.back());

  cursor->close(cursor);
  part_session->close(part_session, nullptr);

  all_node_ids = node_ids_temp;

  // For undirected graphs, each logical edge is stored in both directions in
  // OUT_ADJLIST, so the degree sum = 2 * logical edge count.
  checkpoint_edge_count = opts.is_directed ? total_edges_counted
                                           : total_edges_counted / 2;

  std::cout << "[new_parts_from_adjlist] " << num_nodes << " nodes, "
            << checkpoint_edge_count << " edges, "
            << partitions_created << " partitions (target edges/part="
            << edges_per_thread << ")" << std::endl;

  return num_nodes;
}

node_id_t GraphEngine::make_min_parts(int thread_max,
                                      GraphBase *graph_stats,
                                      int mini_part_scale)
{
  NodeCursor *node_cursor = graph_stats->get_node_iter();
  node found_node;

  // Calculate target number of partitions
  int target_partitions = thread_max * mini_part_scale;

#ifdef DEBUG
  std::cout << "Creating node-count balanced partitions with target "
            << target_partitions << " partitions" << std::endl;
#endif

  // Collect all node IDs and create partitions based on node count
  std::vector<node_id_t> node_ids_temp;
  node_ids_temp.reserve(1000000);

  node_cursor->next(&found_node);

  node_id_t num_nodes = 0;
  int min_parts = get_min_partitions();
  node_ranges.clear();

  // First pass: collect all node IDs
  while (found_node.id != OutOfBand_ID_MAX)
  {
    node_ids_temp.push_back(found_node.id);
    num_nodes++;
    node_cursor->next(&found_node);
  }

  // Calculate nodes per partition
  node_id_t nodes_per_partition =
      (num_nodes + target_partitions - 1) / target_partitions;

  // Second pass: create partition boundaries at fixed intervals
  int partitions_created = 0;

  if (!node_ids_temp.empty())
  {
    // First partition starts at first node
    node_ranges.push_back(node_ids_temp[0]);
    partitions_created = 1;

    // Create partition boundaries at regular node intervals
    for (int i = 1; i < target_partitions; i++)
    {
      node_id_t idx = i * nodes_per_partition;
      if (idx < node_ids_temp.size())
      {
        node_ranges.push_back(node_ids_temp[idx]);
        partitions_created++;
      }
    }
  }

  // Last partition boundary is the last node
  if (!node_ids_temp.empty())
  {
    node_ranges.push_back(node_ids_temp.back());
  }

  node_cursor->close();
  delete node_cursor;

  // Save all node IDs for chunk creation
  all_node_ids = node_ids_temp;

#ifdef DEBUG
  std::cout << "Node-count balanced partitioning: " << num_nodes
            << " nodes across " << target_partitions << " partitions ("
            << nodes_per_partition << " nodes/partition, created "
            << partitions_created << " partitions, min required: " << min_parts
            << ")" << std::endl;
  if (partitions_created < min_parts)
  {
    std::cout << "WARNING: Created fewer partitions (" << partitions_created
              << ") than minimum (" << min_parts << ")" << std::endl;
  }
  for (int i = 0; i < partitions_created; i++)
  {
    auto x = get_key_range(i);
    std::cout << "partition " << i << " [" << x.start << ", " << x.end << "]\n";
  }
#endif

  graph_stats->set_ro_num_nodes(num_nodes);
  return num_nodes;
}

void GraphEngine::_calculate_thread_offsets_edge(int thread_max,
                                                 GraphBase *graph_stats)
{
  edge_ranges.clear();
  node_id_t num_edges = graph_stats->get_num_edges();
  node_id_t per_partition_edge =
      (num_edges / thread_max) +
      ((num_edges % thread_max) != 0);  // ceil division

  EdgeCursor *e_cur = graph_stats->get_edge_iter();
  edge found_edge;
  e_cur->next(&found_edge);
  edge_id_t i = 0;
  //    CommonUtil::dump_edge(found_edge);
  while (found_edge.src_id != OutOfBand_ID_MAX &&
         found_edge.dst_id != OutOfBand_ID_MAX)
  {
    if (i % per_partition_edge == 0)
    {
      edge_ranges.push_back(found_edge);
      //            std::cout << "Edge: " << found_edge.src_id << ","
      //                      << found_edge.dst_id << " at offset i: " <<
      //                      i
      //                      << std::endl;
    }
    if (i == num_edges - 1)
    {
      edge_ranges.push_back(found_edge);
      i++;
      break;
    }
    e_cur->next(&found_edge);
    i++;
  }
  assert(num_edges == i);
  //    std::cout << "The edge boundaries are: " << std::endl;
  //    for (auto x : edge_ranges)
  //    {
  //        std::cout << x.src_id << " " << x.dst_id << std::endl;
  //    }
  e_cur->close();
}

void GraphEngine::check_opts_valid() const
{
  if (opts.db_name.empty())
  {
    throw GraphException("DB name is empty");
  }
  if (opts.db_dir.empty())
  {
    throw GraphException("DB dir is empty");
  }
  if (opts.conn_config.empty())
  {
    throw GraphException("Connection config is empty");
  }
  if (opts.dataset.empty())
  {
    throw GraphException("Dataset is empty");
  }
  if (opts.checkpoint_name.empty())
  {
    std::cerr << "Checkpoint name is empty" << std::endl;
  }
  if (opts.stat_log.empty())
  {
    std::cerr << "Stat log is empty" << std::endl;
  }
  if (opts.num_nodes == 0)
  {
    std::cerr << "Number of nodes is zero" << std::endl;
  }
  if (opts.num_edges == 0)
  {
    std::cerr << "Number of edges is zero" << std::endl;
  }
  if (opts.num_threads == 0)
  {
    std::cerr << "Number of threads is zero" << std::endl;
  }

  if (opts.type == GraphType::Adj || opts.type == GraphType::SplitEKey)
  { /*no-op*/
  }
  else
  {
    throw GraphException("Graph type is not set");
  }
}

void GraphEngine::create_new_graph()
{
  std::string dirname = opts.db_dir + "/" + opts.db_name;
  std::cout << "Creating new graph at " << dirname << std::endl;
  CommonUtil::create_dir(dirname);
  if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                  opts.stat_log,
                                  opts.conn_config,
                                  &conn) < 0)
  {
    throw GraphException("Cannot open connection to new DB");
  };

  if (opts.type == GraphType::Adj)
  {
    AdjList::create_wt_tables(opts, conn);
  }
  else if (opts.type == GraphType::SplitEKey)
  {
    SplitEdgeKey::create_wt_tables(opts, conn);
  }
  else
  {
    throw GraphException("Failed to create graph object");
  }

  GraphBase::create_metadata_table(opts, conn);
}

void GraphEngine::open_connection()
{
  std::string dirname = opts.db_dir + "/" + opts.db_name;
  if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                  opts.stat_log,
                                  opts.conn_config,
                                  &conn) < 0)
  {
    throw GraphException("Cannot open connection");
  };
}

void GraphEngine::close_connection()
{
  // CommonUtil::close_connection(conn);
  if (conn != nullptr)
  {
    conn->close(conn, nullptr);
    conn = nullptr;
  }
}

WT_CONNECTION *GraphEngine::get_connection() { return conn; }

void GraphEngine::close_graph() { close_connection(); }

key_range GraphEngine::get_key_range(int thread_id)
{
  key_range to_return{};
  // assign so that there is no overlap
  to_return.start = node_ranges[thread_id];

  // Check if we're at the last partition
  // Note: node_ranges has (num_partitions + 1) elements
  if (thread_id < (int)node_ranges.size() - 2)
  {
    // Not the last partition: end at the node before the next partition starts
    to_return.end = node_ranges[thread_id + 1] - 1;
  }
  else if (thread_id == (int)node_ranges.size() - 2)
  {
    // Last partition: end at the actual last node (inclusive)
    to_return.end = node_ranges[thread_id + 1];
  }
  else
  {
    // thread_id is out of bounds for available partitions
    throw std::runtime_error("get_key_range: thread_id " +
                             std::to_string(thread_id) +
                             " is out of bounds (node_ranges.size() = " +
                             std::to_string(node_ranges.size()) + ")");
  }
  return to_return;
}

edge_range GraphEngine::get_edge_range(int thread_id)
{
  edge_range to_return{};
  to_return.start.src_id = edge_ranges[thread_id].src_id;
  to_return.start.dst_id = edge_ranges[thread_id].dst_id;
  if (thread_id < num_threads - 1)
  {
    to_return.end.src_id = edge_ranges[thread_id + 1].src_id;
    to_return.end.dst_id = edge_ranges[thread_id + 1].dst_id;
  }
  else
  {
    to_return.end.src_id = OutOfBand_ID_MAX;
    to_return.end.dst_id = OutOfBand_ID_MAX;
  }
  return to_return;
}

std::vector<key_range> GraphEngine::get_work_chunks(int chunk_size)
{
  std::vector<key_range> chunks;

  if (all_node_ids.empty())
  {
    throw GraphException(
        "all_node_ids is empty. Call calculate_thread_offsets() or new_parts() "
        "first.");
  }

  // Create many small chunks based on the actual node IDs for fine-grained
  // dynamic scheduling
  for (size_t i = 0; i < all_node_ids.size(); i += chunk_size)
  {
    size_t end_idx = std::min(i + chunk_size, all_node_ids.size());
    key_range chunk;
    chunk.start = all_node_ids[i];
    chunk.end = all_node_ids[end_idx - 1];
    chunks.push_back(chunk);
  }

#ifdef DEBUG
  std::cout << "Created " << chunks.size()
            << " work chunks with chunk_size=" << chunk_size
            << " from all_node_ids (total nodes: " << all_node_ids.size() << ")"
            << std::endl;
#endif

  return chunks;
}

GraphEngine::GraphEngine() {}
// Created by puneet on 27/03/25.
//
void GraphEngine::force_metadata_sync() { return; }
