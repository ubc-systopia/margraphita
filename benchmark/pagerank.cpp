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

pr_map *ptr; // pointer to mmap region
//float *pr_cur, *pr_next;

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

template <typename Graph>
void pagerank(Graph &graph, graph_opts opts, int iterations, double tolerance)
{

    int num_nodes = graph.get_num_nodes();
    auto start = chrono::steady_clock::now();
    std::vector<node> nodes = graph.get_nodes();
    init_pr_map(nodes);
    auto end = chrono::steady_clock::now();
    cout << "Loading the nodes and constructing the map took " << to_string(chrono::duration_cast<chrono::microseconds>(end - start).count()) << endl;

    for (node n : nodes)
    {
        assert(ptr[hashfn(n.id)].id == n.id);
    }

    double diff = 1.0;
    int iter_count = 0;
    float constant = (1 - dampness) / num_nodes;
    //printf("at: %d \n", __LINE__);
    while (iter_count < iterations)
    {
        auto start = chrono::steady_clock::now();
        for (node n : nodes)
        {
            int index = hashfn(n.id) % N;
            float sum = 0.0f;
            vector<node> in_nodes = graph.get_in_nodes(n.id);
            assert(in_nodes.size() == n.in_degree);
            for (node in : in_nodes)
            {
                cout << "here\n";
                sum += (ptr[hashfn(in.id) % N].p_rank[p_cur]) / in.out_degree;
            }
            ptr[index].p_rank[p_next] = constant + (dampness * sum);
        }
        iter_count++;

        p_cur = 1 - p_cur;
        p_next = 1 - p_next;

        auto end = chrono::steady_clock::now();
        cout << "Iter " << iter_count << "took \t" << to_string(chrono::duration_cast<chrono::microseconds>(end - start).count()) << endl;
    }
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

    if (pr_cli.get_graph_type() == "std")
    {

        Logger *logger = new Logger("pr_run", opts.db_name, "pagerank", pr_cli.get_dataset());

        auto start = chrono::steady_clock::now();

        StandardGraph graph(opts);
        if (pr_cli.is_exit_on_create()) // Exit after creating the db
        {
            exit(0);
        }

        auto end = chrono::steady_clock::now();
        // logger->out("Graph loaded in " + chrono::duration_cast<chrono::microseconds>(end - start).count());
        cout << "Graph loaded in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

        // We assume here that the edges have already been inserted into the
        // graph. We just create indices here. Adjlist does not have any
        // indices.
        start = chrono::steady_clock::now();
        if (pr_cli.get_graph_type() == "std" || pr_cli.get_graph_type() == "edgekey")
        {
            //graph.create_indices();
        }
        end = chrono::steady_clock::now();
        cout << "Indices created in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

        //Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else if (pr_cli.get_graph_type() == "adjlist")
    {
        Logger *logger = new Logger("pr_run", opts.db_name, "pagerank", pr_cli.get_dataset());

        AdjList graph(opts);
        if (pr_cli.is_exit_on_create()) // Exit after creating the db
        {
            exit(0);
        }
        //Now run PR
        auto start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        auto end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else if (pr_cli.get_graph_type() == "edgekey")
    {
        Logger *logger = new Logger("pr_run", opts.db_name, "pagerank", pr_cli.get_dataset());

        EdgeKey graph(opts);
        if (pr_cli.is_exit_on_create()) // Exit after creating the db
        {
            exit(0);
        }
        // We assume here that the edges have already been inserted into the
        // graph. We just create indices here. Adjlist does not have any
        // indices.
        auto start = chrono::steady_clock::now();
        if (pr_cli.get_graph_type() == "std" || pr_cli.get_graph_type() == "edgekey")
        {
            graph.create_indices();
        }
        auto end = chrono::steady_clock::now();
        cout << "Indices created in " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;

        //Now run PR
        start = chrono::steady_clock::now();
        pagerank(graph, opts, pr_cli.iterations(), pr_cli.tolerance(), logger);
        end = chrono::steady_clock::now();
        cout << "PR  completed in : " << chrono::duration_cast<chrono::microseconds>(end - start).count() << endl;
        // logger->out("PR  completed in : " +
        // chrono::duration_cast<chrono::microseconds>(end - start).count());
        graph.close();
    }
    else
    {
        std::cout << "Unrecognized graph representation";
    }
}
