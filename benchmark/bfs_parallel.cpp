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
#include "benchmark_definitions.h"
#include "bitmap.h"
#include "command_line.h"
#include "common.h"
#include "csv_log.h"
#include "edgekey.h"
#include "graph_engine.h"
#include "graph_exception.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

int64_t BUStep(GraphEngine *graph_engine,
               pvector<node_id_t> &parent,
               Bitmap &front,
               Bitmap &next,
               int thread_num)
{
    int64_t awake_count = 0;
    next.reset();

#pragma omp parallel for reduction(+ : awake_count) num_threads(thread_num)
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase *graph = graph_engine->create_graph_handle();
        InCursor *in_cursor = graph->get_innbd_iter();
        adjlist found = {0};

        in_cursor->set_key_range(graph_engine->get_key_range(i));

        in_cursor->next(&found);

        while (found.node_id != -1)
        {
            if (parent[found.node_id] < 0)
            {
                for (node_id_t v : found.edgelist)
                {
                    if (front.get_bit(v))
                    {
                        parent[found.node_id] = v;
                        awake_count++;
                        next.set_bit(found.node_id);
                        break;
                    }
                }
            }
            in_cursor->next(&found);
        }
        graph->close();
    }

    return awake_count;
}

int64_t TDStep(GraphEngine *graph_engine,
               pvector<node_id_t> &parent,
               SlidingQueue<node_id_t> &queue)
{
    int64_t scout_count = 0;

#pragma omp parallel
    {
        QueueBuffer<node_id_t> lqueue(queue);
#pragma omp for reduction(+ : scout_count) nowait
        for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
        {
            node_id_t u = *q_iter;
            GraphBase *graph = graph_engine->create_graph_handle();
            for (node_id_t v : graph->get_out_nodes_id(u))
            {
                node_id_t curr_val = parent[v];
                if (curr_val < 0)
                {
                    if (compare_and_swap(parent[v], curr_val, u))
                    {
                        lqueue.push_back(v);
                        scout_count += -curr_val;
                    }
                }
            }
            graph->close();
        }
        lqueue.flush();
    }
    return scout_count;
}

void QueueToBitmap(const SlidingQueue<node_id_t> &queue, Bitmap &bm)
{
#pragma omp parallel for
    for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
    {
        node_id_t u = *q_iter;
        bm.set_bit_atomic(u);
    }
}

void BitmapToQueue(GraphEngine *graph_engine,
                   const Bitmap &bm,
                   SlidingQueue<node_id_t> &queue,
                   int thread_num)
{
#pragma omp parallel num_threads(thread_num)
    {
        QueueBuffer<node_id_t> lqueue(queue);
#pragma omp for nowait
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase *graph = graph_engine->create_graph_handle();
            NodeCursor *node_cursor = graph->get_node_iter();
            node found = {0};

            node_cursor->set_key_range(graph_engine->get_key_range(i));

            node_cursor->next(&found);

            while (found.id != -1)
            {
                if (bm.get_bit(found.id)) lqueue.push_back(found.id);
                node_cursor->next(&found);
            }
            graph->close();
        }
        lqueue.flush();
    }
    queue.slide_window();
}

pvector<node_id_t> InitParent(GraphEngine *graph_engine, int thread_num)
{
    GraphBase *graph_stat = graph_engine->create_graph_handle();
    pvector<node_id_t> parent(graph_stat->get_num_nodes());
    graph_stat->close();

#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase *graph = graph_engine->create_graph_handle();
        NodeCursor *node_cursor = graph->get_node_iter();
        node found = {0};

        node_cursor->set_key_range(graph_engine->get_key_range(i));

        node_cursor->next(&found);

        while (found.id != -1)
        {
            parent[found.id] = graph->get_out_degree(found.id) != 0
                                   ? -graph->get_out_degree(found.id)
                                   : -1;
            node_cursor->next(&found);
        }
        graph->close();
    }
    return parent;
}

pvector<node_id_t> DOBFS(GraphEngine *graph_engine,
                         node_id_t source,
                         int thread_num = 1,
                         int alpha = 15,
                         int beta = 18)
{
    GraphBase *graph_stat = graph_engine->create_graph_handle();
    node_id_t num_nodes = graph_stat->get_num_nodes();
    pvector<node_id_t> parent = InitParent(graph_engine, thread_num);
    parent[source] = source;
    SlidingQueue<node_id_t> queue(num_nodes);
    queue.push_back(source);
    queue.slide_window();
    Bitmap curr(num_nodes);
    curr.reset();
    Bitmap front(num_nodes);
    front.reset();
    int64_t edges_to_check = graph_stat->get_num_edges();
    int64_t scout_count = graph_stat->get_out_degree(source);

    while (!queue.empty())
    {
        if (scout_count > edges_to_check / alpha)
        {
            int64_t awake_count, old_awake_count;
            QueueToBitmap(queue, front);
            awake_count = queue.size();
            queue.slide_window();
            do
            {
                old_awake_count = awake_count;
                awake_count =
                    BUStep(graph_engine, parent, front, curr, thread_num);
                front.swap(curr);
            } while ((awake_count >= old_awake_count) ||
                     (awake_count > num_nodes / beta));
            BitmapToQueue(graph_engine, front, queue, thread_num);
            scout_count = 1;
        }
        else
        {
            edges_to_check -= scout_count;
            scout_count = TDStep(graph_engine, parent, queue);
            queue.slide_window();
        }
    }

#pragma omp parallel for
    for (node_id_t n = 0; n < num_nodes; n++)
        if (parent[n] < -1) parent[n] = -1;

    graph_stat->close();
    return parent;
}

int main(int argc, char *argv[])
{
    cout << "Running BFS" << endl;
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
    node_id_t start_vertex = bfs_cli.start_vertex();
    const int THREAD_NUM = 16;
    GraphEngine::graph_engine_opts engine_opts{.num_threads = THREAD_NUM,
                                               .opts = opts};

    bfs_info info(0);

    Times t;
    t.start();
    GraphEngine graphEngine(engine_opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    t.start();
    DOBFS(&graphEngine, start_vertex, THREAD_NUM);
    t.stop();
    info.time_taken = t.t_micros();
    std::cout << "BFS finished in " << t.t_micros() << std::endl;

    print_csv_info(opts.db_name, start_vertex, info, start_vertex, bfs_log);
    return 0;
}
