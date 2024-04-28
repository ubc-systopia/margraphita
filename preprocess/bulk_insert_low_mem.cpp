#include "bulk_insert_low_mem.h"

void insert_edge_thread(int _tid, bool is_weighted = false)
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
    while (adj_reader.get_next_adjlist(adj_list) == 0)
    {
        // insert into STD: edge table
        add_to_edge_table(
            std_obj.e_cur, adj_list.node_id, adj_list.edgelist, is_weighted);
        add_to_edgekey(
            ekey_obj.e_cur, adj_list.node_id, adj_list.edgelist, is_weighted);
        add_to_edge_table(
            adj_obj.e_cur, adj_list.node_id, adj_list.edgelist, is_weighted);

        add_to_adjlist(adj_obj.cur, adj_list);

        // acquire a lock and update the global node_degree
#pragma omp critical
        {
            node_degrees[adj_list.node_id].out_degree =
                adj_list.edgelist.size();
        }

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
        // insert into ADJ: inadjlist table
        add_to_adjlist(adj_obj.cur, adj_list);
        // acquire a lock and update the global node_degree
#pragma omp critical
        {
            // check if node_id is already present in the map and update the
            // in_degree
            if (node_degrees.find(adj_list.node_id) != node_degrees.end())
            {
                node_degrees[adj_list.node_id].in_degree =
                    adj_list.edgelist.size();
            }
            else
            {
                node_degrees[adj_list.node_id].in_degree =
                    adj_list.edgelist.size();
                node_degrees[adj_list.node_id].out_degree = 0;
            }
        }

        adj_list.clear();
    }
}

void debug_dump_edges()
{
    worker_sessions ekey_node_obj(conn_ekey, "", GraphType::EKey);
    debug_print_edges(ekey_node_obj.e_cur, GraphType::EKey, "ekey_edges");
}

void insert_nodes()
{
    worker_sessions std_node_obj(conn_std, "", GraphType::Std, false);
    worker_sessions adj_node_obj(conn_adj, "", GraphType::Adj, false);
    worker_sessions ekey_node_obj(conn_ekey, "", GraphType::EKey, false);
    int count = 0;
    // iterate over the node_degrees and insert into the node table
    for (auto it = node_degrees.begin(); it != node_degrees.end(); it++)
    {
        node to_insert;
        to_insert.id = it->first;
        to_insert.in_degree = it->second.in_degree;
        to_insert.out_degree = it->second.out_degree;
        // insert into STD: node table
        add_to_node_table(std_node_obj.n_cur, to_insert);
        // insert into EKEY: node table
        add_node_to_ekey(ekey_node_obj.e_cur, to_insert);
        // insert into ADJ: node table
        add_to_node_table(adj_node_obj.n_cur, to_insert);
        count++;
    }
    std::cout << "inserted " << count << " nodes" << std::endl;
}

void dump_mdata(int key, WT_CURSOR *cursor)
{
    cursor->set_key(cursor, key);
    cursor->search(cursor);
    WT_ITEM item;
    cursor->get_value(cursor, &item);
    std::cout << "Key: " << key << " Value: " << *(node_id_t *)item.data
              << std::endl;
}

void update_metadata(const graph_opts &_opts)
{
    node_id_t key_min = node_degrees.begin()->first;
    node_id_t key_max = node_degrees.rbegin()->first;
    std::cout << "Number of nodes: " << _opts.num_nodes << std::endl;
    std::cout << "Cout of node_degrees: " << node_degrees.size() << std::endl;
    // std::cout << "Number of nodes: " << _opts.num_nodes
    //           << std::endl;
    std::cout << "Number of edges: " << _opts.num_edges << std::endl;

    std::cout << "Min node id: " << key_min << std::endl;
    std::cout << "Max node id: " << key_max << std::endl;

    for (auto conn : {conn_std, conn_adj, conn_ekey})
    {
        worker_sessions obj(conn, "table:metadata", GraphType::META, false);
        WT_CURSOR *cursor = obj.metadata;
        // Key min
        add_metadata(MetadataKey::min_node_id,
                     (char *)&key_min,
                     sizeof(key_min),
                     cursor);
        // Key max
        add_metadata(MetadataKey::max_node_id,
                     (char *)&key_max,
                     sizeof(key_max),
                     cursor);
        // Number of nodes
        add_metadata(MetadataKey::num_nodes,
                     (char *)&opts.num_nodes,
                     sizeof(opts.num_nodes),
                     cursor);
        // Number of edges
        add_metadata(MetadataKey::num_edges,
                     (char *)&opts.num_edges,
                     sizeof(opts.num_edges),
                     cursor);
    }
    for (auto conn : {conn_std, conn_adj, conn_ekey})
    {
        worker_sessions obj(conn, "table:metadata", GraphType::META, false);
        WT_CURSOR *cursor = obj.metadata;
        dump_mdata(MetadataKey::min_node_id, cursor);
        dump_mdata(MetadataKey::max_node_id, cursor);
        dump_mdata(MetadataKey::num_nodes, cursor);
        dump_mdata(MetadataKey::num_edges, cursor);
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
    dump_config(opts, conn_config);
    std::cout << "dataset: " << opts.dataset << std::endl;

    // We first work on the out edges. We will read the edges from the file and
    // insert them into the edge table and the adjlist table
    std::cout << "weighted? " << opts.is_weighted << std::endl;
#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_edge_thread(i, opts.is_weighted);
    }

#pragma omp parallel for num_threads(NUM_THREADS)
    for (int i = 0; i < NUM_THREADS; i++)
    {
        insert_rev_edge_thread(i);
    }

    // insert nodes into the node table

    insert_nodes();
    // debug_dump_edges();

    update_metadata(opts);

    conn_adj->close(conn_adj, nullptr);
    conn_std->close(conn_std, nullptr);
    conn_ekey->close(conn_ekey, nullptr);

    return (EXIT_SUCCESS);
}
