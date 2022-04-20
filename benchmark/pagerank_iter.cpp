#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <vector>

#include "adj_list.h"
#include "command_line.h"
#include "common.h"
#include "edgekey.h"
#include "graph_exception.h"
#include "reader.h"
#include "standard_graph.h"

using namespace std;
const float dampness = 0.85;
int p_cur = 0;
int p_next = 1;

typedef struct pr_map
{
    int id;
    int in_deg;
    int out_deg;
    float p_rank[2];
    mutable std::shared_mutex mutex{};

} pr_map;

pr_map *ptr;  // pointer to mmap region

void calculate_node_offsets(int thread_max,
                            int num_nodes,
                            int node_offset_array[])
{
    int node_offset = 0;
    for (int i = 0; i < thread_max; i++)
    {
        node_offset_array[i] = node_offset;
        node_offset += num_nodes / thread_max;
    }
}

/**
 * @brief This function takes a single parameter - the number of nodes in the
 * graph. We then allocate a memory region of size N*sizeof(pr_map) and
 * initialize the memory region with 1/N.
 *
 * @param N
 */
void init_pr_map(int N)
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
    std::vector<node> nodes(N);
    graph.get_node_degrees(nodes);

#pragma omp parallel for
    for (int i = 0; i < N; i++)
    {
        ptr[i].id = -1;  // We are not assigning node_id's to the pr_map struct
                         // at this point.
        ptr[i].p_rank[0] = init_val;
        ptr[i].p_rank[1] = 0.0f;
        ptr[i].in_deg = nodes.at(i).in_deg;
        ptr[i].out_deg = nodes.at(i).out_deg;
    }
    nodes.clear();
    nodes.shrink_to_fit();  // free up memory. Not guaranteed to be honored.
}

void print_map(std::vector<node> &nodes)
{
    ofstream FILE;
    FILE.open("pr_out.txt", ios::out | ios::ate);
    for (node n : nodes)
    {
        FILE << n.id << "\t" << ptr[n.id].p_rank[p_next] << "\n";
    }
    FILE.close();
}

void delete_map() { munmap(ptr, sizeof(pr_map) * N); }

void print_to_csv(std::string name,
                  std::vector<int64_t> &times,
                  std::string csv_logdir)
{
    fstream FILE;
    std::string _name = csv_logdir + "/" + name + "_pr.csv";
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
template <typename Graph>
void pagerank(Graph &graph,
              graph_opts opts,
              int iterations,
              double tolerance,
              string csv_logdir)
{
    auto start = chrono::steady_clock::now();
    int num_nodes = graph.get_num_nodes();
    init_pr_map(num_nodes);
    auto end = chrono::steady_clock::now();
    cout << "Loading the nodes and constructing the map took "
         << to_string(chrono::duration_cast<chrono::microseconds>(end - start)
                          .count())
         << endl;

    std::vector<int64_t> times;
    times.push_back(
        chrono::duration_cast<chrono::microseconds>(end - start).count());

    double diff = 1.0;
    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;
    auto in_cursor = graph.get_innbd_cursor();  // Have to use type inference
                                                // coz of template
    while (iter_count < iterations)
    {
        auto start = chrono::steady_clock::now();
        int i = 0;

        adjlist found = {0};
        int index = 0;
        while (in_cursor.has_more())
        {
            in_cursor.next(&found);
            if (found.node_id != -1)
            {
                if (iter_count == 0)
                {
                    ptr[index].id = found.node_id;
                }
                float sum = 0.0f;
                for (int in_node : found.edgelist)
                {
                    std::shared_lock<std::shared_mutex> lock(
                        ptr[in_node].mutex);  // read lock
                    sum +=
                        (ptr[in_node].p_rank[p_cur]) / ptr[in_node].out_degree;
                }
                std::unique_lock<std::shared_mutex> lock(
                    ptr[index].mutex);  // write lock
                ptr[index].p_rank[p_next] = constant + (dampness * sum);
                index++;
            }
            else
            {
                break;
            }
        }
        iter_count++;
        in_cursor.reset();

        p_cur = 1 - p_cur;
        p_next = 1 - p_next;

        auto end = chrono::steady_clock::now();
        cout << "Iter " << iter_count << "took \t"
             << to_string(
                    chrono::duration_cast<chrono::microseconds>(end - start)
                        .count())
             << endl;
        times.push_back(
            chrono::duration_cast<chrono::microseconds>(end - start).count());
    }
    print_to_csv(opts.db_name, times, csv_logdir);
    print_map(nodes);
    delete_map();
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

    if (pr_cli.get_graph_type() == "std")
    {
        auto start = chrono::steady_clock::now();
        StandardGraph graph(opts);
        if (pr_cli.is_exit_on_create())  // Exit after creating the db
        {
            graph.close();
            exit(0);
        }
        auto end = chrono::steady_clock::now();
        cout << "Graph loaded in "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << endl;

        if (pr_cli.is_index_create())
        {
            start = chrono::steady_clock::now();
            graph.create_indices();
            end = chrono::steady_clock::now();
            cout << "Indices created in "
                 << chrono::duration_cast<chrono::microseconds>(end - start)
                        .count()
                 << endl;
            graph.close();
            exit(0);
        }

        // Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << endl;
        graph.close();
    }
    else if (pr_cli.get_graph_type() == "adj")
    {
        auto start = chrono::steady_clock::now();
        AdjList graph(opts);
        if (pr_cli.is_exit_on_create())  // Exit after creating the db
        {
            exit(0);
        }
        auto end = chrono::steady_clock::now();
        cout << "Graph loaded in "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << endl;
        // Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << endl;
        graph.close();
    }
    else if (pr_cli.get_graph_type() == "ekey")
    {
        auto start = chrono::steady_clock::now();
        EdgeKey graph(opts);
        if (pr_cli.is_exit_on_create())  // Exit after creating the db
        {
            exit(0);
        }
        auto end = chrono::steady_clock::now();
        cout << "Graph loaded in "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << endl;

        if (pr_cli.is_index_create())
        {
            start = chrono::steady_clock::now();
            graph.create_indices();
            end = chrono::steady_clock::now();
            cout << "Indices created in "
                 << chrono::duration_cast<chrono::microseconds>(end - start)
                        .count()
                 << endl;
            graph.close();
            exit(0);
        }

        // Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : "
             << chrono::duration_cast<chrono::microseconds>(end - start).count()
             << endl;
        graph.close();
    }
    else
    {
        std::cout << "Unrecognized graph representation";
    }
}
