#include <atomic>
#include <cassert>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "common_util.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "sample_graph_adjlist.h"
#include "test_utils.h"
#include "times.h"

#define delim "--------------"
#define INFO() fprintf(stdout, "%s\nNow running: %s\n", delim, __FUNCTION__);

atomic<int> rollbakcs(0);
atomic<int> insert_cnt{0};
static std::atomic<bool> final_close_done(false);

// Function for the dedicated monitoring thread.
// It will periodically measure and log performance metrics.
void monitor_function(std::atomic<bool> &stop_flag,
                      const std::string &log_filepath,
                      int interval_seconds)
{
  // Open the log file in append mode. If it doesn't exist, it will be created.
  std::ofstream log_file(log_filepath, std::ios_base::app);

  // Set up variable to measure elapsed time
  long elapsed_time = 0;
  long prev_insertions = 0;

  if (!log_file.is_open())
  {
    std::cerr << "Error: Could not open log file: " << log_filepath
              << std::endl;
    return;  // Exit if file cannot be opened
  }

  if (log_file.tellp() == 0)
  {  // Check if file is empty and write headers
    log_file << "Timestamp,Threads,Memory_VmRSS_GB,Insertions,Rollbacks,Insert_"
                "Bandwidth"
             << std::endl;
  }

  std::cout << "Monitoring thread started. Logging to " << log_filepath
            << " every " << interval_seconds << " seconds." << std::endl;

  // Loop until the stop_flag is set by the main thread.
  while (!stop_flag.load())
  {
    int thread_count = get_current_thread_count();
    long rss_kb = get_current_rss_kb();

    // Get current time for the log entry
    auto now = std::chrono::system_clock::now();
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm *local_tm = std::localtime(
        &current_time_t);  // Not thread-safe, but avoids localtime_r

    char time_str[100];
    // Format time as YYYY-MM-DD HH:MM:SS
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", local_tm);

    // Calculate insertions and rollbacks
    int current_insertions = insert_cnt.load();
    int current_rollbacks = rollbakcs.load();
    int insertions = current_insertions - prev_insertions;
    prev_insertions = current_insertions;
    double insert_bandwidth =
        (insertions) / (interval_seconds);  // Bandwidth in Edges/sec

    // Write metrics to the log file in CSV format
    log_file << time_str << "," << thread_count << "," << rss_kb << ","
             << current_insertions << "," << current_rollbacks << ","
             << insert_bandwidth << std::endl;
    log_file.flush();  // Ensure data is written to disk immediately

    // Sleep for the specified interval
    std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
  }

  log_file.close();  // Close the log file when done
  std::cout << "Monitoring thread stopped gracefully." << std::endl;
}

std::vector<edge> read_edges_parallel(const std::string &filename)
{
  std::vector<std::string> lines;

  // Read all lines into memory
  {
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line))
    {
      lines.push_back(line);
    }
  }

  std::vector<edge> edges(lines.size());

#pragma omp parallel for
  for (size_t i = 0; i < lines.size(); ++i)
  {
    std::istringstream iss(lines[i]);
    node_id_t u, v;
    if (iss >> u >> v)
    {
      edges[i] = {.id = 0,
                  .src_id = u,
                  .dst_id = v,
                  .edge_weight = 0};  // Assuming edge_weight is 0
    }
    else
    {
      continue;  // Invalid line, skip it
    }
  }

  return edges;
}

void test_rollbacks(WT_CONNECTION *conn,
                    graph_opts &opts,
                    std::string &log_file_path)
{
  INFO()
  std::string filename = opts.dataset;
  std::vector<edge> edges = read_edges_parallel(filename);

  std::random_device rd;
  std::mt19937 gen(rd());
  std::shuffle(edges.begin(), edges.end(), gen);

  // --- Configuration for monitoring ---
  std::atomic<bool> stop_monitoring(
      false);                   // Flag to signal monitoring thread to stop
  log_file_path += "_log.csv";  // Path for the log file
  int monitor_interval_seconds =
      60;  // How often to log (e.g., every 5 seconds)

  // --- Start the monitoring thread ---
  // std::ref is used to pass stop_monitoring by reference to the thread
  // function.
  std::thread monitor_th(monitor_function,
                         std::ref(stop_monitoring),
                         log_file_path,
                         monitor_interval_seconds);

  Times t;
  t.start();
  std::cout << "Inserting edges from file: " << filename << std::endl;
  std::cout << "Total edges to insert: " << edges.size() << std::endl;
  std::cout << "Using 8 threads for insertion." << std::endl;
  std::cout << "Starting insertion..." << std::endl;
#pragma omp parallel num_threads(8) shared(rollbakcs, insert_cnt)
  {
    thread_local AdjList graph(opts, conn);
#pragma omp for
    for (int i = 0; i < edges.size(); ++i)
    {
      bool inserted = false;
      while (!inserted)
      {
        int ret = graph.add_edge(edges[i], true);
        if (ret == 0)
        {
          inserted = true;
          insert_cnt++;
          if (insert_cnt.load() % 10000000 == 0)
          {
            std::cout << "Current Inserted count: " << insert_cnt.load()
                      << std::endl;
          }
        }
        else
        {
          rollbakcs++;
          if (rollbakcs.load() % 100000 == 0)
          {
#pragma omp critical
            {
              std::cout << "Current Rollbacks count: " << rollbakcs.load()
                        << std::endl;
            }  // Only one thread prints the rollback count
          }
          // sleep for a short duration to avoid busy waiting
          std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
      }
    }
    // Now sync the metadata
#pragma omp barrier
    if (!final_close_done.exchange(true))
    {
      graph.close(true);
    }
    else
    {
      graph.close();  // other threads close without syncing metadata
    }
  }
  t.stop();
  std::cout << "Insertion completed in " << t.t_secs() << " seconds."
            << std::endl;
  std::cout << "Total edges inserted: " << insert_cnt.load() << std::endl;
  std::cout << "Total Rollback count: " << rollbakcs.load() << std::endl;

  // --- Signal the monitoring thread to stop and wait for it to finish ---
  std::cout << "Signaling monitoring thread to stop..." << std::endl;
  stop_monitoring.store(true);  // Set the flag to true
  monitor_th.join();            // Wait for the monitoring thread to complete
}

void create_init_nodes(WT_CONNECTION *conn, graph_opts &opts)
{
  INFO()

  atomic<int> rollbakcs(0);
  atomic<int> insert_cnt{0};
#pragma omp parallel for num_threads(1) shared(rollbakcs, insert_cnt)
  for (edge x : SampleGraphAdjList::parallel_insert_edges)
  {
    std::cout << "Inserting edge: " << x.src_id << " -> " << x.dst_id
              << std::endl;
    thread_local AdjList graph(opts, conn);
    bool inserted = false;
    while (!inserted)
    {
      int ret = graph.add_edge(x, true);
      if (ret == 0)
      {
        inserted = true;
        insert_cnt++;
      }
      else
      {
        rollbakcs++;
      }
    }
  }
  std::cout << "Rollback count: " << rollbakcs.load() << std::endl;
  std::cout << "Inserted count: " << insert_cnt.load() << std::endl;
}

void test_node_add(AdjList &graph, graph_opts &opts)
{
  INFO();
  node new_node = {.id = 111, .in_degree = 0, .out_degree = 0};
  graph.add_node(new_node);
  node found = graph.get_node(new_node.id);
  assert(new_node.id == found.id);
  if (opts.read_optimize)
  {
    assert(found.in_degree == 0);
    assert(found.out_degree == 0);
  }
  // Check the adjlists
  if (opts.is_directed)
  {
    WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
    CommonUtil::set_key(in_adj_cur, new_node.id);
    assert(in_adj_cur->search(in_adj_cur) == 0);
  }

  WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
  CommonUtil::set_key(out_adj_cur, new_node.id);
  assert(out_adj_cur->search(out_adj_cur) == 0);
}

void test_get_node(AdjList &graph, graph_opts &opts)
{
  INFO();
  int test_id1 = 1, test_id2 = 55;
  node found = graph.get_node(test_id1);
  CommonUtil::dump_node(found);
  assert(found.id == 1);
  assert(found.out_degree == 5);
  assert(found.in_degree == 0);  // node 1 has no incoming edges in the
                                 // sample graph

  // now get a node that does not exist
  found = graph.get_node(test_id2);
  assert(found.id == OutOfBand_ID_MAX);
  found = graph.get_random_node();
  CommonUtil::dump_node(found);
}

void test_get_nodes(AdjList &graph, graph_opts &opts)
{
  INFO();
  int n = 10;
  for (node x : graph.get_nodes())
  {
    CommonUtil::dump_node(x);
#ifdef B64
    assert(sizeof(x.id) == 8);
#else
    assert(sizeof(x.id) == 4);
#endif
    if (--n == 0) break;
  }
}

void test_get_edges(AdjList &graph)
{
  INFO();
  for (edge e : graph.get_edges())
  {
    CommonUtil::dump_edge(e);
  }
}

void test_get_adjlist(AdjList &graph, int node_id)
{
  INFO();
  std::cout << "Printing the in_adjlist for node " << node_id << std::endl;
  WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
  WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
  for (auto al : graph.get_adjlist(in_adj_cur, node_id))
  {
    std::cout << al << " ";
  }
  std::cout << "Printing the out_adjlist for node " << node_id << std::endl;
  for (auto al : graph.get_adjlist(out_adj_cur, node_id))
  {
    std::cout << al << " ";
  }
}

void test_get_edge(AdjList &graph, bool is_directed)
{
  INFO();
  edge found = graph.get_edge(SampleGraphAdjList::edge1.src_id,
                              SampleGraphAdjList::edge1.dst_id);
  CommonUtil::dump_edge(found);
  assert(found.src_id == SampleGraphAdjList::edge1.src_id);
  assert(found.dst_id == SampleGraphAdjList::edge1.dst_id);
  if (!is_directed)
  {
    found = graph.get_edge(SampleGraphAdjList::edge1.dst_id,
                           SampleGraphAdjList::edge1.src_id);
    assert(found.src_id == SampleGraphAdjList::edge1.dst_id);
    assert(found.dst_id == SampleGraphAdjList::edge1.src_id);
  }

  // Now get a non-existent edge
  int test_id1 = 222, test_id2 = 333;
  found = graph.get_edge(test_id1, test_id2);
  assert(found.src_id == OutOfBand_ID_MAX);
  assert(found.dst_id == OutOfBand_ID_MAX);
  assert(found.edge_weight == 0);
}

void test_add_edge(AdjList &graph, bool is_directed)
{
  INFO();
  edge to_insert = {
      .src_id = 18,
      .dst_id = 19,
      .edge_weight = 33.33};  // node 300 and 400 dont exist yet so we must also
                              // check if the nodes get created
  int test_id1 = 18, test_id2 = 19;
  graph.add_edge(to_insert, false);
  edge found = graph.get_edge(test_id1, test_id2);
  CommonUtil::dump_edge(found);
  assert(found.edge_weight == 33.33);
  if (!is_directed)
  {
    found = graph.get_edge(test_id2, test_id1);
    CommonUtil::dump_edge(found);
    assert(found.src_id == test_id2);
    assert(found.dst_id == test_id1);
    assert(found.edge_weight == 33.33);
  }
  // Check if the nodes were created.
  node got = graph.get_node(test_id1);
  CommonUtil::dump_node(got);
  assert(got.id == test_id1);
  assert(got.out_degree == 1 && got.in_degree == 0);
  // the node should have no in edge; if undirected, we don't consider
  // in_degree.

  got = graph.get_node(test_id2);
  CommonUtil::dump_node(got);
  assert(got.id == test_id2);
  if (is_directed)
    assert(got.in_degree == 1 && got.out_degree == 0);
  else
    assert(got.in_degree == 0 && got.out_degree == 1);
  // node has no out_edge, but for undirected graph, we only consider
  // out_degree.

  // Now check if the adjlists were updated
  WT_CURSOR *in_adj_cur = graph.get_in_adjlist_cursor();
  CommonUtil::set_key(in_adj_cur, test_id2);
  assert(in_adj_cur->search(in_adj_cur) == 0);
  std::vector<node_id_t> adjlist = graph.get_adjlist(in_adj_cur, test_id2);
  std::cout << "Printing in_adjlist for node " << test_id2 << std::endl;
  for (auto x : adjlist)
  {
    std::cout << x << " ";
  }
  std::cout << std::endl;
  assert(adjlist.size() == 1);
  if (!is_directed)
  {
    CommonUtil::set_key(in_adj_cur, test_id1);
    assert(in_adj_cur->search(in_adj_cur) == 0);
    in_adj_cur->reset(in_adj_cur);
    adjlist = graph.get_adjlist(in_adj_cur, test_id2);
    assert(adjlist.size() == 1);
  }
  in_adj_cur->reset(in_adj_cur);

  WT_CURSOR *out_adj_cur = graph.get_out_adjlist_cursor();
  CommonUtil::set_key(out_adj_cur, test_id1);
  assert(out_adj_cur->search(out_adj_cur) == 0);
  out_adj_cur->reset(out_adj_cur);
  adjlist = graph.get_adjlist(out_adj_cur, test_id1);
  assert(adjlist.size() == 1);

  if (!is_directed)
  {
    CommonUtil::set_key(out_adj_cur, test_id2);
    out_adj_cur->search(out_adj_cur);
    assert(out_adj_cur->search(out_adj_cur) == 0);
    out_adj_cur->reset(out_adj_cur);
    adjlist = graph.get_adjlist(out_adj_cur, test_id2);
    assert(adjlist.size() == 1);
  }
}

void test_get_out_edges(AdjList &graph, graph_opts &opts)
{
  INFO();
  int test_id1 = 1, test_id2 = 111,
      test_id3 = 1500;  // 111 is a node that has no edges
  // node 1500 does not exist
  std::vector<edge> edges = graph.get_out_edges(test_id1);

  assert(edges.size() == 5);  // 5 out edges, 0 in edges. deg = 5 always.

  for (auto e : edges)
  {
    CommonUtil::dump_edge(e);
  }
  // compare edge0
  assert(edges.at(0).src_id == SampleGraphAdjList::edge1.src_id);
  assert(edges.at(0).dst_id == SampleGraphAdjList::edge1.dst_id);
  // compare edge1
  assert(edges.at(1).src_id == SampleGraphAdjList::edge2.src_id);
  assert(edges.at(1).dst_id == SampleGraphAdjList::edge2.dst_id);
  // compare edge4
  assert(edges.at(2).src_id == SampleGraphAdjList::edge3.src_id);
  assert(edges.at(2).dst_id == SampleGraphAdjList::edge3.dst_id);

  // Now test for a node that has no out edge
  edges = graph.get_out_edges(test_id2);
  assert(edges.empty());

  // Now try getting out edges for a node that does not exist
  bool assert_fail = false;
  try
  {
    edges = graph.get_out_edges(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << " @@@ caught" << endl;
    assert_fail = true;
  }
  // assert(assert_fail);
}

void test_get_in_edges(AdjList &graph, graph_opts &opts)
{
  INFO();
  int test_id1 = 4, test_id2 = 3, test_id3 = 1500;
  std::vector<edge> edges = graph.get_in_edges(test_id1);

  if (opts.is_directed)
  {
    assert(edges.size() == 3);
  }
  else
  {
    assert(edges.size() == 8);
  }
  // Check edge0 (1, 4)
  assert(edges.at(0).src_id == 1);
  assert(edges.at(0).dst_id == 4);
  // Check edge2 (2,4)
  assert(edges.at(1).src_id == 2);
  assert(edges.at(1).dst_id == 4);
  // Check edge4 (3,4)
  assert(edges.at(2).src_id == 3);
  assert(edges.at(2).dst_id == 4);

  if (!opts.is_directed)
  {
    // check edge reverse of (4,5)
    assert(edges.at(3).src_id == 5);
    assert(edges.at(3).dst_id == 4);
    // Check edge reverse of  (4, 7)
    assert(edges.at(4).src_id == 7);
    assert(edges.at(4).dst_id == 4);
    // Check  edge reverse of   (4,8)
    assert(edges.at(5).src_id == 8);
    assert(edges.at(5).dst_id == 4);
    // Check  edge reverse of   (4,9)
    assert(edges.at(6).src_id == 9);
    assert(edges.at(6).dst_id == 4);
    // Check  edge reverse of   (4,13)
    assert(edges.at(7).src_id == 13);
    assert(edges.at(7).dst_id == 4);
  }

  // now test for a node that has no in-edge (can only happen if the graph is
  // directed. In this case, node 3 has no in-edge)
  if (opts.is_directed)
  {
    edges = graph.get_in_edges(test_id2);
    assert(edges.empty());
  }

  // Now test for a node that has no out edge
  edges = graph.get_out_edges(111);
  assert(edges.empty());

  // Now try getting in edges for a node that does not exist.
  bool assert_fail = false;
  try
  {
    edges = graph.get_out_edges(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_out_nodes(AdjList &graph, graph_opts &opts)
{
  INFO();
  int test_id1 = 1, test_id2 = 111, test_id3 = 1500;
  std::vector<node> nodes = graph.get_out_nodes(test_id1);
  std::vector<node_id_t> nodes_id = graph.get_out_nodes_id(test_id1);
  // Node 1 only has out edges; degree is 5.
  assert(nodes.size() == 5);
  assert(nodes_id.size() == 5);

  assert(nodes.at(0).id == 4);  // edge(1->4)
  assert(nodes.at(0).id == nodes_id.at(0));
  assert(nodes.at(1).id == 5);  // edge(1->5)
  assert(nodes.at(1).id == nodes_id.at(1));
  assert(nodes.at(2).id == 8);  // edge(1->8)
  assert(nodes.at(2).id == nodes_id.at(2));
  assert(nodes.at(3).id == 9);  // edge(1->9)
  assert(nodes.at(3).id == nodes_id.at(3));
  assert(nodes.at(4).id == 13);  // edge(1->13)
  assert(nodes.at(4).id == nodes_id.at(4));

  // test for a node that has no out-edge
  nodes = graph.get_out_nodes(test_id2);
  nodes_id = graph.get_out_nodes_id(test_id2);
  assert(nodes.empty());
  assert(nodes_id.empty());

  // test for a node that does not exist
  bool assert_fail = false;
  try
  {
    nodes = graph.get_out_nodes(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);

  assert_fail = false;
  try
  {
    nodes_id = graph.get_out_nodes_id(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_in_nodes(AdjList &graph, graph_opts &opts)
{
  INFO();
  int test_id1 = 1, test_id2 = 4, test_id3 = 1500;
  std::vector<node> nodes = graph.get_in_nodes(test_id1);
  //lambda to print
  for (auto n : nodes)
  {
    CommonUtil::dump_node(n);
  }
  std::vector<node_id_t> nodes_id = graph.get_in_nodes_id(test_id1);
  std::sort(nodes_id.begin(), nodes_id.end()); // needed for this test
  for (auto id : nodes_id)
  {
    std::cout << "Node ID: " << id << std::endl;
  }

  // Node 1 only has out edges; degree is 5.
  if (opts.is_directed)
  {
    assert(nodes.empty());
    assert(nodes_id.empty());
    return;  // no in edges for node 1 in a directed graph
  }
  else
  {
    assert(nodes.size() == 5);
    assert(nodes_id.size() == 5);

    for (int i = 0; i < nodes.size(); i++)
    {
      std::cout << "Node ID: " << nodes.at(i).id
                << ", Node ID from nodes_id: " << nodes_id.at(i)
                << std::endl;
      assert(nodes.at(i).id == nodes_id.at(i));
    }
  }
  // test for a node that has a valid in-edge
  nodes = graph.get_in_nodes(test_id2);
  nodes_id = graph.get_in_nodes_id(test_id2);
  // Node 4 has 3 inedges (1,2,3) and 5 out edges (5,7,8,9,13)
  std::sort(nodes_id.begin(), nodes_id.end()); // needed for this test
  if (opts.is_directed)
  {
    assert(nodes.size() == 3);
    assert(nodes_id.size() == 3);
    assert(nodes.at(0).id == 1);  // edge(1->4)
    assert(nodes.at(0).id == nodes_id.at(0));
    assert(nodes.at(1).id == 2);  // edge(2->4)
    assert(nodes.at(1).id == nodes_id.at(1));
    assert(nodes.at(2).id == 3);  // edge(3->4)
    assert(nodes.at(2).id == nodes_id.at(2));
  }
  else
  {
    assert(nodes.size() == 8);
    assert(nodes_id.size() == 8);
    assert(nodes.at(0).id == 1);  // edge(1->4)
    assert(nodes.at(0).id == nodes_id.at(0));
    assert(nodes.at(1).id == 2);  // edge(2->4)
    assert(nodes.at(1).id == nodes_id.at(1));
    assert(nodes.at(2).id == 3);  // edge(3->4)
    assert(nodes.at(2).id == nodes_id.at(2));
    assert(nodes.at(3).id == 5);  // edge(4->5)
    assert(nodes.at(3).id == nodes_id.at(3));
    assert(nodes.at(4).id == 7);  // edge(4->7)
    assert(nodes.at(4).id == nodes_id.at(4));
    assert(nodes.at(5).id == 8);  // edge(4->8)
    assert(nodes.at(5).id == nodes_id.at(5));
    assert(nodes.at(6).id == 9);  // edge(4->9)
    assert(nodes.at(6).id == nodes_id.at(6));
    assert(nodes.at(7).id == 13);  // edge (4->13)
    assert(nodes.at(7).id == nodes_id.at(7));
  }

  // test for a node that does not exist
  bool assert_fail = false;
  try
  {
    nodes = graph.get_in_nodes(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);

  assert_fail = false;
  try
  {
    nodes_id = graph.get_in_nodes_id(test_id3);
  }
  catch (GraphException &ex)
  {
    cout << ex.what() << endl;
    assert_fail = true;
  }
  assert(assert_fail);
}

void test_get_in_and_out_degree(AdjList &graph, bool directed)
{
  //  INFO();
  //  // check in_degree for node3
  //  int test_id = 3;
  //  auto deg = graph.get_in_degree(test_id);
  //  std::cout << "In-degree of node " << test_id << " is " << deg <<
  //  std::endl; assert(deg == 2);
  INFO()
  degree_t indeg, outdeg;
  indeg = graph.get_in_degree(3);
  outdeg = graph.get_out_degree(3);
  assert(indeg == 2);
  if (!directed)
  {
    assert(outdeg == 2);
  }
  else
  {
    assert(outdeg == 0);
  }
  indeg = graph.get_in_degree(1);
  outdeg = graph.get_out_degree(1);
  assert(outdeg == 3);
  if (!directed)
  {
    assert(indeg == 3);
  }
  else
  {
    assert(indeg == 0);
  }
}

void test_delete_node(AdjList &graph, bool is_directed)
{
  INFO();
  int ret = 0;
#ifdef MK_NEDGES
  WT_CURSOR *n_cursor = graph.get_node_cursor();
  WT_CURSOR *e_cursor = graph.get_edge_cursor();
#endif
  WT_CURSOR *adj_out_cur = graph.get_out_adjlist_cursor();
  WT_CURSOR *adj_in_cur = graph.get_in_adjlist_cursor();

#ifdef MK_NEDGES
  // Verify node2 exists
  CommonUtil::set_key(n_cursor, SampleGraphAdjList::node2.id);
  ret = n_cursor->search(n_cursor);
  assert(ret == 0);
  n_cursor->reset(n_cursor);
#endif
  // Delete node2 and verify it was actually deleted
  graph.delete_node(SampleGraphAdjList::node2.id);

#ifdef MK_NEDGES
  CommonUtil::set_key(n_cursor, SampleGraphAdjList::node2.id);
  ret = n_cursor->search(n_cursor);
  assert(ret != 0);
#endif
  // Verify node2's adjacency lists are deleted
  CommonUtil::set_key(adj_out_cur, SampleGraphAdjList::node2.id);
  ret = adj_out_cur->search(adj_out_cur);
  assert(ret != 0);
  CommonUtil::set_key(adj_in_cur, SampleGraphAdjList::node2.id);
  ret = adj_in_cur->search(adj_in_cur);
  assert(ret != 0);
  /*
  This was valid for the old sample graph.
  #ifdef MK_NEDGES
    // check that edge(2,3) is deleted
    CommonUtil::set_key(e_cursor, 2, 3);
    assert(e_cursor->search(e_cursor) != 0);
    // check that edge(1,2) is deleted
    CommonUtil::set_key(e_cursor, 1, 2);
    assert(e_cursor->search(e_cursor) != 0);
    // Now delete the reverse edges for undirected graph
    if (!is_directed)
    {
      CommonUtil::set_key(e_cursor, 3, 2);
      assert(e_cursor->search(e_cursor) != 0);
      CommonUtil::set_key(e_cursor, 2, 1);
      assert(e_cursor->search(e_cursor) != 0);
    }
  #endif
  */
  // Verify that node 2 is deleted from adjlist of node1 and node3
  // In the new sample graph, for node 2: ((out) 13, 4, 5,7, 8, 9 | in {} )
  adj_out_cur->reset(adj_out_cur);
  auto outnodes = {13, 4, 5, 7, 8, 9};
  for (auto out : outnodes)
  {
    for (auto dst : graph.get_adjlist(adj_out_cur, out))
    {
      // std::cout << "@408 dst: " << dst << std::endl;
      assert(dst !=
             SampleGraphAdjList::node2.id);  // node2 should have been deleted
    }
  }

  adj_in_cur->reset(adj_in_cur);
  for (auto in : outnodes)
  {
    for (auto src : graph.get_adjlist(adj_in_cur, in))
    {
      // std::cout << "@412 src: " << src << std::endl;
      assert(src !=
             SampleGraphAdjList::node2.id);  // node2 should have been deleted
    }
  }
}

void test_delete_isolated_node(AdjList &graph, bool is_directed)
{
  INFO();
  int ret = 0;

  graph.delete_node(SampleGraphAdjList::isolated_node.id);
  return;

#ifdef MK_NEDGES
  // Verify node4 exists
  WT_CURSOR *n_cursor = graph.get_node_cursor();
  // Delete isolated_node and verify it was actually deleted
  CommonUtil::set_key(n_cursor, SampleGraphAdjList::isolated_node.id);
  ret = n_cursor->search(n_cursor);
  assert(ret != 0);
#endif

  // Verify node4's adjacency lists are deleted
  WT_CURSOR *adj_out_cur = graph.get_out_adjlist_cursor();
  CommonUtil::set_key(adj_out_cur, SampleGraphAdjList::isolated_node.id);
  assert(adj_out_cur->search(adj_out_cur) != 0);

  WT_CURSOR *adj_in_cur = graph.get_in_adjlist_cursor();
  CommonUtil::set_key(adj_in_cur, SampleGraphAdjList::isolated_node.id);
  assert(adj_in_cur->search(adj_in_cur) != 0);
  return;

#ifdef MK_NEDGES
  // Check no edge has node4 in source or dst
  std::vector<edge> edges = graph.get_edges();
  for (edge e : edges)
  {
    assert(e.src_id != SampleGraphAdjList::isolated_node.id);
    assert(e.dst_id != SampleGraphAdjList::isolated_node.id);
  }
#endif
  // Now check if node4 is present in in/out_adj_list of any of the remaining
  // nodes;
  adj_out_cur->reset(adj_out_cur);
  while (adj_out_cur->next(adj_out_cur) == 0)
  {
    adjlist found;
    CommonUtil::record_to_adjlist(adj_out_cur, &found);
    for (auto dst : found.edgelist)
    {
      assert(dst != SampleGraphAdjList::isolated_node.id);  // node4 should have
                                                            // been deleted
    }
  }

  adj_in_cur->reset(adj_in_cur);
  while (adj_in_cur->next(adj_in_cur) == 0)
  {
    adjlist found;
    CommonUtil::record_to_adjlist(adj_in_cur, &found);
    for (auto src : found.edgelist)
    {
      assert(src != SampleGraphAdjList::isolated_node.id);  // node4 should have
                                                            // been deleted
    }
  }
}

void test_InCursor(AdjList &graph)
{
  INFO();
  auto *in_cursor = (AdjInCursor *)graph.get_innbd_iter();
  adjlist found;

  // in_cursor->setAllNodes(true);
  // std::cout << "Printing in-adjlists for all nodes (AllNodes=true)\n"
  //           << std::endl;
  // in_cursor->next(&found);
  // while (found.node_id != OutOfBand_ID_MAX)
  // {
  //   CommonUtil::dump_adjlist(found);
  //   found.clear();
  //   in_cursor->next(&found);
  // }
  // std::cout << "the found.node id is " << found.node_id << std::endl;
  // assert(found.node_id == OutOfBand_ID_MAX);
  // found.clear();
  std::cout
      << "Printing in-adjlists for nodes with non-null nbd (AllNodes=false)\n"
      << std::endl;
  in_cursor->setAllNodes(false);
  // adjlist found;
  in_cursor->next(&found);
  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    in_cursor->next(&found);
  }
  in_cursor->close();
  delete in_cursor;
}

void test_OutCursor(AdjList &graph)
{
  INFO();
  adjlist found;

  auto *out_cursor = (AdjOutCursor *)graph.get_outnbd_iter();
  out_cursor->setAllNodes(true);
  std::cout << "Printing in-adjlists for all nodes (AllNodes=true)\n";
  out_cursor->next(&found);
  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    out_cursor->next(&found);
  }
  delete out_cursor;

  out_cursor = (AdjOutCursor *)graph.get_outnbd_iter();
  out_cursor->setAllNodes(false);
  std::cout << "Printing in-adjlists for nodes with non-null nbd "
               "(AllNodes=false)\n";
  out_cursor->next(&found);
  while (found.node_id != OutOfBand_ID_MAX)
  {
    CommonUtil::dump_adjlist(found);
    found.clear();
    out_cursor->next(&found);
  }
  out_cursor->close();
  delete out_cursor;
}

void test_NodeCursor(AdjList &graph)
{
  INFO();
  NodeCursor *node_cursor = graph.get_node_iter();
  node found;
  node_id_t nodeIdList[] = {1,
                            3,
                            4,
                            5,
                            6,
                            7,
                            8,
                            9,  // node 2 is deleted
                            10,
                            11,
                            12,
                            13,
                            14,
                            15,
                            16,
                            18,
                            19, // test_add_edge adds nodes 18 and 19
                            222,// update_edge adds nodes 222 and 333
                            333};  
  int i = 0;
  node_cursor->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    std::cout << "Found node " << found.id << "\tExpected node "
              << nodeIdList[i] << std::endl;
    assert(found.id == nodeIdList[i]);
    CommonUtil::dump_node(found);
    node_cursor->next(&found);
    i++;
  }
  node_cursor->close();
  delete node_cursor;
}

void test_NodeCursor_Range(AdjList &graph)
{
  INFO();
  NodeCursor *node_cursor = graph.get_node_iter();
  node found;
  node_id_t nodeIdList[] = {3, 4, 5, 6};  // nodes in range [3,6]
  int i = 0;
  node_cursor->set_key_range(key_range{3, 6});
  node_cursor->next(&found);
  while (found.id != OutOfBand_ID_MAX)
  {
    assert(found.id == nodeIdList[i]);
    CommonUtil::dump_node(found);
    node_cursor->next(&found);
    i++;
  }
  node_cursor->close();
  delete node_cursor;
}

void test_EdgeCursor(AdjList &graph, bool is_directed)
{
  INFO();
  EdgeCursor *edge_cursor = graph.get_edge_iter();
  edge found;
  int i = 0;
  edge_cursor->next(&found);
  while (found.src_id != OutOfBand_ID_MAX)
  {
    // assert(found.src_id == expected[i].first);
    // assert(found.dst_id == expected[i].second);
    std::cout << "(" << found.src_id << " , " << found.dst_id << ")"
              << std::endl;
    // CommonUtil::dump_edge(found);
    edge_cursor->next(&found);
    i++;
  }
  edge_cursor->close();
  delete edge_cursor;
}

void test_EdgeCursor_Range(AdjList &graph, bool is_directed)
{
  INFO();
  EdgeCursor *edge_cursor = graph.get_edge_iter();
  edge_cursor->set_key_range(edge_range(key_pair{1, 5}, key_pair{3, 5}));
  edge found;
  std::vector<std::pair<node_id_t, node_id_t>> expected;
  if (is_directed)
  {
    expected = {{1, 5}, {1, 8}, {1, 9}, {1, 13}, {3, 4}, {3, 5}};
  }
  else
  {
    expected = {{1, 5}, {1, 8}, {1, 9}, {1, 13}, {3, 1}, {3, 4}, {3, 5}};
  }
  
  int i = 0;
  edge_cursor->next(&found);
  while (found.src_id != OutOfBand_ID_MAX)
  {
    assert(found.src_id == expected[i].first);
    assert(found.dst_id == expected[i].second);
    CommonUtil::dump_edge(found);
    edge_cursor->next(&found);
    i++;
  }
  edge_cursor->close();
  delete edge_cursor;
}
void tearDown(AdjList &graph) { graph.close(true); }

void test_ro_get_nodes(GraphBase *graph)
{
  INFO();
  int n = 10;
  for (node x : graph->get_nodes())
  {
    CommonUtil::dump_node(x);
#ifdef B64
    assert(sizeof(x.id) == 8);
#else
    assert(sizeof(x.id) == 4);
#endif
    if (--n == 0) break;
  }
}

void test_update_edge(AdjList &graph, bool is_directed)
{
  INFO();
  edge to_update = {
      .src_id = 1,
      .dst_id = 3,
      .edge_weight = 111.11};  // edge (1,3) exists in the sample graph
  int test_id1 = 1, test_id2 = 3;
  graph.update_edge(to_update);
  edge found = graph.get_edge(test_id1, test_id2);
  CommonUtil::dump_edge(found);
  assert(found.edge_weight == 111.11);
  if (!is_directed)
  {
    found = graph.get_edge(test_id2, test_id1);
    CommonUtil::dump_edge(found);
    assert(found.src_id == test_id2);
    assert(found.dst_id == test_id1);
    assert(found.edge_weight == 111.11);
  }

  // Now try updating a non-existent edge
  to_update = {.src_id = 222,
               .dst_id = 333,
               .edge_weight = 44.44};  // edge (222,333) does not exist

  bool updated = graph.update_edge(to_update);
  assert(updated == true);  // add the edge if it does not exist.
  edge found1 = graph.get_edge(222, 333);
  CommonUtil::dump_edge(found1);
  assert(found1.src_id == 222);
  assert(found1.dst_id == 333);
  assert(found1.edge_weight == 44.44);
}

// accept read_opt, sort_edges from command line
int main(int argc, char *argv[])
{
  const int THREAD_NUM = 1;
  graph_opts opts;
  opts.create_new = true;
  opts.optimize_create = false;
  opts.is_directed = false;
  //opts.is_directed = true;

  opts.is_weighted = true;
  opts.type = GraphType::Adj;
  opts.db_dir = "./db";
  opts.db_name = "test_adj";
  opts.conn_config =
      "cache_size=100GB,eviction_trigger=95,eviction_dirty_trigger=95,eviction_"
      "dirty_target=85";  //",verbose=[evict:1,evictserver:1,transaction:1]";
  opts.sort_edges = true;
  if (const char *env_p = std::getenv("GRAPH_PROJECT_DIR"))
  {
    opts.stat_log = std::string(env_p);
  }
  else
  {
    std::cout << "GRAPH_PROJECT_DIR not set. Using CWD" << std::endl;
    opts.stat_log = "./";
  }
  opts.dataset = "/drives/hdd_main/datasets_downloads/dota_league/dota_league";
  std::string log_name = "results/";
  // extract last part of the dataset path to make the log name
  log_name += "DotaLeague_";  // parser cannot handle underscores in the name
#ifdef MK_NEDGES
  std::cout << "DB being created with MK_NEDGES enabled" << std::endl;
  log_name += "withNodes";
#else
  std::cout << "DB being created with MK_NEDGES disabled" << std::endl;
  log_name += "withoutNodes";
#endif

#ifdef B64
  std::cout << "Using 64-bit IDs" << std::endl;
#else
  std::cout << "Using 32-bit IDs" << std::endl;
#endif

#ifdef OrderNodes
  std::cout << "Using OrderNodes" << std::endl;
  log_name += "_OrderNodes";
#else
  std::cout << "Not using OrderNodes" << std::endl;
  log_name += "_NoOrderNodes";
#endif

  // opts.sort_edges ? log_name += "_SortEdges" : log_name += "_NoSortEdges";
  // opts.read_optimize ? log_name += "_ReadOpt" : log_name += "_NoReadOpt";

  opts.sort_edges = false;
  opts.read_optimize = false;  // false;

  // GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
  //                                            .opts = opts};
  //  opts.create_new = false;
  GraphEngine myEngine(THREAD_NUM, opts);
  WT_CONNECTION *conn = myEngine.get_connection();
  create_init_nodes(conn, opts);

  // test_rollbacks(conn, opts, log_name);

  AdjList graph(opts, conn);
  //  graph.dump_meta_data();
  //  graph.close();
  test_EdgeCursor(graph, opts.is_directed);
  test_get_nodes(graph, opts);
  test_get_node(graph, opts);
  test_add_edge(graph, opts.is_directed);
  test_node_add(graph, opts);
  test_get_edge(graph, opts.is_directed);
  test_get_out_edges(graph, opts);
  test_get_in_edges(graph, opts);
  test_get_out_nodes(graph, opts);
  test_get_in_nodes(graph, opts);
  test_update_edge(graph, opts.is_directed);
  test_delete_node(graph, opts.is_directed);

  test_delete_isolated_node(graph, opts.is_directed);
  test_InCursor(graph);
  test_OutCursor(graph);
  test_NodeCursor(graph);
  test_NodeCursor_Range(graph);
  test_EdgeCursor(graph, opts.is_directed);
  test_EdgeCursor_Range(graph, opts.is_directed);

  std::cout << "Number of nodes in graph: " << graph.get_num_nodes() << std::endl;

  tearDown(graph);
  myEngine.close_graph();

  ////////////
  // Now test for read_only mode
  opts.create_new = false;
  opts.read_only = true;
  GraphEngine roEngine(THREAD_NUM, opts);
  std::string chkpt_name;
  GraphBase *rograph = roEngine.create_ro_graph_handle(chkpt_name);
  std::cout << "Number of nodes in RO graph: " << rograph->get_num_nodes() << std::endl;
  test_ro_get_nodes(rograph);


  //try to insert a node - should fail
  int ret = rograph->add_node({.id = 1000, .in_degree = 0, .out_degree = 0}, false);
  assert(ret == WT_ROLLBACK);
  rograph->close(false);
  roEngine.close_graph();
}

/** notes:
 * 
 * update edge: if the nodes do not exist, should we create them? check the graphalytics spec.
 */