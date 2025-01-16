
#include <sys/mman.h>

#include <iostream>
#include <shared_mutex>
#include <vector>

#include "command_line.h"
#include "common_util.h"
#include "graph.h"
#include "graph_engine.h"
#include "times.h"

int main(int argc, char *argv[])
{
  cout << "Initializing Database" << endl;
  CmdLineInsert init_cli(argc, argv);
  if (!init_cli.parse_args())
  {
    return -1;
  }

  cmdline_opts opts = init_cli.get_parsed_opts();
  opts.stat_log += "/" + opts.db_name;
  // create stat_log directory if not exists
  std::filesystem::create_directories(opts.stat_log);

  if (opts.conn_config.empty()) opts.conn_config = "cache_size=10GB";
  opts.dump_cmd_config(opts.stat_log + "/init_db_config.txt");
  const int THREAD_NUM = 1;

  Times t;
  t.start();
  GraphEngine graphEngine(THREAD_NUM, opts);
  t.stop();
  cout << "Graph created in " << t.t_micros() << endl;

  // must use derived class object here
  if (opts.exit_on_create)  // Exit after creating the db
  {
    graphEngine.close_graph();
    exit(0);
  }

  // create_indices does not apply to adjacency lists
  if (opts.create_indices && opts.type != GraphType::Adj)
  {
    t.start();
    graphEngine.create_indices();
    t.stop();
    cout << "Indices created in " << t.t_micros() << endl;
  }
  graphEngine.close_graph();
  exit(0);
}