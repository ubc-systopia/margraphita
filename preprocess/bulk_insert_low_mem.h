//
// Created by puneet on 05/02/24.
//

#ifndef GRAPHAPI_BULK_INSERT_LOW_MEM_H
#define GRAPHAPI_BULK_INSERT_LOW_MEM_H
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common_util.h"
#include "reader.h"
#include "time_structs.h"
#include "times.h"

#define NUM_THREADS 16
#define PRINT_ADJ_ERROR(node_id, ret, msg)                                    \
    {                                                                         \
        std::cerr << "Error inserting adjlist for node " << (node_id) << ": " \
                  << (msg) << " (" << (ret) << ")" << std::endl;              \
    }

#define PRINT_EDGE_ERROR(src, dst, ret, msg)                        \
    std::cerr << "Error inserting edge (" << (src) << ", " << (dst) \
              << "): " << (msg) << " (" << (ret) << ")" << std::endl;
#define CHECK(x)                                                         \
    do                                                                   \
    {                                                                    \
        int retval = (x);                                                \
        if (retval != 0)                                                 \
        {                                                                \
            fprintf(stderr,                                              \
                    "Runtime error: %s returned %d at %s:%d",            \
                    #x,                                                  \
                    retval,                                              \
                    __FILE__,                                            \
                    __LINE__);                                           \
            fprintf(stderr, "Error: %s\n", wiredtiger_strerror(retval)); \
            exit(1); /* or throw or whatever */                          \
        }                                                                \
    } while (0)

int space = 0;
graph_opts opts;
int num_per_chunk;

WT_CONNECTION *conn_std, *conn_adj, *conn_ekey;

typedef struct fgraph_conn_object
{
    GraphType type;
    WT_CONNECTION *conn = nullptr;
    WT_SESSION *session = nullptr;
    WT_CURSOR *e_cur = nullptr;
    WT_CURSOR *n_cur = nullptr;
    WT_CURSOR *cur = nullptr;  // adjlist cursor
    std::string adjlistable_name;

    fgraph_conn_object(WT_CONNECTION *_conn,
                       std::string _wt_adjtable_str,
                       GraphType _type,
                       bool is_edge = true)
        : type(_type),
          conn(_conn),
          adjlistable_name(std::move(_wt_adjtable_str))
    {
        int ret = conn->open_session(conn, nullptr, nullptr, &session);
        if (ret != 0)
        {
            std::cerr << "Error opening session: " << wiredtiger_strerror(ret)
                      << std::endl;
            exit(1);
        }
        if (is_edge)
        {
            ret = session->open_cursor(
                session, "table:edge", nullptr, nullptr, &e_cur);
            if (ret != 0)
            {
                std::cerr << "Error opening cursor: "
                          << wiredtiger_strerror(ret) << std::endl;
                exit(1);
            }
            if (type == GraphType::Adj)
            {
                ret = session->open_cursor(
                    session, adjlistable_name.c_str(), nullptr, nullptr, &cur);
                if (ret != 0)
                {
                    std::cerr
                        << "Error opening cursor: " << wiredtiger_strerror(ret)
                        << std::endl;
                    exit(1);
                }
            }
        }
        else  // We are inserting a node.
        {
            if (type == GraphType::EKey)
            {
                ret = session->open_cursor(
                    session, "table:edge", nullptr, nullptr, &e_cur);
                if (ret != 0)
                {
                    std::cerr
                        << "Error opening cursor: " << wiredtiger_strerror(ret)
                        << std::endl;
                    exit(1);
                }
            }
            else
            {
                ret = session->open_cursor(
                    session, "table:node", nullptr, nullptr, &n_cur);
                if (ret != 0)
                {
                    std::cerr
                        << "Error opening cursor: " << wiredtiger_strerror(ret)
                        << std::endl;
                    exit(1);
                }
            }
        }
    }

    ~fgraph_conn_object()
    {
        if (cur != nullptr)
        {
            cur->close(cur);
        }
        if (e_cur != nullptr)
        {
            e_cur->close(e_cur);
        }
        if (n_cur != nullptr)
        {
            n_cur->close(n_cur);
        }
        if (session != nullptr)
        {
            session->close(session, nullptr);
        }
    }
} worker_sessions;

typedef struct _degrees
{
    degree_t in_degree = 0;
    degree_t out_degree = 0;
} degrees;

std::unordered_map<node_id_t, degrees> node_degrees;
std::mutex lock_var;

int check_cursors(worker_sessions &info)
{
    std::cerr << "Checking cursors\n";
    assert(info.cur != nullptr);
    assert(info.session != nullptr);
    assert(info.conn != nullptr);
    return 0;
}

int add_to_adjlist(WT_CURSOR *adjcur, adjlist &adj)
{
    CommonUtil::set_key(adjcur, adj.node_id);
    WT_ITEM item;
    item.data = adj.edgelist.data();
    item.size = adj.edgelist.size() * sizeof(node_id_t);
    space += adj.edgelist.size() * sizeof(node_id_t);
    adjcur->set_value(adjcur, adj.edgelist.size(), &item);
    int ret = adjcur->insert(adjcur);
    if (ret != 0)
    {
        PRINT_ADJ_ERROR(adj.node_id, ret, wiredtiger_strerror(ret))
        return ret;
    }
    return ret;
}

int add_to_edge_table(WT_CURSOR *cur,
                      const node_id_t node_id,
                      const std::vector<node_id_t> &edgelist)
{
    // std::cout << edgelist.size() << std::endl;
    for (node_id_t dst : edgelist)
    {
        CommonUtil::set_key(cur, node_id, dst);
        cur->set_value(cur, 0);
        int ret = cur->insert(cur);
        if (ret != 0)
        {
            PRINT_EDGE_ERROR(node_id, dst, ret, wiredtiger_strerror(ret))
            return ret;
        }
    }
    return 0;
}

int add_to_edgekey(WT_CURSOR *ekey_cur,
                   const node_id_t node_id,
                   const std::vector<node_id_t> &edgelist)
{
    node_id_t src = node_id;
    for (node_id_t dst : edgelist)
    {
        CommonUtil::set_key(ekey_cur, src, dst);
        ekey_cur->set_value(ekey_cur, 0, OutOfBand_Val);
        int ret = ekey_cur->insert(ekey_cur);
        if (ret != 0)
        {
            PRINT_EDGE_ERROR(node_id, dst, ret, wiredtiger_strerror(ret))
            return ret;
        }
    }
    return 0;
}

int add_to_node_table(WT_CURSOR *cur, const node &node)
{
    CommonUtil::set_key(cur, node.id);
    cur->set_value(cur, node.in_degree, node.out_degree);
    int ret = cur->insert(cur);
    if (ret != 0)
    {
        std::cerr << "Error inserting node " << node.id << ": "
                  << wiredtiger_strerror(ret) << std::endl;
        return ret;
    }
    return ret;
}
int add_node_to_ekey(WT_CURSOR *ekey_cur, const node &node)
{
    CommonUtil::set_key(ekey_cur, node.id, OutOfBand_ID);
    ekey_cur->set_value(ekey_cur, node.in_degree, node.out_degree);
    int ret = ekey_cur->insert(ekey_cur);
    if (ret != 0)
    {
        std::cerr << "Error inserting node " << node.id << ": "
                  << wiredtiger_strerror(ret) << std::endl;
        return ret;
    }
    return ret;
}

void make_connections(graph_opts &opts, const std::string &conn_config)
{
    std::string middle;
    if (opts.read_optimize)
    {
        middle += "r";
    }
    if (opts.is_directed)
    {
        middle += "d";
    }
    // make sure the dataset is a directory, if not, extract the directory name
    // and use it
    if (opts.dataset.back() != '/')
    {
        size_t found = opts.dataset.find_last_of("/\\");
        if (found != std::string::npos)
        {
            opts.dataset = opts.dataset.substr(0, found);
        }
    }
    std::cout << "Dataset: " << opts.dataset << std::endl;

    std::string _db_name = opts.db_dir + "/adj_" + middle + "_" + opts.db_name;
    if (wiredtiger_open(_db_name.c_str(),
                        nullptr,
                        const_cast<char *>(conn_config.c_str()),
                        &conn_adj) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }
    // open ekey connection
    _db_name = opts.db_dir + "/ekey_" + middle + "_" + opts.db_name;
    if (wiredtiger_open(_db_name.c_str(),
                        nullptr,
                        const_cast<char *>(conn_config.c_str()),
                        &conn_ekey) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }

    _db_name = opts.db_dir + "/std_" + middle + "_" + opts.db_name;

    if (wiredtiger_open(_db_name.c_str(),
                        nullptr,
                        const_cast<char *>(conn_config.c_str()),
                        &conn_std) != 0)
    {
        std::cout << "Could not open the DB: " << _db_name;
        exit(1);
    }

    assert(conn_adj != nullptr);
    assert(conn_std != nullptr);
    assert(conn_ekey != nullptr);
}

void dump_config(const graph_opts &opts, const std::string &conn_config)
{
    // dump the config
    std::cout << "Dumping parameters: " << std::endl;
    std::cout << "Dataset: " << opts.dataset << std::endl;
    std::cout << "Read optimized: " << opts.read_optimize << std::endl;
    std::cout << "Directed: " << opts.is_directed << std::endl;
    std::cout << "Num edges: " << opts.num_edges << std::endl;
    std::cout << "Num nodes: " << opts.num_nodes << std::endl;
    std::cout << "Num threads: " << NUM_THREADS << std::endl;
    std::cout << "DB dir: " << opts.db_dir << std::endl;
    std::cout << "DB name: " << opts.db_name << std::endl;
    std::cout << "DB config: " << conn_config << std::endl;
}

#endif  // GRAPHAPI_BULK_INSERT_LOW_MEM_H
