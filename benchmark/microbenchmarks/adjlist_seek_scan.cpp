#include <times.h>

#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/vector.hpp>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

#include "adj_list.h"
#include "common_util.h"
#include "graph_engine.h"

struct time_result
{
  long double time_seek;
  long double time_scan;
  degree_t degree;
};

struct time_result seek_and_scan(node_id_t vertex,
                                 AdjList &graph,
                                 WT_SESSION *session)
{
  struct time_result results;
  WT_CURSOR *adj_cursor = graph.get_out_adjlist_cursor();

  // seek
  Times timer;
  timer.start();
  CommonUtil::set_key(adj_cursor, vertex);
  int ret = adj_cursor->search(adj_cursor);
  if (ret != 0)
  {
    std::cout << "Vertex " << vertex << " not found" << std::endl;
    exit(1);
  }
  timer.stop();
  results.time_seek = timer.t_nanos();

  // scan
  adjlist adj_list;
  timer.start();
  CommonUtil::record_to_adjlist(session, adj_cursor, &adj_list);
  std::cout << " Vertex " << vertex << " has edges: [";
  for ([[maybe_unused]] int node : adj_list.edgelist)
  {
    std::cout << node << " ";
  }
  std::cout << "]" << std::endl;
  timer.stop();
  results.time_scan = timer.t_nanos();
  results.degree = adj_list.edgelist.size();

  return results;
}

bool exists_file(const char *name)
{
  ifstream f(name);
  return f.good();
}

void profile_wt_adjlist(const filesystem::path &graphfile,
                        WT_SESSION *session,
                        int samples,
                        AdjList &graph)
{
  std::vector<node_id_t> random_ids;
  WT_CURSOR *random_out_adj_cursor = graph.get_new_random_outadj_cursor();
  int num = 0;
  while (num < samples)
  {
    random_out_adj_cursor->next(random_out_adj_cursor);
    node rando;
    CommonUtil::get_key(random_out_adj_cursor, &rando.id);
    if (graph.get_out_degree(rando.id) == 0)
      continue;  // skip vertices with no edges
    random_ids.push_back(rando.id);
    num++;
  }
  assert(random_ids.size() == 1000);
  random_out_adj_cursor->close(random_out_adj_cursor);

  // write random ids to file
  char random_id_file_name[256];
  sprintf(random_id_file_name, "%s_random_ids.bin", graphfile.stem().c_str());
  std::ofstream random_id_file =
      std::ofstream(random_id_file_name, std::ios::out | std::ios::binary);
  boost::archive::binary_oarchive oa(random_id_file);
  oa << random_ids;
  random_id_file.close();

  // create file for adjlist seek and scan times results
  char outfile_name[256];
  sprintf(outfile_name, "%s_adjlist_ubench.txt", graphfile.stem().c_str());

  std::fstream adjlist_seek_scan_outfile;
  if (!exists_file(outfile_name))
  {
    adjlist_seek_scan_outfile.open(outfile_name, std::ios::out);
    adjlist_seek_scan_outfile
        << "vertex_id,degree,seek_time_ns,scan_time_per_edge_ns" << std::endl;
  }
  else
  {
    adjlist_seek_scan_outfile.open(outfile_name, std::ios::out | std::ios::app);
  }

  for (node_id_t sample : random_ids)
  {
    struct time_result time = seek_and_scan(sample, graph, session);
    adjlist_seek_scan_outfile << sample << "," << time.degree << ","
                              << time.time_seek << ","
                              << (time.time_scan / time.degree) << std::endl;
  }
  adjlist_seek_scan_outfile.close();
}

int main(int argc, char *argv[])
{
  if (argc != 4)
  {
    std::cout << "Usage: ./ubenchmark <graphfile> <wt_db_dir> <wt_db_name>"
              << std::endl;
    return 0;
  }

  cpu_set_t mask;
  int status;

  CPU_ZERO(&mask);
  CPU_SET(6, &mask);
  status = sched_setaffinity(0, sizeof(mask), &mask);
  if (status != 0)
  {
    perror("sched_setaffinity");
  }

  std::filesystem::path graphfile(argv[1]);
  string wt_db_dir(argv[2]);
  std::string wt_db_name = argv[3];
  [[maybe_unused]] int num_random_samples = 1000;

  const int THREAD_NUM = 1;
  graph_opts opts;
  opts.create_new = false;
  opts.optimize_create = false;
  opts.is_directed = true;
  opts.read_optimize = true;
  opts.is_weighted = true;
  opts.type = GraphType::Adj;
  opts.db_dir = wt_db_dir;
  opts.db_name = wt_db_name;
  opts.conn_config = "cache_size=10GB";
  if (const char *env_p = std::getenv("GRAPH_PROJECT_DIR"))
  {
    opts.stat_log = std::string(env_p);
  }
  else
  {
    std::cout << "GRAPH_PROJECT_DIR not set. Using CWD" << std::endl;
    opts.stat_log = "./";
  }

  GraphEngine myEngine(THREAD_NUM, opts);
  WT_CONNECTION *conn = myEngine.get_connection();
  WT_SESSION *session;
  if (CommonUtil::open_session(conn, &session) != 0)
  {
    throw GraphException("Cannot open session");
  }
  AdjList graph(opts, conn);
  graph.init_cursors();

  profile_wt_adjlist(graphfile, session, num_random_samples, graph);
}
