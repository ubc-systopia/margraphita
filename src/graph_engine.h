#ifndef GRAPH_ENGINE
#define GRAPH_ENGINE

#include "lock.h"
#include "graph.h"

using namespace std;

class GraphEngine
{
    public:
    GraphEngine();
    GraphBase* create_graph_handle();
    

    private:
    LockSet* locks;
};

#endif