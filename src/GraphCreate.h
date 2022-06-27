#include "adj_list.h"
#include "common.h"
#include "edgekey.h"
#include "graph.h"
#include "graph_exception.h"
#include "standard_graph.h"

class GraphFactory
{
   public:
    GraphBase *CreateGraph(graph_opts &opts)
    {
        GraphBase *ptr = nullptr;
        if (opts.type == GraphType::Std)
        {
            ptr = new StandardGraph(opts);
            return ptr;
        }
        else if (opts.type == GraphType::Adj)
        {
            ptr = new AdjList(opts);
            return ptr;
        }
        else if (opts.type == GraphType::EKey)
        {
            ptr = new EdgeKey(opts);
            return ptr;
        }

        throw GraphException("Failed to create graph object");
    }
};
