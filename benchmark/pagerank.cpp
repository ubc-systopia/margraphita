#include <math.h>
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
#include "times.h"

using namespace std;
const float dampness = 0.85;
std::hash<int> hashfn;
int N = 1610612741;  // Hash bucket size
int p_cur = 0;
int p_next = 1;
typedef struct pr_map
{
    int id;
    float p_rank[2];
} pr_map;

pr_map *ptr;  // pointer to mmap region
// float *pr_cur, *pr_next;

void init_pr_map(std::vector<node> &nodes)
{
    int size = nodes.size();
    // cout << size;
    ptr = (pr_map *)mmap(NULL,
                         sizeof(pr_map) * N,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS,
                         0,
                         0);
    // since the access pattern is random and because the node id's
    // non-continuous we cannot do any clever madvise tricks.

    float init_val = 1.0f / size;

    for (node n : nodes)
    {
        if (ptr[n.id].id == 0)
        {
            ptr[n.id].id = n.id;
            ptr[n.id].p_rank[p_cur] = init_val;
            ptr[n.id].p_rank[p_next] = 0.0f;
        }
    }
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

template <typename Graph>
void pagerank(Graph &graph,
              graph_opts opts,
              int iterations,
              double tolerance,
              string csv_logdir)
{
    int num_nodes = graph.get_num_nodes();
    auto start = chrono::steady_clock::now();
    std::vector<node> nodes = graph.get_nodes();
    init_pr_map(nodes);
    std::vector<int64_t> times;
    auto end = chrono::steady_clock::now();
    cout << "Loading the nodes and constructing the map took "
         << to_string(chrono::duration_cast<chrono::microseconds>(end - start)
                          .count())
         << endl;
    times.push_back(
        chrono::duration_cast<chrono::microseconds>(end - start).count());

    double diff = 1.0;
    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;

    while (iter_count < iterations)
    {
        auto start = chrono::steady_clock::now();
        for (node n : nodes)
        {
            int index = hashfn(n.id) % N;
            float sum = 0.0f;
            vector<node> in_nodes = graph.get_in_nodes(
                n.id);  // <-- make this just list of node_ids to avoid looking
                        // up node table

            for (node in : in_nodes)
            {
                sum += (ptr[hashfn(in.id) % N].p_rank[p_cur]) / in.out_degree;
            }
            ptr[index].p_rank[p_next] = constant + (dampness * sum);
        }
        iter_count++;

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

    GraphBase* graph = nullptr;
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

    Times Timer;
    Timer.start();
    if(pr_cli.get_graph_type() == "std") {
        graph = new StandardGraph(opts);
    } else if(pr_cli.get_graph_type() == "adj") {
        graph = new AdjList(opts);
    } else if(pr_cli.get_graph_type() == "ekey") {
        graph = new EdgeKey(opts);
    } else {
        std::cout << "Unrecognized graph representation";
        //exit(0);
    }
    if (pr_cli.is_exit_on_create())  // Exit after creating the db
    {
        graph->close();
        exit(0);
    }
    Timer.stop();
    cout << "Graph loaded in "
         << Timer.t_micros()
         << endl;

    if ((pr_cli.get_graph_type() == "std" || pr_cli.get_graph_type() == "ekey") 
        && pr_cli.is_index_create()) {
        Timer.start();
        graph->make_indexes();
        Timer.stop();
        cout << "Indices created in "
                << Timer.t_micros()
                << endl;
        graph->close();
        exit(0);
    }

    // Now run PR
    Timer.start();
    pagerank(*graph, opts, pr_cli.iterations(), pr_cli.tolerance(), pr_log);
    Timer.stop();
    cout << "PR  completed in : "
            << Timer.t_micros()
            << endl;
    graph->close();
}
