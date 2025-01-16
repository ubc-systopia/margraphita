#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "common_util.h"
#include "graph_engine.h"
#include "reader.h"
#include "time_structs.h"
#include "times.h"
#include "wiredtiger.h"

std::vector<edge> failed_inserts;
std::mutex fail_lock;

void print_time_csvline(const graph_opts &params, time_info &t)
{
  std::ofstream log_file;
  std::string logfile_name = params.stat_log + "_api_tx.csv";

  log_file.open(logfile_name, std::fstream::app);

  struct stat st;
  stat(logfile_name.c_str(), &st);
  if (st.st_size == 0)
  {
    log_file
        << "DB path, type, is_readopt, num_nodes, num_edges, "
           "t_insert, t_rback, num_rback, num_fail, num_ins, num_threads\n";
  }

  log_file << params.db_dir << "/" << params.db_name << ","
           << get_type_str(params.type) << "," << params.read_optimize << ","
           << params.num_nodes << "," << params.num_edges << ","
           << t.insert_time << "," << t.rback_time << "," << t.num_rollbacks
           << "," << t.num_failures << "," << t.num_inserted << ","
           << params.num_threads << "\n";
  log_file.close();
}

void insert_edge_thread(int _tid,
                        std::string filename,
                        int num_threads,
                        int num_edges,
                        GraphBase *graph,
                        time_info &info)
{
  int tid = _tid;
  int num_per_chunk = (num_edges / num_threads);
  int beg = tid * num_per_chunk;

  Times outer, inner;
  outer.start();
  reader::EdgeReader graph_reader(
      std::move(filename), beg, num_per_chunk, "out");

  edge to_insert = {0};
  while (graph_reader.get_next_edge(to_insert) == 0)
  {
    int ret = graph->add_edge(to_insert, false);
    if (ret == 0)
    {
      info.num_inserted++;
    }
    else if (ret == WT_ROLLBACK)
    {
      inner.start();
      info.num_rollbacks++;
      bool succeeded = false;
      int tries = 3;
      do
      {
        ret = graph->add_edge(to_insert, false);
        if (ret == 0)
        {
          info.num_inserted++;
          succeeded = true;
        }
        else if (ret == WT_ROLLBACK)
        {
          tries--;
          info.num_rollbacks++;
        }
      } while (!succeeded && tries > 0);

      if (!succeeded && tries == 0)
      {
        info.num_failures++;
        fail_lock.lock();
        failed_inserts.push_back(to_insert);
        fail_lock.unlock();
      }
      inner.stop();
      info.rback_time += inner.t_micros();
    }
  }
  outer.stop();
  info.insert_time = outer.t_micros();

  std::cout << "Time spent in retrying rollbacks: " << info.rback_time << "us";
}

int main(int argc, char *argv[])
{
  time_info edge_insert_times;

  InsertOpts test_params(argc, argv);
  if (!test_params.parse_args())
  {
    test_params.print_help();
    return -1;
  }

  Times timer;
  timer.start();
  graph_opts opts = test_params.make_graph_opts();
  GraphEngine graphEngine(opts.num_threads, opts);
  // GraphBase *graph = graphEngine.create_graph_handle();

  timer.stop();
  std::cout << " Total time to create empty DB was " << timer.t_micros()

            << "us" << std::endl;

  // Now insert edges
  timer.start();
  int NUM_THREADS = opts.num_threads;
  std::cout << "Inserting edges with " << NUM_THREADS << " threads"
            << std::endl;

#pragma omp parallel for num_threads(NUM_THREADS)
  for (int i = 0; i < NUM_THREADS; i++)
  {
    GraphBase *graph = graphEngine.create_graph_handle();
    time_info this_thread_time;
    insert_edge_thread(i,
                       opts.dataset,
                       opts.num_threads,
                       opts.num_edges,
                       graph,
                       this_thread_time);
#pragma omp critical
    {
      edge_insert_times.insert_time += this_thread_time.insert_time;
      edge_insert_times.num_failures += this_thread_time.num_failures;
      edge_insert_times.num_rollbacks += this_thread_time.num_rollbacks;
      edge_insert_times.num_inserted += this_thread_time.num_inserted;
      edge_insert_times.rback_time += this_thread_time.rback_time;
    }
  }
  timer.stop();

  std::cout << "Time to insert in threads: " << timer.t_secs() << "s" << endl;

  GraphBase *graph = graphEngine.create_graph_handle();
  timer.start();

  for (edge x : failed_inserts)
  {
    graph->add_edge(x, false);
  }
  timer.stop();
  cout << "Inserted the failed edges in : " << timer.t_micros() << endl;

  std::cout << "Insertion summary (summed for all threads):\n";
  std::cout << "Number (nodes, edges) in graph : " << opts.num_nodes << ", "
            << opts.num_edges << std::endl;
  std::cout << "Number of edges inserted: " << edge_insert_times.num_inserted
            << std::endl;
  std::cout << "Number of Rollbacks: " << edge_insert_times.num_rollbacks
            << endl;
  std::cout << "Number of failures: " << edge_insert_times.num_failures << endl;
  std::cout << "Time spent in retrying rollbacks: "

            << edge_insert_times.rback_time << " us" << endl;
  std::cout << "Total time taken : " << edge_insert_times.insert_time << "us"

            << endl;

  // Adjust for threads
  edge_insert_times.rback_time = edge_insert_times.rback_time / NUM_THREADS;
  edge_insert_times.insert_time = edge_insert_times.insert_time / NUM_THREADS;

  print_time_csvline(opts, edge_insert_times);

  cout << "-------------------" << endl;
  for (edge x : failed_inserts)
  {
    CommonUtil::dump_edge(x);
  }

  graphEngine.close_graph();
  return (EXIT_SUCCESS);
}