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

    /**
     * @brief Sets the path that the graph resides in, establishes *one* WT
     * connection to that graph
     *
     * @param opts options specified by the user for the graph
     * @param db_name relative path to the WT db
     */

    void SetDBPath(graph_opts &opts, string db_name)
    {
        CommonUtil::open_connection(const_cast<char *>(db_name.c_str()),
                                    opts.stat_log,
                                    opts.conn_config,
                                    &conn);
    }

    /**
     * @brief Creates a handle to the graph for parallel execution
     *
     * @param opts options specified by the user for the graph
     *
     * @return a GraphBase pointer
     */

    GraphBase *CreateGraphHandle(
        graph_opts &opts)  // TODO: merge with CreateGraph
    {
        if (!conn)
        {
            throw GraphException("DB Path not set");
        }
        WT_SESSION *sess;
        CommonUtil::open_session(conn, &sess);
        wt_conn t{.connection = conn, .session = sess};

        GraphBase *ptr = nullptr;
        if (opts.type == GraphType::Std)
        {
            ptr = new StandardGraph(opts, t);
            return ptr;
        }
        else if (opts.type == GraphType::Adj)
        {
            ptr = new AdjList(opts, t);
            return ptr;
        }
        else if (opts.type == GraphType::EKey)
        {
            ptr = new EdgeKey(opts, t);
            return ptr;
        }

        throw GraphException("Failed to create graph object");
    }

    /**
     * @brief Closes connection to the DB, commits all changes
     */
    void CloseGraph() { CommonUtil::close_connection(conn); }

   private:
    WT_CONNECTION *conn = nullptr;
};
