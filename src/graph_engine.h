#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include <array>

#include "adj_list.h"
#include "common_util.h"
#include "edgekey.h"
#include "edgekey_split.h"
#include "graph.h"
#include "graph_exception.h"
// #include "standard_graph.h"

class GraphEngine
{
 public:
  GraphEngine(int _num_threads, graph_opts &engine_opts);
  GraphEngine();
  ~GraphEngine();
  GraphBase *create_graph_handle(bool read_only = false);
  void create_indices();
  void calculate_thread_offsets(bool make_edge = false);
  key_range get_key_range(int thread_id);
  edge_range get_edge_range(int thread_id);
  void close_graph();
  WT_CONNECTION *get_connection();
  std::string get_last_checkpoint() { return last_checkpoint; }

 protected:
  WT_CONNECTION *conn = nullptr;
  std::vector<node_id_t> node_ranges;
  std::vector<edge> edge_ranges;
  int num_threads{};
  graph_opts opts;
  node_id_t last_node_id{};

  void check_opts_valid();
  void create_new_graph();
  void open_connection();
  void close_connection();

 private:
  std::string last_checkpoint;
  void force_metadata_sync();
  void make_checkpoint();
  void _calculate_thread_offsets(int thread_max, GraphBase *graph_stats);
  void _calculate_thread_offsets_edge(int thread_max, GraphBase *graph_stats);
};

GraphEngine::GraphEngine(int _num_threads, graph_opts &engine_opts)
{
  num_threads = _num_threads;
  // init with the engine_opts passed as args without copying
  opts = engine_opts;
  opts.print_config("cmd_config.txt");
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

void GraphEngine::make_checkpoint()
{
  WT_SESSION *session;
  // use timestamp
  auto now =
      std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  std::tm localTime = *std::localtime(&now);
  char cpt_name[27];
  std::strftime(cpt_name, 27, "name=%Y_%m_%d_%H_%M_%S", &localTime);

  std::cout << "Creating checkpoint " << cpt_name << std::endl;
  conn->open_session(conn, nullptr, nullptr, &session);
  if (session->checkpoint(session, cpt_name))
  {
    throw GraphException("Failed to create checkpoint");
  }
  // the checkpoint name without the name= prefix
  std::strftime(cpt_name, 27, "%Y_%m_%d_%H_%M_%S", &localTime);
  last_checkpoint = cpt_name;
  session->close(session, nullptr);
}

GraphBase *GraphEngine::create_graph_handle(bool read_only)
{
  GraphBase *ptr;
  graph_opts new_opts = opts;
  if (read_only)
  {
    make_checkpoint();
    new_opts.read_only = true;
    new_opts.create_new = false;
    new_opts.checkpoint_name = this->last_checkpoint;
  }
  if (new_opts.type == GraphType::Adj)
    ptr = new AdjList(new_opts, conn);
  else if (new_opts.type == GraphType::SplitEKey)
    ptr = new SplitEdgeKey(new_opts, conn);
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
    SplitEdgeKey::create_indices(sess);
  }
  else
  {
    throw GraphException("Failed to create graph object");
  }
}

void GraphEngine::calculate_thread_offsets(bool make_edge)
{
  // Create snapshot here first?
  GraphBase *graph_stats = create_graph_handle();
  _calculate_thread_offsets(num_threads, graph_stats);
  if (make_edge) _calculate_thread_offsets_edge(num_threads, graph_stats);
  graph_stats->close(false);
}

/**
 * This function can be used to calculate the thread offsets if we keep account
 * of what's the min and max key the threads have seen so far.
 *
 * If we don't have this metadata recorded, we can extract this by using the
 * GraphAPI calls: get_min_node, get_max_node.
 *
 * TODO:  Implement these functions in the representations/GraphBase Iface
 */
void GraphEngine::_calculate_thread_offsets(int thread_max,
                                            GraphBase *graph_stats)
{
  node_ranges.clear();
  node_id_t num_nodes = graph_stats->get_num_nodes();

  node_id_t per_partition_nodes =
      (num_nodes / thread_max) +
      ((num_nodes % thread_max) != 0);  // ceil division

  NodeCursor *n_cur = graph_stats->get_node_iter();

  // iterate over the cursor, and then assign the node id when the count is
  // equal to the per_partition
  node_id_t i = 0;
  node found;
  n_cur->next(&found);
  //    std::cout << found.id << std::endl;

  while (found.id != OutOfBand_ID_MAX)
  {
    if (i % per_partition_nodes == 0)
    {
      node_ranges.push_back(found.id);
      //            std::cout << "Node offset: " << found.id << "at offset
      //            " << i
      //                      << std::endl;
    }
    if (i == num_nodes - 1)
    {
      node_ranges.push_back(found.id);
    }
    n_cur->next(&found);
    i++;
  }
  //    std::cout << "The node boundaries are: " << std::endl;
  //    for (auto x : node_ranges)
  //    {
  //        std::cout << x << std::endl;
  //    }
  assert(num_nodes == i);
  n_cur->close();
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
void GraphEngine::check_opts_valid()
{
  if (num_threads < 1)
  {
    throw GraphException("Number of threads must be at least 1");
  }

  try
  {
    CommonUtil::check_graph_params(opts);
  }
  catch (GraphException &G)
  {
    std::cout << G.what() << std::endl;
  }
  if (!CommonUtil::check_dir_exists(opts.stat_log))
  {
    try
    {
      std::cout << "Creating stat log directory " << opts.stat_log << std::endl;
      std::filesystem::create_directories(opts.stat_log);
    }
    catch (GraphException &G)
    {
      std::cout << G.what() << std::endl;
    }
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
    to_return.end = OutOfBand_ID_MAX;
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
GraphEngine::GraphEngine() {}

#endif