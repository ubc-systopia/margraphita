#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "reader.h"
#include "standard_graph.h"
#include "time_structs.h"
#include "times.h"

void print_time_csvline(std::string db_name,
                        std::string db_path,
                        time_info *edget,
                        time_info *nodet,
                        bool is_readopt,
                        std::string logfile_name,
                        std::string type)
{
    std::ofstream log_file;
    log_file.open(logfile_name, std::fstream::app);

    struct stat st;
    stat(logfile_name.c_str(), &st);
    if (st.st_size == 0)
    {
        log_file << "db_name, db_path, type, is_readopt, num_nodes, num_edges, "
                    "t_e_read, t_e_insert, t_n_read, t_n_insert\n";
    }

    log_file << db_name << "," << db_path << "/" << db_name << "," << type
             << "," << is_readopt << "," << nodet->num_inserted << ","
             << edget->num_inserted << "," << edget->read_time << ","
             << edget->insert_time << "," << nodet->read_time << ","
             << nodet->insert_time << "\n";
    log_file.close();
}

time_info *insert_edge_thread(int _tid, std::string dataset, GraphBase *graph)
{
    int tid = _tid;  //*(int *)arg;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);
    time_info *info = new time_info(0);

    Times t;
    t.start();
    reader::EdgeReader graph_reader(filename);
    edge to_insert = {0};
    while (graph_reader.get_next_edge(to_insert) == 0)
    {
        graph->add_edge(to_insert, false);
    }
    t.stop();
    info->read_time = t.t_micros();
    // info->num_inserted = edjlist.size();

    t.start();

    return info;
}

int main(int argc, char *argv[])
{
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
            insert_edge_thread(i, test_params.get_dataset(), graph);
    }

    return (EXIT_SUCCESS);
}