#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common_util.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "reader.h"
#include "standard_graph.h"
#include "time_structs.h"
#include "times.h"
#include "wiredtiger.h"

std::vector<edge> failed_inserts;
std::mutex fail_lock;

void print_time_csvline(InsertOpts params, time_info *t)
{
    std::ofstream log_file;
    std::string logfile_name =
        params.get_logdir() + "/" + params.get_db_name() + "_api_notx.csv";
    log_file.open(logfile_name, std::fstream::app);

    struct stat st;
    stat(logfile_name.c_str(), &st);
    if (st.st_size == 0)
    {
        log_file
            << "DB path, type, is_readopt, num_nodes, num_edges, "
               "t_insert, t_rback, num_rback, num_fail, num_ins, num_threads\n";
    }

    log_file << params.get_db_path() << "/" << params.get_db_name() << ","
             << params.get_type_str() << "," << params.is_read_optimize() << ","
             << params.get_num_nodes() << "," << params.get_num_edges() << ","
             << t->insert_time << "," << t->rback_time << ","
             << t->num_rollbacks << "," << t->num_failures << ","
             << t->num_inserted << "," << params.get_num_threads() << "\n";
    log_file.close();
}

time_info *insert_edge_thread(int _tid,
                              std::string filename,
                              int num_threads,
                              int num_edges,
                              GraphBase *graph)
{
    int tid = _tid;
    int chunk_size = (num_edges / num_threads);
    int beg = tid * chunk_size;

    time_info *info = new time_info(0);

    Times inner;
    inner.start();
    reader::EdgeReader graph_reader(filename, beg, chunk_size);
    edge to_insert = {0};
    while (graph_reader.get_next_edge(to_insert) == 0)
    {
        graph->add_edge(to_insert, false);
        info->num_inserted++;
    }
    inner.stop();
    info->insert_time = inner.t_micros();
    return info;
}

int main(int argc, char *argv[])
{
    time_info *edge_insert_times = new time_info(0);
    InsertOpts test_params(argc, argv);
    if (!test_params.parse_args())
    {
        test_params.print_help();
        return -1;
    }

    Times timer;
    timer.start();

    GraphEngine::graph_engine_opts engine_opts{
        .num_threads = test_params.get_num_threads(),
        .opts = test_params.make_graph_opts()};

    GraphEngine graphEngine(engine_opts);
    // GraphBase *graph = graphEngine.create_graph_handle();

    timer.stop();
    std::cout << " Total time to create empty DB was " << timer.t_micros()
              << std::endl;
    // Now insert edges
    timer.start();
    int NUM_THREADS = test_params.get_num_threads();
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        GraphBase *graph = graphEngine.create_graph_handle();
        time_info *this_thread_time =
            insert_edge_thread(i,
                               test_params.get_dataset(),
                               test_params.get_num_threads(),
                               test_params.get_num_edges(),
                               graph);
#pragma omp critical
        {
            edge_insert_times->insert_time += this_thread_time->insert_time;
            edge_insert_times->num_failures += this_thread_time->num_failures;
            edge_insert_times->num_rollbacks += this_thread_time->num_rollbacks;
            edge_insert_times->num_inserted += this_thread_time->num_inserted;
            edge_insert_times->rback_time += this_thread_time->rback_time;
        }
    }
    timer.stop();

    std::cout << "Time to insert in threads: " << timer.t_micros() << endl;

    timer.start();
    GraphBase *graph = graphEngine.create_graph_handle();
    for (edge x : failed_inserts)
    {
        graph->add_edge(x, false);
    }
    timer.stop();
    cout << "Inserted the failed edges in : " << timer.t_micros() << endl;

    std::cout << "Insertion summary (summed for all threads):\n";
    std::cout << "Number (nodes, edges) in graph : "
              << test_params.get_num_nodes() << ", "
              << test_params.get_num_edges() << std::endl;
    std::cout << "Number of edges inserted: " << edge_insert_times->num_inserted
              << std::endl;
    std::cout << "Number of Rollbacks: " << edge_insert_times->num_rollbacks
              << endl;
    std::cout << "Number of failures: " << edge_insert_times->num_failures
              << endl;
    std::cout << "Time spent in retrying rollbacks: "
              << edge_insert_times->rback_time << endl;
    std::cout << "Total time taken : " << edge_insert_times->insert_time
              << endl;

    // Adjust for threads
    edge_insert_times->rback_time = edge_insert_times->rback_time / NUM_THREADS;
    edge_insert_times->insert_time =
        edge_insert_times->insert_time / NUM_THREADS;

    print_time_csvline(test_params, edge_insert_times);

    cout << "-------------------" << endl;
    for (edge x : failed_inserts)
    {
        CommonUtil::dump_edge(x);
    }
    return (EXIT_SUCCESS);
}