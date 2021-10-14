#include <stdio.h>
#include <vector>
#include <math.h>
#include <chrono>
#include <unistd.h>
#include <cassert>
#include "common.h"
#include "graph_exception.h"
#include "standard_graph.h"
#include "adj_list.h"
#include "edgekey.h"
#include <sys/mman.h>
#include "command_line.h"
#include "logger.h"
#include "reader.h"
#include <thread>
#include <omp.h>

using namespace std;
const float dampness = 0.85;
std::hash<int> hashfn;
int N = 1610612741; //Hash bucket size
int p_cur = 0;
int p_next = 1;
typedef struct pr_map
{
    int id;
    float p_rank[2];
} pr_map;
pr_map *ptr;
std::vector<node> nodes;

void init_pr_map(std::vector<node> &nodes)
{

    int size = nodes.size();
    //cout << size;
    ptr = (pr_map *)mmap(NULL, sizeof(pr_map) * N, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    float init_val = 1.0f / size;
    int i = 0;
    for (node n : nodes)
    {

        int index = hashfn(n.id) % N;
        if (ptr[index].id == 0)
        {
            ptr[index].id = n.id;
            ptr[index].p_rank[p_cur] = init_val;
            ptr[index].p_rank[p_next] = 0.0f;
        }
        else
        {
            cout << "collision at element " << n.id;
        }
        i++;
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

void delete_map()
{
    munmap(ptr, sizeof(pr_map) * 1610612741);
}

void pagerank(graph_opts opts, int iterations, double tolerance)
{
    cout << "hello";
    StandardGraph graph(opts);
    int num_nodes = graph.get_num_nodes();
    auto start = chrono::steady_clock::now();
    init_pr_map(nodes);
    std::vector<int64_t> times;

    double diff = 1.0;
    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;
    // std::vector<StandardGraph> db_conns;
    // int tid = 0;
    // while (tid < NUM_THREADS)
    // {
    //     db_conns.push_back(StandardGraph(opts));
    //     tid++;
    // }
    while (iter_count < iterations)
    {
        auto start = chrono::steady_clock::now();
        int i = 0;
#pragma omp parallel num_threads(NUM_THREADS)
        {
#pragma omp for
            for (node n : nodes)
            {
                int index = hashfn(n.id) % N;
                float sum = 0.0f;
                StandardGraph g(opts);
                vector<node> in_nodes = g.get_in_nodes(n.id);

                for (node in : in_nodes)
                {
                    sum += (ptr[hashfn(in.id) % N].p_rank[p_cur]) / in.out_degree;
                }
                ptr[index].p_rank[p_next] = constant + (dampness * sum);
                i++;
            }
        }
        iter_count++;

        p_cur = 1 - p_cur;
        p_next = 1 - p_next;

        auto end = chrono::steady_clock::now();
        cout << "Iter " << iter_count << "took \t" << to_string(chrono::duration_cast<chrono::microseconds>(end - start).count()) << endl;
        times.push_back(chrono::duration_cast<chrono::microseconds>(end - start).count());
    }
    //print_to_csv(graph.get_db_name(), times);
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
    opts.db_name = pr_cli.get_db_name();
    opts.db_dir = pr_cli.get_db_path();

    // open connection to WT
    std::filesystem::path dirname = opts.db_dir + "/" + opts.db_name;
    int ret = CommonUtil::open_connection(const_cast<char *>(dirname.c_str()), &opts.conn);
    if (ret < 0)
    {
        std::cout << "Failed to open a connection to the DB" << std::endl;
        exit(-1);
    };

    ret = CommonUtil::open_session(opts.conn, &opts.session);
    if (ret != 0)
    {
        cout << "did not work";
    }

    pagerank(opts, pr_cli.iterations(), pr_cli.tolerance());

    // auto start = chrono::steady_clock::now();

    // graph.create_indices();
    // auto end = chrono::steady_clock::now();
    // cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

    // //Now run PR
    // start = chrono::steady_clock::now();

    //

    // end = chrono::steady_clock::now();
    // cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
    // graph.close();
}
