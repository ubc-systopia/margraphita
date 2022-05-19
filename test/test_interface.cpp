#include "test_interface.h"

#include <cassert>

#include "common.h"
#include "graph_exception.h"
#include "sample_graph.h"

// enum GraphRepresentation
// {
//     adj = 1,
//     std = 2,
//     edg = 3
// };

#define delim "--------------"
// void printInfo(GraphRepresentation representation)
// {
//     fprintf(stderr, "%s\nNow running: %s on:\n", delim, __FUNCTION__);
//     switch (representation)
//     {
//         case adj:
//             fprintf(stderr, "%s\n", "AdjacencyList representation");
//             break;

//         case std:
//             fprintf(stderr, "%s\n", "StandardGraph representation");
//             break;

//         case edg:
//             fprintf(stderr, "%s\n", "EdgeList representation");
//             break;
//     }
// }

int main()
{
    graph_opts opts;
    opts.create_new = true;
    opts.optimize_create = false;
    opts.is_directed = true;
    opts.read_optimize = true;
    opts.is_weighted = true;
    opts.db_dir = "./db";
    opts.db_name = "test_adj";
    opts.conn_config = "cache_size=10GB";
    opts.stat_log = "/home/puneet/scratch/margraphita/profile/test";

    AdjList adjGraph = AdjList(opts);
    StandardGraph stdGraph = StandardGraph(opts);
    EdgeKey edgeGraph = EdgeKey(opts);

    GraphBase* graphs[] = {&adjGraph, &stdGraph, &edgeGraph};
}