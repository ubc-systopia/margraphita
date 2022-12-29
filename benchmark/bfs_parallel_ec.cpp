#include <math.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <deque>
#include <fstream>
#include <iostream>
#include <list>
#include <set>
#include <vector>

#include "adj_list.h"
#include "bitmap.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

#define INFO() fprintf(stderr, "%s\nNow running: %s\n", delim, __FUNCTION__);

// debug workflow:
// get to compile. look at cmakelists in benchmarks.

template <typename Graph>
int find_random_start(Graph &graph)
{
    while (1)
    {
        node random_start = graph.get_random_node();
        std::cout << random_start.id << std ::endl;
        std::cout << "random start vertex is " << random_start.id << std::endl;
        if (graph.get_out_degree(random_start.id) != 0)
        {
            return random_start.id;
        }
        else
        {
            std::cout << "out degree was zero. finding a new random vertex."
                      << std::endl;
        }
    }
}

void BFS_EC(GraphEngine *graph_engine,
               pvector<node_id_t> &parent,
               int thread_num)
{
bool done = false;

while (!done) {

    done = true;
    #pragma omp parallel for reduction(& : done) num_threads(thread_num)
        for (int i = 0; i < thread_num; i++)
        {   

            //cout << "first" << i << endl;

            GraphBase *graph = graph_engine->create_graph_handle();
            edge found = {0};

            //cout << "second" << i << endl;

            EdgeCursor* edge_cursor = graph->get_edge_iter();
            edge_cursor->set_key(graph_engine->get_edge_range(i));
            edge_cursor->next(&found);

            while (found.src_id != -1)
            {   
                // we need to check this edge on the frontier
                //cout << "inside" << endl;
                long int old_val = -1;
                if (parent[found.src_id] != -1 && compare_and_swap(parent[found.dst_id], old_val, found.src_id)) 
                {   
                    done = false;
                }
                edge_cursor->next(&found);
            }

            //cout << " close" << endl;
            graph->close();

            //cout << "closed" << endl;
        }
    }
}



// BFS verifier does a serial BFS from same source and asserts:
// - parent[source] = source
// - parent[v] = u  =>  depth[v] = depth[u] + 1 (except for source)
// - parent[v] = u  => there is edge from u to v
// - all vertices reachable from source have a parent
bool BFSVerifier(GraphEngine* graph_engine, node_id_t source, // i removed 'const' graph_engine
                 const pvector<node_id_t> &parent) {


  cout << "start BFS verifier" << endl;
  GraphBase *g = graph_engine->create_graph_handle();

  pvector<int> depth(g->get_num_nodes(), -1); 
  depth[source] = 0;
  vector<node_id_t> to_visit;
  to_visit.reserve(g->get_num_nodes());
  to_visit.push_back(source);
  for (auto it = to_visit.begin(); it != to_visit.end(); it++) {
    node_id_t u = *it;
    for (node_id_t v : g->get_out_nodes_id(u)) {
      if (depth[v] == -1) {
        depth[v] = depth[u] + 1;
        to_visit.push_back(v);
      }
    }
  }
  for (node_id_t u = 0; u < (node_id_t) g->get_num_nodes(); ++u) { // is this cast ok?
    if ((depth[u] != -1) && (parent[u] != -1)) {
      if (u == source) {
        if (!((parent[u] == u) && (depth[u] == 0))) {
          cout << "Source wrong" << endl;
          return false;
        }
        continue;
      }
      bool parent_found = false;
      for (node_id_t v : g->get_in_nodes_id(u)) {
        if (v == parent[u]) {
          if (depth[v] != depth[u] - 1) {
            cout << "Wrong depths for " << u << " & " << v << endl; // hitting this error!!!
            return false;
          }
          parent_found = true;
          break;
        }
      }
      if (!parent_found) {
        cout << "Couldn't find edge from " << parent[u] << " to " << u << endl;
        return false;
      }
    } else if (depth[u] != parent[u]) {
      cout << "Reachability mismatch" << endl;
      return false;
    }

    g->close();
  }
  return true;
}

void create_init_nodes(AdjList graph, bool is_directed)
{
    INFO()
    for (node x : SampleGraph::test_nodes)
    {
        graph.add_node(x);
    }

    if (!is_directed)
    {
        SampleGraph::create_undirected_edges();
        assert(SampleGraph::test_edges.size() ==
               6);  // checking if directed edges got created and stored in
                    // test_edges
    }
    int edge_cnt = 1;
    for (node n : SampleGraph::test_nodes)
    {
        graph.add_node(n);
    }

    for (edge x : SampleGraph::test_edges)
    {
        graph.add_edge(x, false);
        edge_cnt++;
    }
}

void tearDown(AdjList graph) { graph.close(); }

void test_bfs_ec(AdjList graph)
{

  // testing
    const int THREAD_NUM = 1;
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.type = GraphType::Adj;
    opts.db_dir = "./db";
    opts.db_name = "test_adj";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = std::getenv("GRAPH_PROJECT_DIR");

    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};
    GraphEngineTest myEngine(engine_opts);
    WT_CONNECTION *conn = myEngine.public_get_connection();
    AdjList graph(opts, conn);

    create_init_nodes(graph, opts.is_directed);

    // start testing

    // print out parent vector

    Times t;
    t.start();
    GraphEngine graph_engine(engine_opts);
    graph_engine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;   

    GraphBase *graph = (&graph_engine)->create_graph_handle();

    cout << "hi" << endl;
    pvector<node_id_t> parent(graph->get_num_nodes(), -1);

    node_id_t start = find_random_start(*graph);
    parent[start] = start; // init parent... // eventually swap this find_random_start with any node from the largest connected component. 

    graph->close();


    BFS_EC(&myEngine, parent, THREAD_NUM) ;// how do we use graph?

    INFO();


    tearDown(graph);
}

int main(int argc, char *argv[])
{
    cout << "Running Edge-centric BFS" << endl;
    CmdLineApp bfs_cli(argc, argv);
    if (!bfs_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = bfs_cli.is_create_new();
    opts.is_directed = bfs_cli.is_directed();
    opts.read_optimize = bfs_cli.is_read_optimize();
    opts.is_weighted = bfs_cli.is_weighted();
    opts.optimize_create = bfs_cli.is_create_optimized();
    opts.db_name = bfs_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = bfs_cli.get_db_path();
    std::string bfs_log = bfs_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = bfs_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // bfs_cli.get_conn_config();
    opts.type = bfs_cli.get_graph_type();

    cout << "hi1" << endl;

    const int THREAD_NUM = 8;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    // int num_trials = bfs_cli.get_num_trials();

    // test
    test_bfs_ec();
    
    Times t;
    t.start();
    GraphEngine graph_engine(engine_opts);
    graph_engine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;   

    GraphBase *graph = (&graph_engine)->create_graph_handle();

    cout << "hi" << endl;
    pvector<node_id_t> parent(graph->get_num_nodes(), -1);

    node_id_t start = find_random_start(*graph);
    parent[start] = start; // init parent... // eventually swap this find_random_start with any node from the largest connected component. 

    graph->close();

    BFS_EC(&graph_engine, parent, THREAD_NUM);

    cout << "start BFS verifier" << endl;

    assert(BFSVerifier(&graph_engine, start, parent)); // test for correctness

    return 0;
}