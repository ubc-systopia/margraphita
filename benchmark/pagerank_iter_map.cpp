#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <sstream>
#include <vector>

#include "GraphCreate.h"
#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "pvector.h"
#include "standard_graph.h"
#include "thread_utils.h"
#include "times.h"

const float dampness = 0.85;
int p_cur = 0;
int p_next = 1;
typedef float ScoreT;
typedef struct pr_map
{
    int id;
    int in_deg;
    int out_deg;
    float p_rank[2];
    mutable std::shared_mutex mutex{};

} pr_map;

pr_map *ptr;  // pointer to mmap region

/**
 * @brief This function takes a single parameter - the number of nodes in
 * the graph. We then allocate a memory region of size N*sizeof(pr_map) and
 * initialize the memory region with 1/N.
 *
 * @param N
 */
void init_pr_map(int N, std::vector<node> &nodes)
{
    ptr = (pr_map *)mmap(NULL,
                         sizeof(pr_map) * N,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED | MAP_ANONYMOUS,
                         -1,
                         0);  // Should I make this file-backed?
    if (ptr == MAP_FAILED)
    {
        perror("mmap failed");
        exit(1);
    }
    int ret =
        madvise(ptr,
                sizeof(pr_map) * N,
                MADV_SEQUENTIAL);  // we are guranteed to read this sequentially
    if (ret != 0)
    {
        fprintf(
            stderr, "madvise failed with error code: %s\n", strerror(errno));
        exit(1);
    }
    float init_val = 1.0f / N;

#pragma omp parallel for
    for (int i = 0; i < N; i++)
    {
        ptr[i].id = nodes.at(i).id;  // We are not assigning node_id's to the
                                     // pr_map struct at this point.
        ptr[i].p_rank[0] = init_val;
        ptr[i].p_rank[1] = 0.0f;
        ptr[i].in_deg = nodes.at(i).in_degree;
        ptr[i].out_deg = nodes.at(i).out_degree;
    }
}

void print_map(int N)
{
    ofstream FILE;
    FILE.open("pr_out.txt", ios::out | ios::ate);
    for (int i = 0; i < N; i++)
    {
        FILE << ptr[i].id << "\t" << ptr[i].p_rank[p_next] << "\n";
    }
    FILE.close();
}

void delete_map(int N) { munmap(ptr, sizeof(pr_map) * N); }

void print_to_csv(std::string name,
                  std::vector<double> &times,
                  std::string csv_logdir)
{
    fstream FILE;
    std::string _name = csv_logdir + "/" + name + "_pr_iter_map.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name,bmark,map_t,i0,i1,i2,i3,i4,i5,i6,i7,i8,i9"
             << "\n ";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",pr,";
    for (int i = 0; i < times.size(); i++)
    {
        FILE << times[i];
        if (i != times.size() - 1)
        {
            FILE << ",";
        }
        else
        {
            FILE << "\n";
        }
    }
    FILE.close();
}

/**
 * !If we want to parallelize this function, we need per-thread offsets into the
 * pr_map. init_pr_map would need to be called prior to this function. Extend
 * PR_map to have a lock.
 */
void pagerank(GraphBase *graph,
              graph_opts opts,
              int iterations,
              double tolerance,
              string csv_logdir)
{
    Times t;
    t.start();
    std::vector<node> nodes = graph->get_nodes();
    int num_nodes = nodes.size();
    init_pr_map(num_nodes, nodes);
    nodes.clear();
    std::vector<node>().swap(nodes);
    t.stop();
    cout << "Loading the nodes and constructing the map took " << t.t_micros()
         << endl;
    std::vector<double> times;
    times.push_back(t.t_micros());

    double diff = 1.0;
    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;

    while (iter_count < iterations)
    {
        t.start();
        int i = 0;

        int index = 0;
        adjlist found = {0};
        node curr_node = {0};
        InCursor *in_cursor = graph->get_innbd_iter();
        in_cursor->next(&found);

        while (found.node_id != -1)
        {
            float sum = 0.0f;
            for (auto in_node : found.edgelist)
            {
                sum += (ptr[in_node].p_rank[p_cur]) / ptr[in_node].out_deg;
            }
            ptr[index].p_rank[p_next] = constant + (dampness * sum);
            index++;
            in_cursor->next(&found);
        }
        iter_count++;

        p_cur = 1 - p_cur;
        p_next = 1 - p_next;

        t.stop();
        cout << "Iter " << iter_count << "took \t" << t.t_micros()
             << "(nodes = " << index << ")" << endl;
        times.push_back(t.t_micros());
    }
    print_to_csv(opts.db_name, times, csv_logdir);
    print_map(num_nodes);
    delete_map(num_nodes);
}

int main(int argc, char *argv[])
{
    cout << "Running PageRank" << endl;
    PageRankOpts pr_cli(argc, argv, 1e-4, 10);
    if (!pr_cli.parse_args())
    {
        return -1;
    }

    graph_opts opts;
    opts.create_new = pr_cli.is_create_new();
    opts.is_directed = pr_cli.is_directed();
    opts.read_optimize = pr_cli.is_read_optimize();
    opts.is_weighted = pr_cli.is_weighted();
    opts.optimize_create = pr_cli.is_create_optimized();
    opts.db_name = pr_cli.get_db_name();  //${type}_rd_${ds}
    opts.db_dir = pr_cli.get_db_path();
    std::string pr_log = pr_cli.get_logdir();  //$RESULT/$bmark
    opts.stat_log = pr_log + "/" + opts.db_name;
    opts.conn_config = "cache_size=10GB";  // tc_cli.get_conn_config();
    opts.type = pr_cli.get_graph_type();

    Times t;
    t.start();
    GraphFactory f;
    GraphBase *graph = f.CreateGraph(opts);
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    // Now run PR
    t.start();
    pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
    t.stop();
    cout << "PR  completed in : " << t.t_micros() << endl;
    graph->close();
}