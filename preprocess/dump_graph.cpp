//
// Created by puneet on 15/08/24.
//
#include <fmt/core.h>
#include <getopt.h>

#include <iostream>
#include <shared_mutex>
#include <vector>

#include "common_util.h"
#include "graph.h"
#include "graph_engine.h"
using namespace std;

int main(int argc, char *argv[])
{
  graph_opts opts;
  opts.create_new = false;
  opts.optimize_create = true;  // no indexes while inserting
  opts.is_directed =
      false;  // <-- Make sure to set this to true for directed graphs
  opts.is_weighted = true;

  if (argc != 5)
  {
    std::cout << "Usage: ./dump_graph <db_home> <table_name> "
                 "<graph_type> <num_records>"
              << std::endl;
    exit(1);
  }
  for (int i = 0; i < argc; i++)
  {
    std::cout << "argv[" << i << "] = " << argv[i] << std::endl;
  }
  // We are now only accepting the path to the DB folder. We need to construct
  // the db dir and db name from this.
  //  for path: /a/b/c/d, db_dir = /a/b/c, db_name = d

  std::string db_path = argv[1];
  opts.db_dir = db_path.substr(0, db_path.find_last_of('/'));
  opts.db_name = db_path.substr(db_path.find_last_of('/') + 1);

  std::string table_name = argv[2];
  GraphType type;

  if (strcmp(argv[3], "adj") == 0)
  {
    type = GraphType::Adj;
  }
  else if (strcmp(argv[3], "split_ekey") == 0)
  {
    type = GraphType::SplitEKey;
  }
  else
  {
    throw GraphException("Unrecognized graph representation");
  }
  opts.type = type;
  // number of records to dump
  int num_records = atoi(argv[4]);

  GraphEngine graphEngine(1, opts);

  if (opts.type == GraphType::Adj)
  {
    auto *graph = dynamic_cast<AdjList *>(graphEngine.create_graph_handle());
    if (table_name == METADATA)
      graph->dump_meta_data();
    else
      graph->dump_table(table_name, num_records);
    graph->close();
  }
  //  else if (opts.type == GraphType::EKey)
  //  {
  //    auto *graph = dynamic_cast<EdgeKey
  //    *>(graphEngine.create_graph_handle()); if (table_name == METADATA)
  //      graph->dump_meta_data();
  //    else
  //      graph->dump_table(table_name, num_records);
  //    graph->close();
  //  }
  else if (opts.type == GraphType::SplitEKey)
  {
    auto *graph =
        dynamic_cast<SplitEdgeKey *>(graphEngine.create_graph_handle());
    if (table_name == METADATA)
      graph->dump_meta_data();
    else
      graph->dump_table(table_name, num_records);
    graph->close();
  }
  else
  {
    throw GraphException("Unrecognized graph representation");
  }
  graphEngine.close_graph();
}