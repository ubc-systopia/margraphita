#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "standard_graph.h"

class GraphFactory
{
   public:
    GraphBase *CreateGraph(graph_opts opts)
    {
        GraphBase *ptr = nullptr;
        if (opts.type == GraphType::Std)
        {
            ptr = new StandardGraph(opts);
        }
        else if (opts.type == GraphType::Adj)
        {
            ptr = new AdjList(opts);
        }
        else if (opts.type == GraphType::EKey)
        {
            ptr = new EdgeKey(opts);
        }
        if (!ptr)
        {
            return ptr;
        }
        else
        {
            throw "Failed to create graph object";
        }
    }
};
