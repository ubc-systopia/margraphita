#include "graph_engine.h"

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
  // graph_opts new_opts = opts; //Do we even need to do this?

  // Determine which checkpoint to use
  if (checkpoint_name.empty())
  {
    if (this->last_checkpoint.empty())
    {
      checkpoint_name = make_checkpoint();
    }
    checkpoint_name = this->last_checkpoint;
  }
  opts.checkpoint_name = checkpoint_name;

#ifdef DEBUG
  // std::cout << "Creating read-only handle for checkpoint: " <<
  // checkpoint_name
  //           << " (overestimated) atomic counts: nnodes=" <<
  //           new_opts.num_nodes
  //           << " nedges=" << new_opts.num_edges << std::endl;
#endif

  if (opts.type == GraphType::Adj)
    ptr = new AdjList(opts, conn);
  else if (opts.type == GraphType::SplitEKey)
    ptr = new SplitEdgeKey(opts, conn);
  else
    throw GraphException("Failed to create graph object");

  // node_id_t max_node_id = ptr->get_max_node_id();
  // node_id_t min_node_id = ptr->get_min_node_id();

  // opts.num_nodes = _calculate_exact_node_count(ptr);
  //  node_id_t another_count = compute_nodes_and_partition(num_threads, ptr);
  if (checkpoint_node_count != 0)
  {
    opts.num_nodes = checkpoint_node_count;
  }
  else
  {
    opts.num_nodes = compute_nodes_and_partition(num_threads, ptr);
  }

  // if (another_count != opts.num_nodes)
  // {
  //   throw GraphException("Node count mismatch");
  // }

#ifdef DEBUG
  // std::cout << "Min node count in checkpoint " << target_checkpoint
  //           << " is: " << min_node_id << std::endl;
  // std::cout << "Max node count in checkpoint " << target_checkpoint
  //           << " is: " << max_node_id << std::endl;
#endif

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
  _calculate_thread_offsets_fast(num_threads, graph_stats);
  if (make_edge) _calculate_thread_offsets_edge(num_threads, graph_stats);
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
 * This function does not traverse the node list to find thread offsets. Instead
 * it uses the max and min node IDs from the metadata table, to calculate the
 * thread offsets.
 * @param thread_max The number of threads (partitions) to create the offsets
 * @param graph_stats The graph object to calculate the offsets from
 */
void GraphEngine::_calculate_thread_offsets_fast(int thread_max,
                                                 GraphBase *graph_stats)
{
  node_ranges.clear();
  node_id_t min_node = graph_stats->get_min_node_id();
  node_id_t max_node = graph_stats->get_max_node_id();

  node_id_t per_partition_nodes = (max_node - min_node) / thread_max;

  for (int i = 0; i <= thread_max; ++i)
  {
    node_ranges.push_back(min_node + i * per_partition_nodes);
  }
  // add the last node
  node_ranges.back() = max_node;
  #ifdef DEBUG
  std::cout << "The number of nodes is: " << num_nodes << std::endl;
  std::cout << "The number of partitions is: " << node_ranges.size()
            << std::endl;
  std::cout << "The min node is: " << min_node
            << " and the max node is: " << max_node << std::endl;
  for (auto x : node_ranges)
  {
    std::cout << x << std::endl;
  }
  std::cout << "printing the key ranges" << std::endl;
  for (int i = 0; i < num_threads; i++)
  {
    auto x = get_key_range(i);
    std::cout << "thread " << i << " [" << x.start << ", " << x.end << "]\n";
  }
  #endif
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
  std::vector<node_id_t> all_node_ids(1000000);  // preallocate for efficiency
  size_t idx = 0;
  while (found.id != OutOfBand_ID_MAX)
  {
    if (idx >= all_node_ids.size())
    {
      all_node_ids.resize(all_node_ids.size() * 2);  // double the size
    }
    all_node_ids[idx++] = found.id;
    n_cur->next(&found);
  }
  all_node_ids.resize(idx);
  n_cur->close();

  node_id_t num_nodes = all_node_ids.size();
  #ifdef DEBUG
  std::cout << "The number of nodes is: " << num_nodes << std::endl;
  #endif
  graph_stats->set_ro_num_nodes(num_nodes);

  // check if node_ranges needs to be populated
  if (node_ranges.size() > 0)
  {
    return num_nodes;  // already populated; can return
  }
  // Create balanced partitions based on actual node count
  if (num_nodes > 0 && thread_max > 0)
  {
    node_id_t nodes_per_partition =
        (num_nodes + thread_max - 1) / thread_max;  // ceil division

    // First partition starts at first node
    node_ranges.push_back(all_node_ids[0]);

    // Create partition boundaries at every nodes_per_partition interval
    for (int i = 1; i < thread_max; i++)
    {
      node_id_t idx = i * nodes_per_partition;
      if (idx < all_node_ids.size())
      {
        node_ranges.push_back(all_node_ids[idx]);
      }
    }

    // Last partition boundary is the last node
    node_ranges.push_back(all_node_ids.back());

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
  if (thread_id < num_threads - 1)
  {
    to_return.end = node_ranges[thread_id + 1] - 1;
  }
  else
  {
    to_return.end = node_ranges[thread_id + 1];
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
GraphEngine::GraphEngine() {}  //
// Created by puneet on 27/03/25.
//
void GraphEngine::force_metadata_sync() { return; }
