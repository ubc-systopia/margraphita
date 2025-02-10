#include "bulk_insert_low_mem.h"

void insert_edge_thread(int _tid, bool is_weighted = false)
{
  int tid = _tid;
  std::string filename = opts.dataset + "/out_";
  int first_char = tid / 26;
  int second_char = tid % 26;
  char c1 = (char)(97 + first_char);
  char c2 = (char)(97 + second_char);
  filename.push_back(c1);
  filename.push_back(c2);

  worker_sessions adj_obj(conn_adj, "table:adjlistout", GraphType::Adj);
  worker_sessions split_ekey_out(
      conn_split_ekey, "table:edge_out", GraphType::SplitEKey);

  reader::AdjReader adj_reader(filename);
  adjlist adj_list;
  int edge_count = 0;
  int adj_count = 0;
  while (adj_reader.get_next_adjlist(adj_list) == 0)
  {
    add_to_edgekey(
        split_ekey_out.e_cur, adj_list.node_id, adj_list.edgelist, is_weighted);
    add_to_edge_table(adj_obj.e_cur,
                      adj_list.node_id,
                      adj_list.edgelist,
                      &edge_count,
                      is_weighted);

    add_to_adjlist(adj_obj.cur, adj_list);

    //    node_degrees[adj_list.node_id].out_degree = adj_list.edgelist.size();
    degrees d = {0, static_cast<degree_t>(adj_list.edgelist.size())};
    {
      degree_map::accessor acc;
      if (node_degrees.insert(acc, adj_list.node_id))
      {
        acc->second = d;
      }
    }  // for releasing the accessor lock.
    adj_count++;
    /**if (adj_count % 100000 == 0)
    {
      std::cout << "Thread " << tid << " inserted " << adj_count
                << " adjlists that had" << edge_count << " edges" << std::endl;
    }**/
    adj_list.clear();
  }
  std::cout << "Thread " << tid << " inserted " << adj_count
            << " adjlists that had" << edge_count << " edges" << std::endl;
}

/**
 * THIS CAN OVERWRITE THE ADJACENCY LISTS OF THE NODES. GET the in_adjlist and
 * merge.
 * @param _tid
 * @param is_directed
 */
void insert_rev_edge_thread(int _tid, bool is_directed)
{
  int tid = _tid;
  std::string filename = opts.dataset + "/in_";
  int first_char = tid / 26;
  int second_char = tid % 26;
  char c1 = (char)(97 + first_char);
  char c2 = (char)(97 + second_char);
  filename.push_back(c1);
  filename.push_back(c2);

  std::string adj_table_name, ekey_table_name;
  if (is_directed)
  {
    adj_table_name = "table:adjlistin";
    ekey_table_name = "table:edge_in";
  }
  else
  {
    adj_table_name = "table:adjlistout";
    ekey_table_name = "table:edge_out";
  }
  worker_sessions adj_obj(conn_adj, adj_table_name, GraphType::Adj);
  worker_sessions split_ekey_in(
      conn_split_ekey, ekey_table_name, GraphType::SplitEKey);

  reader::AdjReader adj_reader(filename);
  adjlist adj_list;
  while (adj_reader.get_next_adjlist(adj_list) == 0)
  {
    // insert into ADJ: inadjlist table
    add_to_adjlist(adj_obj.cur, adj_list);
    add_to_edgekey(split_ekey_in.e_cur, adj_list.node_id, adj_list.edgelist);
    // get the node degree from the map and update the in_degree
    {
      degree_map::accessor acc;
      if (node_degrees.find(acc, adj_list.node_id))
      {
        acc->second.in_degree = adj_list.edgelist.size();
      }
      else
      {
        node_degrees.insert(acc, adj_list.node_id);
        acc->second = {static_cast<degree_t>(adj_list.edgelist.size()), 0};
      }
    }
    adj_list.clear();
  }
}

void insert_nodes()
{
  tbb::parallel_for(
      node_degrees.range(),
      [](tbb::concurrent_hash_map<node_id_t, degrees>::range_type &r)
      {
        worker_sessions adj_node_obj(conn_adj, "", GraphType::Adj, false);
        worker_sessions split_ekey_node_obj(
            conn_split_ekey, "", GraphType::SplitEKey, false);
        int count = 0;
        for (auto it = r.begin(); it != r.end(); it++)
        {
          // insert into ADJ: node table
          add_to_node_table(adj_node_obj.n_cur,
                            it->first,
                            it->second.in_degree,
                            it->second.out_degree);
          // insert into SPLIT_EKEY_OUT table
          // for nodes
          add_node_to_ekey(split_ekey_node_obj.e_cur,
                           it->first,
                           it->second.in_degree,
                           it->second.out_degree);
          count++;
        }
        //        std::cout << "inserted " << count << " nodes" << std::endl;
      });
}

void debug_dump_nodes()
{
  std::ofstream out("dump_node_degrees.txt", std::ofstream::out);
  std::vector<int> keys;
  for (tbb::concurrent_hash_map<node_id_t, degrees>::const_iterator it =
           node_degrees.begin();
       it != node_degrees.end();
       ++it)
  {
    keys.push_back(it->first);
  }

  // Step 2: Sort the keys
  std::sort(keys.begin(), keys.end());

  // Step 3: Print the sorted keys0
  out << "Sorted keys: ";
  for (const int &key : keys)
  {
    out << key << "\n";
  }
  out << std::endl;
  out.close();
}

void dump_mdata(int key, char *key_str, WT_CURSOR *cursor)
{
  cursor->set_key(cursor, key);
  cursor->search(cursor);
  WT_ITEM item;
  cursor->get_value(cursor, &item);
  std::cout << "Key: " << key_str << " Value: " << *(node_id_t *)item.data
            << std::endl;
}

void update_metadata(const graph_opts &_opts)
{
  std::cout << "Number of nodes: " << _opts.num_nodes << std::endl;
  std::cout << "Count of node_degrees: " << node_degrees.size() << std::endl;
  // std::cout << "Number of nodes: " << _opts.num_nodes
  //           << std::endl;
  std::cout << "Number of edges: " << _opts.num_edges << std::endl;

  auto [key_min, key_max] = get_min_max_key();
  std::cout << "Min node id: " << key_min << std::endl;
  std::cout << "Max node id: " << key_max << std::endl;

  for (auto conn : {conn_adj, conn_split_ekey})
  {
    worker_sessions obj(conn, "table:metadata", GraphType::META, false);
    WT_CURSOR *cursor = obj.metadata;
    // Key min
    add_metadata(
        MetadataKey::min_node_id, (char *)&key_min, sizeof(key_min), cursor);
    // Key max
    add_metadata(
        MetadataKey::max_node_id, (char *)&key_max, sizeof(key_max), cursor);
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
  for (auto conn :
       {conn_adj, conn_split_ekey})  //, conn_ekey, conn_split_ekey})
  {
    worker_sessions obj(conn, "table:metadata", GraphType::META, false);
    WT_CURSOR *cursor = obj.metadata;
    dump_mdata(MetadataKey::min_node_id, "min_node_id", cursor);
    dump_mdata(MetadataKey::max_node_id, "max_node_id", cursor);
    dump_mdata(MetadataKey::num_nodes, "num_nodes", cursor);
    dump_mdata(MetadataKey::num_edges, "num_edges", cursor);
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
  assert(opts.is_directed == false);
  std::string conn_config = "create,cache_size=10GB";
#ifdef STAT
  std::string stat_config =
      "statistics=(all),statistics_log=(wait=0,on_close=true";
  conn_config += "," + stat_config;
#endif

  // open connections to all three dbs
  make_connections(opts, conn_config);

  num_per_chunk = (int)(opts.num_edges / opts.num_threads);
  dump_config(opts, conn_config);
  std::cout << "dataset: " << opts.dataset << std::endl;

  // We first work on the out edges. We will read the edges from the file and
  // insert them into the edge table and the adjlist table
  std::cout << "weighted? " << opts.is_weighted << std::endl;
  Times t;
  t.start();
#pragma omp parallel for num_threads(opts.num_threads)
  for (int i = 0; i < opts.num_threads; i++)
  {
    insert_edge_thread(i, opts.is_weighted);
  }

  t.stop();
  std::cout << "Time taken to insert edges: " << t.t_secs() << "s" << std::endl;
  t.start();
#pragma omp parallel for num_threads(opts.num_threads)
  for (int i = 0; i < opts.num_threads; i++)
  {
    insert_rev_edge_thread(i, opts.is_directed);
  }
  t.stop();
  std::cout << "Time taken to insert rev edges: " << t.t_secs() << "s"
            << std::endl;
  // insert nodes into the node table
  t.start();

  insert_nodes();

  t.stop();
  std::cout << "Time taken to insert nodes: " << t.t_secs() << "s" << std::endl;
  // debug_dump_nodes();

  update_metadata(opts);

  conn_adj->close(conn_adj, nullptr);
  conn_split_ekey->close(conn_split_ekey, nullptr);

  return (EXIT_SUCCESS);
}
