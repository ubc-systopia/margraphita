#include "graph_engine.h"

#include <filesystem>

using namespace std;

GraphEngine::GraphEngine(graph_engine_opts engine_opts)
{
    num_threads = engine_opts.num_threads;
    opts = engine_opts.opts;
    check_opts_valid();
    locks = new LockSet();
    if (opts.create_new)
    {
        create_new_graph();
    }
    else
    {
        open_connection();
    }
    GraphBase *graph_stats = this->create_graph_handle();
    uint64_t edgeno = graph_stats->get_num_edges();
    insert_head = edgeno + 1;
    graph_stats->close();
}

GraphEngine::~GraphEngine()
{
    delete locks;
    if (conn != NULL)
    {
        close_connection();
    }
}

GraphBase *GraphEngine::create_graph_handle()
{
    GraphBase *ptr = nullptr;
    if (opts.type == GraphType::Std)
    {
        ptr = new StandardGraph(opts, conn);
    }
    else if (opts.type == GraphType::Adj)
    {
        ptr = new AdjList(opts, conn);
    }
    else if (opts.type == GraphType::EKey)
    {
        ptr = new EdgeKey(opts, conn);
    }
    else if (opts.type == GraphType::EList)
    {
        ptr = new UnOrderedEdgeList(opts, conn, this);
    }
    else
    {
        throw GraphException("Failed to create graph object");
    }
    ptr->set_locks(locks);
    return ptr;
}

void GraphEngine::create_indices()
{
    WT_SESSION *sess;
    CommonUtil::open_session(conn, &sess);
    // Should enforce other handles are closed here (TODO?)
    if (opts.type == GraphType::Std)
    {
        StandardGraph::create_indices(sess);
    }
    else if (opts.type == GraphType::EKey)
    {
        EdgeKey::create_indices(sess);
    }
    else if (opts.type == GraphType::EList)
    {
        UnOrderedEdgeList::create_indices(sess);
    }
    else
    {
        throw GraphException("Failed to create graph object");
    }
}

void GraphEngine::calculate_thread_offsets()
{
    // Create snapshot here first?
    GraphBase *graph_stats = this->create_graph_handle();
    calculate_thread_offsets_node(
        num_threads, graph_stats->get_num_nodes(), node_ranges);
    calculate_thread_offsets_edge(
        num_threads, graph_stats->get_num_edges(), edge_ranges);
    graph_stats->close();
}

std::pair<uint64_t, uint64_t> GraphEngine::request_insert_range()
{
    omp_set_lock(locks->get_insert_range_lock());
    uint64_t start = insert_head;
    uint64_t end = insert_head + 10000;
    insert_head += 10000;
    omp_unset_lock(locks->get_insert_range_lock());
    return pair<uint64_t, uint64_t>(start, end);
}

void GraphEngine::check_opts_valid()
{
    if (num_threads < 1)
    {
        throw GraphException("Number of threads must be at least 1");
    }

    try
    {
        CommonUtil::check_graph_params(opts);
    }
    catch (GraphException &G)
    {
        std::cout << G.what() << std::endl;
    }
    if (!CommonUtil::check_dir_exists(opts.stat_log))
    {
        try
        {
            std::cout << "Creating stat log directory " << opts.stat_log
                      << std::endl;
            std::filesystem::create_directories(opts.stat_log);
        }
        catch (GraphException &G)
        {
            std::cout << G.what() << std::endl;
        }
    }
    std::cout << "db_dir = " << opts.db_dir << std::endl;
    std::cout << "db_name = " << opts.db_name << std::endl;
}

void GraphEngine::create_new_graph()
{
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    std::cout << "Creating new graph at " << dirname << std::endl;
    CommonUtil::create_dir(dirname);
    if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                    opts.stat_log,
                                    opts.conn_config,
                                    &conn) < 0)
    {
        throw GraphException("Cannot open connection to new DB");
    };

    if (opts.type == GraphType::Std)
    {
        StandardGraph::create_wt_tables(opts, conn);
    }
    else if (opts.type == GraphType::Adj)
    {
        AdjList::create_wt_tables(opts, conn);
    }
    else if (opts.type == GraphType::EKey)
    {
        EdgeKey::create_wt_tables(opts, conn);
    }
    else if (opts.type == GraphType::EList)
    {
        UnOrderedEdgeList::create_wt_tables(opts, conn);
    }
    else
    {
        throw GraphException("Failed to create graph object");
    }

    GraphBase::create_metadata_table(opts, conn);
}

void GraphEngine::open_connection()
{
    std::string dirname = opts.db_dir + "/" + opts.db_name;
    if (CommonUtil::open_connection(const_cast<char *>(dirname.c_str()),
                                    opts.stat_log,
                                    opts.conn_config,
                                    &conn) < 0)
    {
        throw GraphException("Cannot open connection");
    };
}

void GraphEngine::close_connection()
{
    CommonUtil::close_connection(conn);
    conn = NULL;
}

WT_CONNECTION *GraphEngine::get_connection() { return conn; }

void GraphEngine::close_graph() { close_connection(); }

std::pair<node, node> GraphEngine::get_key_range(int thread_id)
{
    return node_ranges[thread_id];
}

std::pair<edge, edge> GraphEngine::get_edge_range(int thread_id)
{
    return edge_ranges[thread_id];
}

void GraphEngine::calculate_thread_offsets_edge(
    int thread_max,
    node_id_t num_edges,
    std::vector<std::pair<edge, edge>> &edge_ranges)
{
    edge_ranges.clear();
    node_id_t per_partition = num_edges / thread_max;

    GraphBase *graph = this->create_graph_handle();
    EdgeCursor *edge_cursor = graph->get_edge_iter();

    edge found;
    std::pair<edge, edge> temp;

    for (int i = 0; i < thread_max; i++)
    {
        edge_cursor->next(&found);
        if (found.src_id != -1)
        {
            temp.first = {found.id, found.src_id, found.dst_id};
        }

        node_id_t curr_partition = 1;
        if (i == thread_max - 1)
        {
            temp.second = {UINT64_MAX, -1, -1};
        }
        else
        {
            while (found.src_id != -1 && curr_partition < per_partition)
            {
                curr_partition++;
                edge_cursor->next(&found);
            }
            temp.second = {found.id, found.src_id, found.dst_id};
        }
        edge_ranges.push_back(temp);
    }

    graph->close();
}

void GraphEngine::calculate_thread_offsets_node(
    int thread_max,
    node_id_t num_nodes,
    std::vector<std::pair<node, node>> &node_ranges)
{
    node_id_t node_offset = 0;
    node_ranges.clear();

    for (int i = 0; i < thread_max; i++)
    {
        std::pair<node, node> temp;
        if (i == thread_max - 1)
        {
            temp = {node(node_offset), node(num_nodes)};
        }
        else
        {
            temp = {node(node_offset),
                    node((node_offset + (num_nodes / thread_max) - 1))};
        }

        node_ranges.push_back(temp);
        node_offset += num_nodes / thread_max;
    }
}

// void GraphEngine::calculate_thread_offsets_edge_partition_better(
//     int thread_max,
//     node_id_t num_nodes,
//     node_id_t num_edges,
//     std::vector<key_range> &node_ranges,
//     std::vector<edge_range> &edge_offsets,
//     GraphType type)
// {
//     node_id_t node_offset = 0;
//     node_ranges.clear();
//     edge_offsets.clear();

//     GraphBase *graph = this->create_graph_handle();
//     InCursor *in_cursor = graph->get_innbd_iter();
//     OutCursor *out_cursor = graph->get_outnbd_iter();
//     adjlist innbd = {0};
//     adjlist outnbd = {0};

//     node_id_t edge_count = graph->get_num_edges();
//     node_id_t edge_target = edge_count / thread_max;
//     in_cursor->next(&innbd);
//     out_cursor->next(&outnbd);

//     for (int i = 0; i < thread_max; i++)
//     {
//         key_range temp;
//         node_id_t edge_count_sum = 0;
//         node_id_t prev_difference = edge_target;

//         if (i == thread_max - 1)
//         {
//             temp = {node_offset, num_nodes};
//         }
//         else
//         {
//             while (innbd.node_id != -1 && outnbd.node_id != -1)
//             {
//                 node_id_t cur_difference = abs(edge_count_sum + innbd.degree
//                 +
//                                                outnbd.degree - edge_target);
//                 if (cur_difference > prev_difference)
//                 {
//                     temp = {node_offset, innbd.node_id};
//                     break;
//                 }
//                 in_cursor->next(&innbd);
//                 out_cursor->next(&outnbd);
//                 prev_difference = cur_difference;
//                 edge_count_sum += innbd.degree + outnbd.degree;
//             }
//         }

//         node_ranges.push_back(temp);
//         node_offset = innbd.node_id;
//         edge_count -= edge_count_sum;
//         edge_target = edge_count / thread_max;
//     }

//     for (auto x : node_ranges)
//     {
//         key_pair first = {x.start, 0}, last = {x.end + 1, -1};
//         edge_range er(first, last);
//         edge_offsets.push_back(er);
//     }
// }