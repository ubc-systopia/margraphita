#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
#include "bulk_insert.h"

#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "reader.h"
#include "time_structs.h"
#include "times.h"

#define PRINT_ERROR(src, dst, ret, msg)                        \
    std::cerr << "Error inserting edge (" << src << "," << dst \
              << "). ret= " << ret << " : " << msg << std::endl;
#define PRINT_NODE_ERROR(node, ret, msg)                                      \
    std::cerr << "Error inserting node " << node << ". ret = " << ret << ": " \
              << msg << std::endl;
std::string pack_int_to_str(degree_t a, degree_t b)
{
    return std::string(to_string(a) + to_string(b));
}


const static int NUM_THREADS = 16;
const static int BUFFER_LENGTH = 20;

WT_CONNECTION *conn_std, *conn_adj, *conn_ekey;
double num_edges;
double num_nodes;
std::string dataset;
int read_optimized = 0;
int is_directed = 1;

std::string type_opt;
std::unordered_map<node_id_t, std::vector<node_id_t>> in_adjlist;
std::unordered_map<node_id_t, std::vector<node_id_t>> out_adjlist;
std::mutex lock;


void print_time_csvline(const std::string &db_name,
                        const std::string &db_path,

                        time_info *edget,
                        time_info *nodet,
                        bool is_readopt,
                        const std::string &logfile_name,
                        const std::string &type)
{
    std::ofstream log_file;
    log_file.open(logfile_name, std::fstream::app);

    struct stat st
    {
    };
    stat(logfile_name.c_str(), &st);
    if (st.st_size == 0)
    {
        log_file << "db_name, db_path, type, is_readopt, num_nodes, num_edges, "
                    "t_e_read, t_e_insert, t_n_read, t_n_insert\n";
    }

    log_file << db_name << "," << db_path << "/" << db_name << "," << type
             << "," << is_readopt << "," << nodet->num_inserted << ","
             << edget->num_inserted << "," << edget->read_time << ","
             << edget->insert_time << "," << nodet->read_time << ","
             << nodet->insert_time << "\n";
    log_file.close();
}

void insert_stats_to_session(WT_SESSION *session, size_t edgeNo, size_t nodeNo)
{
    WT_CURSOR *cursor;
    if (session->open_cursor(session, "table:metadata", NULL, NULL, &cursor) !=
        0)
    {
        std::cout << "Failed to open metadata table";
    }
    char buffer[BUFFER_LENGTH];
    sprintf(buffer, "%zu", nodeNo);
    cursor->set_key(cursor, node_count.c_str());
    cursor->set_value(cursor, buffer);
    cursor->insert(cursor);
    sprintf(buffer, "%zu", edgeNo);

    memset(buffer, 0, BUFFER_LENGTH);
    sprintf(buffer, "%zu", nodeNo);
    cursor->set_key(cursor, node_count.c_str());
    cursor->set_value(cursor, buffer);
    cursor->insert(cursor);

    sprintf(buffer, "%zu", edgeNo);
    cursor->set_key(cursor, edge_count.c_str());
    cursor->set_value(cursor, std::to_string(edgeNo).c_str());
    cursor->insert(cursor);

    cursor->close(cursor);
}

void insert_stats(size_t edgeNo, size_t nodeNo, const std::string &type)
{
    WT_SESSION *session = nullptr;
    if (type == "std")
        conn_std->open_session(conn_std, NULL, NULL, &session);
    else if (type == "adj")
        conn_adj->open_session(conn_adj, NULL, NULL, &session);
    else if (type == "ekey")
        conn_ekey->open_session(conn_ekey, NULL, NULL, &session);
    insert_stats_to_session(session, edgeNo, nodeNo);
    session->close(session, NULL);
}

time_info *insert_edge_thread(int _tid)
{
    int tid = _tid;
    std::string filename = dataset + "_edges";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);
    auto *info = new time_info();
    Times t;
    t.start();
    std::vector<edge> edjlist = reader::parse_edge_entries(filename);
    t.stop();
    // write nodes vector to a file
    std::ofstream out((filename + "_read").c_str());
    for (auto x : edjlist)
    {
        out << x.src_id << "\t" << x.dst_id << std::endl;
    }
    out.close();
    info->read_time = t.t_micros();
    info->num_inserted = edjlist.size();

    WT_CURSOR *cursor;
    WT_SESSION *session;
    t.start();

    if (type_opt == "std")
    {
        int ret;
        conn_std->open_session(conn_std, NULL, NULL, &session);
        session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

        for (edge e : edjlist)
        {
            cursor->set_key(cursor, e.src_id, e.dst_id);
            cursor->set_value(cursor, e.edge_weight);
            if ((ret = cursor->insert(cursor)) != 0)
            {
                PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
            }
        }

        cursor->close(cursor);
        session->close(session, NULL);
    }
    else if (type_opt == "adj")
    {
        int ret;
        conn_adj->open_session(conn_adj, NULL, NULL, &session);
        session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

        for (edge e : edjlist)
        {
            cursor->set_key(cursor, e.src_id, e.dst_id);
            cursor->set_value(cursor, 0);
            if ((ret = cursor->insert(cursor)) != 0)
            {
                PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))
            }
        }
        cursor->close(cursor);
        session->close(session, NULL);
    }
    else if (type_opt == "ekey")
    {
        int ret;
        conn_ekey->open_session(conn_ekey, NULL, NULL, &session);
        session->open_cursor(session, "table:edge", NULL, NULL, &cursor);

        for (edge e : edjlist)
        {
            cursor->set_key(cursor, e.src_id, e.dst_id);
            cursor->set_value(cursor, "");
            if ((ret = cursor->insert(cursor)) != 0)
            {

                PRINT_ERROR(e.src_id, e.dst_id, ret, wiredtiger_strerror(ret))

            }
        }
        cursor->close(cursor);
        session->close(session, NULL);
    }

    t.stop();
    info->insert_time = t.t_micros();
    return info;
}

time_info *insert_node(int _tid)
{
    int tid = _tid;  //*(int *)arg;
    std::string filename = dataset + "_nodes";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    auto *info = new time_info();
    Times t;
    t.start();
    std::vector<node> nodes = reader::parse_node_entries(filename);
    // write nodes vector to a file
    std::ofstream out((filename + "_read").c_str());
    for (auto x : nodes)
    {
        out << x.id << std::endl;
    }
    out.close();

    t.stop();
    info->read_time = t.t_micros();
    info->num_inserted = nodes.size();

    WT_CURSOR *std_cur, *ekey_cur, *adj_cur, *adj_incur, *adj_outcur;
    WT_SESSION *std_sess, *ekey_sess, *adj_sess;

    if (type_opt == "all" || type_opt == "std")
    {
        conn_std->open_session(conn_std, NULL, NULL, &std_sess);
        std_sess->open_cursor(std_sess, "table:node", NULL, NULL, &std_cur);
    }

    if (type_opt == "all" || type_opt == "ekey")
    {
        conn_ekey->open_session(conn_ekey, NULL, NULL, &ekey_sess);
        ekey_sess->open_cursor(ekey_sess, "table:edge", NULL, NULL, &ekey_cur);
    }

    if (type_opt == "all" || type_opt == "adj")
    {
        conn_adj->open_session(conn_adj, NULL, NULL, &adj_sess);
        adj_sess->open_cursor(adj_sess, "table:node", NULL, NULL, &adj_cur);
        adj_sess->open_cursor(
            adj_sess, "table:adjlistin", NULL, NULL, &adj_incur);
        adj_sess->open_cursor(
            adj_sess, "table:adjlistout", NULL, NULL, &adj_outcur);
    }
    t.start();
    for (node to_insert : nodes)
    {
        if (type_opt == "std")
        {
            int ret;
            std_cur->set_key(std_cur, to_insert.id);

            if (read_optimized)
            {
                std_cur->set_value(
                    std_cur, to_insert.in_degree, to_insert.out_degree);
            }
            else
            {
                std_cur->set_value(std_cur, "");
            }

            ret = std_cur->insert(std_cur) != 0;
            if (ret)
            {
                PRINT_NODE_ERROR(to_insert.id, ret, wiredtiger_strerror(ret))
            }
        }
        else if (type_opt == "ekey")
        {
            int ret;
            ekey_cur->set_key(ekey_cur, to_insert.id, OutOfBand_ID);
            if (read_optimized)
            {
                std::string packed =
                    pack_int_to_str(to_insert.in_degree, to_insert.out_degree);
                ekey_cur->set_value(ekey_cur, packed.c_str());
            }
            else
            {
                ekey_cur->set_value(ekey_cur, "");
            }

            ret = ekey_cur->insert(ekey_cur) != 0;
            if (ret)
            {
                PRINT_NODE_ERROR(to_insert.id, ret, wiredtiger_strerror(ret))
            }
        }
        else if (type_opt == "adj")
        {
            int ret;
            adj_cur->set_key(adj_cur, to_insert.id);
            if (read_optimized)
            {
                adj_cur->set_value(
                    adj_cur, to_insert.in_degree, to_insert.out_degree);
            }
            else
            {
                adj_cur->set_value(adj_cur, "");
            }

            if ((ret = adj_cur->insert(adj_cur)) != 0)
            {
                PRINT_NODE_ERROR(to_insert.id, ret, wiredtiger_strerror(ret))
            }

            // Now insert into in and out tables.
            adj_incur->set_key(adj_incur, to_insert.id);
            adj_outcur->set_key(adj_outcur, to_insert.id);
            try
            {
                WT_ITEM item;
                item.data = CommonUtil::pack_int_vector_wti(
                    adj_sess, in_adjlist.at(to_insert.id), &item.size);
                adj_incur->set_value(adj_incur, to_insert.in_degree, &item);
            }
            catch (const std::out_of_range &oor)
            {
                WT_ITEM item = {.data = {}, .size = 0};  // todo: check
                adj_incur->set_value(adj_incur, 0, &item);
            }

            try
            {
                WT_ITEM item;
                item.data = CommonUtil::pack_int_vector_wti(
                    adj_sess, out_adjlist.at(to_insert.id), &item.size);
                adj_outcur->set_value(adj_outcur, to_insert.out_degree, &item);
            }
            catch (const std::out_of_range &oor)
            {
                WT_ITEM item = {.data = {}, .size = 0};
                adj_outcur->set_value(adj_outcur, 0, &item);
            }

            adj_incur->insert(adj_incur);
            adj_outcur->insert(adj_outcur);  // TODO: do a check here as
                                             // well
        }
    }
    t.stop();
    info->insert_time = t.t_micros();
    if (type_opt == "all" || type_opt == "std")
    {
        std_cur->close(std_cur);
        std_sess->close(std_sess, NULL);
    }
    if (type_opt == "all" || type_opt == "ekey")
    {
        ekey_cur->close(ekey_cur);
        ekey_sess->close(ekey_sess, NULL);
    }
    if (type_opt == "all" || type_opt == "adj")
    {
        adj_cur->close(adj_cur);
        adj_sess->close(adj_sess, NULL);
    }

    return info;
}

int main(int argc, char *argv[])
{
    InsertOpts params(argc, argv);
    if (!params.parse_args())
    {
        params.print_help();
        return -1;
    }

    std::string middle;
    if (params.is_read_optimize())
    {
        middle += "r";
    }
    if (params.is_directed())
    {
        middle += "d";
    }
    dataset = params.get_dataset();
    read_optimized = params.is_read_optimize();
    std::string _db_name;
    std::string conn_config = "create,cache_size=10GB";
#ifdef STAT
    std::string stat_config =
        "statistics=(all),statistics_log=(wait=0,on_close=true";
    conn_config += "," + stat_config;
#endif
    // open std connection
    type_opt = params.get_type_str();
    if (type_opt == "all" || type_opt == "std")
    {
        _db_name = params.get_db_path() + "/std_" + middle + "_" +
                   params.get_db_name();
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_std) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    // open adjlist connection
    if (type_opt == "all" || type_opt == "adj")
    {
        _db_name = params.get_db_path() + "/adj_" + middle + "_" +
                   params.get_db_name();
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_adj) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    // open ekey connection
    if (type_opt == "all" || type_opt == "ekey")
    {
        _db_name = params.get_db_path() + "/ekey_" + middle + "_" +
                   params.get_db_name();
        if (wiredtiger_open(_db_name.c_str(),
                            NULL,
                            const_cast<char *>(conn_config.c_str()),
                            &conn_ekey) != 0)
        {
            std::cout << "Could not open the DB: " << _db_name;
            exit(1);
        }
    }

    Times t;
    t.start();
    auto *edge_times = new time_info;
    auto *node_times = new time_info;
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        time_info *this_thread_time = insert_edge_thread(i);
        edge_times->insert_time += this_thread_time->insert_time;
        edge_times->num_inserted += this_thread_time->num_inserted;
        edge_times->read_time += this_thread_time->read_time;
    }
    // write nodes vector to a file
    std::ofstream out((dataset + "inadj").c_str());
    for (const auto &x : in_adjlist)
    {
        out << x.first << " ";
        for (auto y : x.second)
        {
            out << y << " ";
        }
        out << std::endl;
    }
    out.close();
    out.open((dataset + "outadj").c_str());
    for (const auto &x : out_adjlist)
    {
        out << x.first << " ";
        for (auto y : x.second)
        {
            out << y << " ";
        }
        out << std::endl;
    }
    out.close();

    t.stop();
    std::cout << " Total time to insert edges was " << t.t_micros()
              << std::endl;

    // Now insert nodes;
    t.start();
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        time_info *this_thread_time = insert_node(i);
        node_times->insert_time += this_thread_time->insert_time;
        node_times->num_inserted += this_thread_time->num_inserted;
        node_times->read_time += this_thread_time->read_time;
    }
    insert_stats(edge_times->num_inserted, node_times->num_inserted, type_opt);

    t.stop();
    std::cout << " Total time to insert nodes was " << t.t_micros()
              << std::endl;

    // Adjust for threads
    node_times->insert_time = node_times->insert_time / NUM_THREADS;
    node_times->read_time = node_times->read_time / NUM_THREADS;
    edge_times->insert_time = edge_times->insert_time / NUM_THREADS;
    edge_times->read_time = edge_times->read_time / NUM_THREADS;

    std::cout << "total inserted edges = " << edge_times->num_inserted
              << " num_edges = " << num_edges << std::endl;
    std::cout << "time to insert edges " << edge_times->insert_time
              << std::endl;
    std::cout << "time to read edges " << edge_times->read_time << std::endl;

    std::cout << "total inserted nodes  = " << node_times->num_inserted
              << " num_nodes = " << num_nodes << std::endl;
    std::cout << "time to insert_nodes " << node_times->insert_time
              << std::endl;
    std::cout << "time to read nodes " << node_times->read_time << std::endl;

    print_time_csvline(params.get_db_name(),
                       params.get_db_path(),
                       edge_times,
                       node_times,
                       params.is_read_optimize(),
                       params.get_logdir(),
                       type_opt);

    if (type_opt == "all" || type_opt == "std")
    {
        conn_std->close(conn_std, NULL);
    }
    if (type_opt == "all" || type_opt == "adj")
    {
        conn_adj->close(conn_adj, NULL);
    }
    if (type_opt == "all" || type_opt == "ekey")
    {
        conn_ekey->close(conn_ekey, NULL);
    }

    return (EXIT_SUCCESS);
}
#pragma clang diagnostic pop