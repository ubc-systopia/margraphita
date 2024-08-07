#include <omp.h>

#include <iostream>

#include "benchmark_definitions.h"
#include "bitmap.h"
#include "command_line.h"
#include "common_util.h"
#include "graph_engine.h"
#include "platform_atomics.h"
#include "pvector.h"
#include "sliding_queue.h"
#include "standard_graph.h"
#include "times.h"

/**
/*
GAP Benchmark Suite
Kernel: Breadth-First Search (BFS)
Author: Scott Beamer

Will return parent array for a BFS traversal from a source vertex

This BFS implementation makes use of the Direction-Optimizing approach [1].
It uses the alpha and beta parameters to determine whether to switch search
directions. For representing the frontier, it uses a SlidingQueue for the
top-down approach and a Bitmap for the bottom-up approach. To reduce
false-sharing for the top-down approach, thread-local QueueBuffer's are used.

To save time computing the number of edges exiting the frontier, this
implementation precomputes the degrees in bulk at the beginning by storing
them in parent array as negative numbers. Thus the encoding of parent is:
  parent[x] < 0 implies x is unvisited and parent[x] = -out_degree(x)
  parent[x] >= 0 implies x been visited

[1] Scott Beamer, Krste Asanović, and David Patterson. "Direction-Optimizing
    Breadth-First Search." International Conference on High Performance
    Computing, Networking, Storage and Analysis (SC), Salt Lake City, Utah,
    November 2012.
*/

// ^^^ This is why the parent array is templated with int64_t instead of
// node_id_t (uint32_t) one would expect. this is to match the original GAPBS
// implementation.
typedef int64_t NodeID;

int64_t BUStep(GraphEngine *graph_engine,
               pvector<NodeID> &parent,
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
        adjlist found;

        in_cursor->set_key_range(graph_engine->get_key_range(i));

        in_cursor->next(&found);

        while (found.node_id != OutOfBand_ID_MAX)
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
        graph->close(false);
    }

    return awake_count;
}

int64_t TDStep(GraphEngine *graph_engine,
               pvector<NodeID> &parent,
               SlidingQueue<NodeID> &queue)
{
    int64_t scout_count = 0;

#pragma omp parallel
    {
        QueueBuffer<NodeID> lqueue(queue);
#pragma omp for reduction(+ : scout_count) nowait
        for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
        {
            NodeID u = *q_iter;
            GraphBase *graph = graph_engine->create_graph_handle();
            for (node_id_t v : graph->get_out_nodes_id(u))
            {
                NodeID curr_val = parent[v];
                if (curr_val < 0)
                {
                    if (compare_and_swap(parent[v], curr_val, u))
                    {
                        lqueue.push_back(v);
                        scout_count += -curr_val;
                    }
                }
            }
            graph->close(false);
        }
        lqueue.flush();
    }
    return scout_count;
}

void QueueToBitmap(const SlidingQueue<NodeID> &queue, Bitmap &bm)
{
#pragma omp parallel for
    for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++)
    {
        NodeID u = *q_iter;
        bm.set_bit_atomic(u);
    }
}

void BitmapToQueue(GraphEngine *graph_engine,
                   const Bitmap &bm,
                   SlidingQueue<NodeID> &queue,
                   int thread_num)
{
#pragma omp parallel num_threads(thread_num)
    {
        QueueBuffer<NodeID> lqueue(queue);
#pragma omp for nowait
        for (int i = 0; i < thread_num; i++)
        {
            GraphBase *graph = graph_engine->create_graph_handle();
            NodeCursor *node_cursor = graph->get_node_iter();
            node_cursor->set_key_range(graph_engine->get_key_range(i));

            node found = {0};
            node_cursor->next(&found);
            while (found.id != OutOfBand_ID_MAX)
            {
                if (bm.get_bit(found.id)) lqueue.push_back(found.id);
                node_cursor->next(&found);
            }
            graph->close(false);
        }
        lqueue.flush();
    }
    queue.slide_window();
}

pvector<NodeID> InitParent(GraphEngine *graph_engine,
                           node_id_t max_node_id,
                           int thread_num)
{
    pvector<NodeID> parent(max_node_id);

#pragma omp parallel for num_threads(thread_num)
    for (int i = 0; i < thread_num; i++)
    {
        GraphBase *graph = graph_engine->create_graph_handle();
        NodeCursor *node_cursor = graph->get_node_iter();
        node found = {0};
        node_cursor->set_key_range(graph_engine->get_key_range(i));

        node_cursor->next(&found);
        while (found.id != OutOfBand_ID_MAX)
        {
            parent[found.id] = graph->get_out_degree(found.id) != 0
                                   ? -graph->get_out_degree(found.id)
                                   : -1;
            node_cursor->next(&found);
        }
        graph->close(false);
    }
    return parent;
}

/**
 * In this function, only the parent array is being accessed by node_id. We need
 * to assign the array to be as large as the max_node_id
 *
 * SlidingQueue and the Bitmaps are always accessed by their methods (flush,
 * push, begin, end, etc.) and therefore can be created with size = num_nodes
 */
pvector<NodeID> DOBFS(GraphEngine *graph_engine,
                      node_id_t source,
                      node_id_t num_nodes,
                      node_id_t max_node_id,
                      int thread_num,
                      int alpha = 15,
                      int beta = 18)
{
    GraphBase *graph_stat = graph_engine->create_graph_handle();
    Times t;
    t.start();
    pvector<NodeID> parent = InitParent(graph_engine, max_node_id, thread_num);
    t.stop();

    printf("%5s%23.5Lf\n", "i", t.t_secs());

    parent[source] = source;
    SlidingQueue<NodeID> queue(num_nodes);
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
            uint64_t awake_count, old_awake_count;
            t.start();
            QueueToBitmap(queue, front);
            t.stop();
            printf("%5s%23.5Lf\n", "e", t.t_secs());
            awake_count = queue.size();
            queue.slide_window();
            do
            {
                t.start();
                old_awake_count = awake_count;
                awake_count =
                    BUStep(graph_engine, parent, front, curr, thread_num);
                front.swap(curr);
                t.stop();
                printf("%5s%23.5Lf\n", "bu", t.t_secs());

            } while ((awake_count >= old_awake_count) ||
                     (awake_count > num_nodes / beta));
            t.start();
            BitmapToQueue(graph_engine, front, queue, thread_num);
            t.stop();
            printf("%5s%23.5Lf\n", "c", t.t_secs());
            scout_count = 1;
        }
        else
        {
            t.start();
            edges_to_check -= scout_count;
            scout_count = TDStep(graph_engine, parent, queue);
            queue.slide_window();
            t.stop();
            printf("%5s%23.5Lf\n", "td", t.t_secs());
        }
    }

#pragma omp parallel for
    for (node_id_t n = 0; n < num_nodes; n++)
        if (parent[n] < -1) parent[n] = -1;

    graph_stat->close(false);
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
    cmdline_opts opts = bfs_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;

    const int THREAD_NUM = omp_get_max_threads();
    std::cout << "THREAD_NUM: " << THREAD_NUM << std::endl;
    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_micros() << std::endl;

    t.start();
    GraphBase *g = graphEngine.create_graph_handle();
    node_id_t num_nodes = g->get_num_nodes();
    node_id_t max_node_id = g->get_max_node_id();
    node source = g->get_random_node();
    g->close(false);
    DOBFS(&graphEngine, source.id, num_nodes, max_node_id, THREAD_NUM, 15, 18);
    t.stop();
    std::cout << "BFS completed in " << t.t_micros() << std::endl;
    graphEngine.close_graph();
}
