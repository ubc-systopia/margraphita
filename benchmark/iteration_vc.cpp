#include <iostream>
#include <unordered_set>

#include "benchmark_definitions.h"
#include "command_line.h"
#include "common_util.h"
#include "graph_engine.h"
#include "standard_graph.h"
#include "times.h"
#ifdef DEBUG
#include <ittnotify.h>
__itt_domain* domain = __itt_domain_create("MyTraces.MyDomain");
__itt_string_handle* InScan = __itt_string_handle_create("InNbd Scan Task");
__itt_string_handle* OutScan = __itt_string_handle_create("OutNbd Scan Task");

#endif
#define ITERS 50

int main(int argc, char* argv[])
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    std::cout << "Testing Neighborhood Iteration" << std::endl;
    CmdLineApp iter_cli(argc, argv);
    if (!iter_cli.parse_args())
    {
        return -1;
    }

    cmdline_opts opts = iter_cli.get_parsed_opts();
    opts.stat_log += "/" + opts.db_name;

    const int THREAD_NUM = 1;
    Times t;
    t.start();
    GraphEngine graphEngine(THREAD_NUM, opts);
    graphEngine.calculate_thread_offsets();
    t.stop();
    std::cout << "Graph loaded in " << t.t_secs() << std::endl;
    std::unordered_set<node_id_t> nodes;
    std::unordered_set<node_id_t> in_nodes;
    for (int i = 0; i < opts.num_trials; i++)
    {
        iter_info info(0);
        for (int id = 0; id < THREAD_NUM; id++)
        {
            GraphBase* graph = graphEngine.create_graph_handle();
            adjlist found;
            t.start();

#ifdef DEBUG
            __itt_task_begin(domain, __itt_null, __itt_null, InScan);
#endif
            OutCursor* out_cursor = graph->get_outnbd_iter();
            out_cursor->set_key_range(graphEngine.get_key_range(id));
            out_cursor->next(&found);
            while (found.node_id != -1)
            {
                nodes.insert(found.node_id);
                for (node_id_t v : found.edgelist)
                {
                    nodes.insert(v);
                }
                found.clear();
                out_cursor->next(&found);
            }
#ifdef DEBUG
            __itt_task_end(domain);
#endif
            t.stop();
            info.time_taken = t.t_secs();
            std::cout << "Iteration over out edges completed in : "
                      << info.time_taken << std::endl;
            out_cursor->close();

            found.clear();
            t.start();
#ifdef DEBUG
            __itt_task_begin(domain, __itt_null, __itt_null, InScan);
#endif
            InCursor* in_cursor = graph->get_innbd_iter();
            in_cursor->set_key_range(graphEngine.get_key_range(id));
            in_cursor->next(&found);
            while (found.node_id != -1)
            {
                in_nodes.insert(found.node_id);
                for (node_id_t v : found.edgelist)
                {
                    in_nodes.insert(v);
                }
                found.clear();
                in_cursor->next(&found);
            }
#ifdef DEBUG
            __itt_task_end(domain);
#endif
            t.stop();
            info.time_taken = t.t_secs();
            std::cout << "Iteration over in edges completed in : "
                      << info.time_taken << std::endl;
            std::cout << "number of in-nodes: " << in_nodes.size() << std::endl;
            graph->close();
        }
    }

    return 0;
}