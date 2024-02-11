#include "bulk_insert_low_mem.h"

#include <omp.h>
void insert_edge_thread(int _tid)
{
    int tid = _tid;
    std::string filename = opts.dataset + "/out_";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    worker_sessions adj_obj(conn_adj, "table:adjlistout", GraphType::Adj);
    worker_sessions std_obj(conn_std, "table:edge", GraphType::Std);
    worker_sessions ekey_obj(conn_ekey, "table:edge", GraphType::EKey);

    reader::AdjReader adj_reader(filename);
    adjlist adj_list;
    int line = 0;
    while (adj_reader.get_next_adjlist(adj_list) == 0)
    {
        int ret;
        line++;
        // insert into STD: edge table
        add_to_edge_table(std_obj.e_cur, adj_list.node_id, adj_list.edgelist);
        // insert into EKEY: edge key table
        add_to_edgekey(ekey_obj.e_cur, adj_list.node_id, adj_list.edgelist);
        // insert into ADJ: edge and adjlist out tables
        add_to_edge_table(adj_obj.e_cur, adj_list.node_id, adj_list.edgelist);
        add_to_adjlist(adj_obj.cur, adj_list);

        // acquire a lock and update the global node_degree
        std::lock_guard<std::mutex> lock(lock_var);
        node_degrees[adj_list.node_id].out_degree = adj_list.edgelist.size();

        adj_list.clear();
    }
}

void insert_rev_edge_thread(int _tid)
{
    int tid = _tid;
    std::string filename = opts.dataset + "/in_";
    char c = (char)(97 + tid);
    filename.push_back('a');
    filename.push_back(c);

    worker_sessions adj_obj(conn_adj, "table:adjlistin", GraphType::Adj);

    reader::AdjReader adj_reader(filename);
    adjlist adj_list;
    while (adj_reader.get_next_adjlist(adj_list) == 0)
    {
        int ret;
        // insert into ADJ: inadjlist table
        add_to_adjlist(adj_obj.cur, adj_list);
        // acquire a lock and update the global node_degree
        std::lock_guard<std::mutex> lock(lock_var);
        // check if node_id is already present in the map and update the
        // in_degree
        if (node_degrees.find(adj_list.node_id) != node_degrees.end())
        {
            node_degrees[adj_list.node_id].in_degree = adj_list.edgelist.size();
        }
        else
        {
            node_degrees[adj_list.node_id].in_degree = adj_list.edgelist.size();
            node_degrees[adj_list.node_id].out_degree = 0;
        }

        adj_list.clear();
    }
}

void insert_nodes(size_t start, size_t end)
{
    worker_sessions std_node_obj(conn_std, "", GraphType::Std, false);
    worker_sessions adj_node_obj(conn_adj, "", GraphType::Adj, false);
    worker_sessions ekey_node_obj(conn_ekey, "", GraphType::EKey, false);

    for (int key = start; key <= end; ++key)
    {
        auto it = node_degrees.find(key);
        if (it != node_degrees.end())
        {
            // Process the element
            node to_insert;
            to_insert.id = key;
            to_insert.in_degree = it->second.in_degree;
            to_insert.out_degree = it->second.out_degree;
            // insert into STD: node table
            add_to_node_table(std_node_obj.n_cur, to_insert);
            // insert into EKEY: node table
            add_node_to_ekey(ekey_node_obj.e_cur, to_insert);
            // insert into ADJ: node table
            add_to_node_table(adj_node_obj.n_cur, to_insert);
        }
    }
}

int main(int argc, char *argv[])
{
    InsertOpts params(argc, argv);
    if (!params.parse_args())
    {
        params.print_help();
        return -1;
    }
    opts = params.make_graph_opts();
    std::string conn_config = "create,cache_size=10GB";
#ifdef STAT
    std::string stat_config =
        "statistics=(all),statistics_log=(wait=0,on_close=true";
    conn_config += "," + stat_config;
#endif
    // open connections to all three dbs
    make_connections(opts, conn_config);

    num_per_chunk = (int)(opts.num_edges / NUM_THREADS);
    //

    dump_config(opts, conn_config);
    std::cout << "dataset: " << opts.dataset << std::endl;

    // We first work on the out edges. We will read the edges from the file and
    // insert them into the edge table and the adjlist table
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_edge_thread(i);
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_rev_edge_thread(i);
    }

    // insert nodes into the node table
    const size_t totalKeys = node_degrees.size();
    const size_t keysPerThread =
        (totalKeys + NUM_THREADS - 1) / NUM_THREADS;  // Ceiling division

#pragma omp parallel num_threads(NUM_THREADS)
    {
        int tid = omp_get_thread_num();
        size_t start = tid * keysPerThread;
        size_t end = std::min((tid + 1) * keysPerThread - 1, totalKeys - 1);

        insert_nodes(start, end);
    }
    conn_adj->close(conn_adj, nullptr);
    conn_std->close(conn_std, nullptr);
    conn_ekey->close(conn_ekey, nullptr);

    return (EXIT_SUCCESS);
}
