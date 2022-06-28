#include "graph_engine.h"

using namespace std;

GraphEngine::GraphEngine(graph_engine_opts engine_opts)
{
    num_threads = engine_opts.num_threads;
    opts = engine_opts.num_threads;
    locks = new LockSet();
}

GraphEngine::~GraphEngine()
{
    delete locks;
}

GraphBase* GraphEngine::create_graph_handle()
{
    GraphBase *ptr = nullptr;
        if (opts.type == GraphType::Std)
        {
            ptr = new StandardGraph(opts);
            ptr->set_locks(locks);
            return ptr;
        }
        else if (opts.type == GraphType::Adj)
        {
            ptr = new AdjList(opts);
            ptr->set_locks(locks);
            return ptr;
        }
        else if (opts.type == GraphType::EKey)
        {
            ptr = new EdgeKey(opts);
            ptr->set_locks(locks);
            return ptr;
        }

        throw GraphException("Failed to create graph object");
}