#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <fmt/core.h>
#include <getopt.h>

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "common_util.h"

struct cmdline_opts : graph_opts
{
    // option to create a new db
    bool create_indices = false;
    bool exit_on_create = false;
    // app opts
    int num_trials = 16;
    node_id_t start_vertex = -1;
    // pagerank opts
    double tolerance = 1e-4;
    int iterations = 1;
    bool print_stats = false;
    // SSSP options
    edgeweight_t delta_value = 1;

    // dump the options
    void print_config(const std::string &filename)
    {
        std::ofstream out;
        out.open(filename, std::ios::out);
        out << "CREATE_NEW: " << create_new << std::endl;
        out << "CREATE_NEW: " << create_new << std::endl;
        out << "READ_OPTIMIZE: " << read_optimize << std::endl;
        out << "DIRECTED: " << is_directed << std::endl;
        out << "WEIGHTED: " << is_weighted << std::endl;
        out << "DB_NAME: " << db_name << std::endl;
        out << "DB_DIR: " << db_dir << std::endl;
        out << "OPTIMIZE_CREATE: " << optimize_create << std::endl;
        out << "CONN_CONFIG: " << conn_config << std::endl;
        out << "STAT_LOG: " << stat_log << std::endl;
        out << "GRAPH_TYPE: " << type << std::endl;
        out << "DATASET: " << dataset << std::endl;
        out << "NUM_NODES" << num_nodes << std::endl;
        out << "NUM_EDGES" << num_edges << std::endl;
        out << "CREATE_INDICES: " << create_indices << std::endl;
        out << "EXIT_ON_CREATE: " << exit_on_create << std::endl;
        out << "NUM_TRIALS: " << num_trials << std::endl;
        out << "START_VERTEX: " << start_vertex << std::endl;
        out << "TOLERANCE: " << tolerance << std::endl;
        out << "ITERATIONS: " << iterations << std::endl;
        out.close();
    }
};

class CmdLineBase
{
   protected:
    int argc_;
    char **argv_;

    std::string argstr_ =
        "p:m:g:"          // required args
        "s:nordwl:hz:a";  //! Construct this after you finish the
                          //! rest of this thing
    std::vector<std::string> help_strings_;
    cmdline_opts opts;

    void add_help_message(char opt,
                          const std::string &opt_arg,
                          const std::string &text)
    {
        std::string buf;
        // use fmtlib to format the string
        fmt::format_to(std::back_inserter(buf),
                       " -{0} {1:20}: {2:100}",
                       opt,
                       opt_arg,
                       text);
        help_strings_.emplace_back(buf);
    }

   public:
    CmdLineBase(int argc, char **argv) : argc_(argc), argv_(argv)
    {
        add_help_message('h', "h", "Print this help message");
        // required opts
        add_help_message('p', "path", "(Required) DB directory");
        add_help_message('m', "db_name", "(Required) Name of the WT DB");
        add_help_message('g',
                         "graph_type",
                         "(Required) The representation of the graph."
                         "Can be one of "
                         "adjlist, std, edgekey");

        // Optional opts
        add_help_message('s',
                         "dataset_name",
                         "(Optional) The name of the dataset being used. It is "
                         "set to match the DB name if not provided.");
        add_help_message(
            'n', "create_new", "(Optional) Create a new DB (default = false");
        add_help_message(
            'o',
            "create_optimized",
            "(Optional) If set, then indices are not created while "
            "inserting. Default = false.");
        add_help_message(
            'r',
            "read_optimize",
            "(Optional) Is the DB read optimized. Default = false");
        add_help_message('d',
                         "directed",
                         "(Optional) Is the graph directed. Default = false.");
        add_help_message('w',
                         "weighted",
                         "(Optional) Is the graph weighted. Default = false");
        add_help_message(
            'l',
            "Log dir",
            "(Optional) Output dir for log. If not set, CWD is used");
        add_help_message('z',
                         "conn_config",
                         "(Optional) Provide a WT connection config string to "
                         "the open_conn call");
        add_help_message(
            'a',
            "print_stats",
            "(Optional) Print stats after running the app. Default = false");

        if (argc_ == 1)
        {
            std::cout << "No arguments provided. Use -h for help." << std::endl;
            std::exit(1);
        }
    }

    bool parse_args()
    {
        signed char opt_c;
        while ((opt_c = getopt(argc_, argv_, argstr_.c_str())) != -1)
            handle_args(opt_c, optarg);

        // DB_DIR cannot be empty
        if (opts.db_dir.empty())
        {
            std::cout << "No specified database path. Use -h for help."
                      << std::endl;
            return false;
        }
        // IF DB_NAME cannot be empty
        if (opts.db_name.empty())
        {
            std::cout << "No database name provided. Inferring from the path "
                         "of the database path."
                      << std::endl;
            return false;
        }
        // IF DATASET_NAME is empty, make it match db_name
        if (opts.dataset.empty()) opts.dataset = opts.db_name;

        if (opts.stat_log.empty())
        {
            // use CWD if not set
            opts.stat_log = std::filesystem::current_path().string();
        }

        return true;
    }

    virtual void handle_args(signed char opt, char *opt_arg)
    {
        switch (opt)
        {
            case 'p':
                opts.db_dir = std::string(opt_arg);
                break;
            case 'm':
                opts.db_name = std::string(opt_arg);
                break;
            case 'g':
                opts.type = handle_graph_type(opt_arg);
                break;
                // Optional opts
            case 's':
                opts.dataset = std::string(opt_arg);
                break;
            case 'n':
                opts.create_new = true;
                break;
            case 'o':
                opts.optimize_create = true;
                break;
            case 'r':
                opts.read_optimize = true;
                break;
            case 'd':
                opts.is_directed = true;
                break;
            case 'w':
                opts.is_weighted = true;
                break;
            case 'l':
                opts.stat_log = std::string(opt_arg);
                break;
            case 'z':
                opts.conn_config = std::string(opt_arg);
                break;
            case 'a':
                opts.print_stats = true;
                break;
            case 'h':
                print_help();
                break;
        }
    }

    GraphType handle_graph_type(char *opt_arg)
    {
        GraphType type;
        if (strcmp(opt_arg, "std") == 0)
        {
            type = GraphType::Std;
        }
        else if (strcmp(opt_arg, "adj") == 0)
        {
            type = GraphType::Adj;
        }
        else if (strcmp(opt_arg, "ekey") == 0)
        {
            type = GraphType::EKey;
        }
        else if (strcmp(opt_arg, "") == 0)
        {
            throw GraphException(
                "Please specify the graph representation being used "
                "for this benchmark. Use -h for help.");
        }
        else
        {
            throw GraphException("Unrecognized graph representation");
        }
        return type;
    }

    void print_help()
    {
        for (const std::string &h : help_strings_) std::cout << h << std::endl;
        std::exit(0);
    }

    cmdline_opts get_parsed_opts() { return opts; };
};

class CmdLineInsert : public CmdLineBase
{
   public:
    CmdLineInsert(int argc, char **argv) : CmdLineBase(argc, argv)
    {
        argstr_ += "xe";
        add_help_message('x',
                         "create_index",
                         "(Optional) Flag to create indexes. Default = false");
        add_help_message(
            'e',
            "exit_on_create",
            "(Optional) Exit after creating the db. Default = false");
    }

    void handle_args(signed char opt, char *opt_arg) override
    {
        switch (opt)
        {
            case 'x':
                opts.create_indices = true;
                break;
            case 'e':
                opts.exit_on_create = true;
                break;
            default:
                CmdLineBase::handle_args(opt, opt_arg);
        }
    }
};

class CmdLineApp : public CmdLineBase
{
   public:
    CmdLineApp(int argc, char **argv) : CmdLineBase(argc, argv)
    {
        argstr_ += "#:v:";  // add v: for start_vertex
        add_help_message(
            '#',
            "num_trials",
            "(Optional) Number of trials for each benchmark. Defaults to " +
                std::to_string(opts.num_trials));
        add_help_message(
            'v',
            "start_vertex",
            "(Optional) Starting vertex id. If not provided, then a random "
            "vertex is picked. ");
    }

    void handle_args(signed char opt, char *opt_arg) override
    {
        switch (opt)
        {
            case '#':
                opts.num_trials = (int)strtol(opt_arg, nullptr, 0);
                break;
            case 'v':
                opts.start_vertex = (node_id_t)strtol(opt_arg, nullptr, 0);
                break;
            default:
                CmdLineBase::handle_args(opt, opt_arg);
        }
    }
};

class PageRankOpts : public CmdLineApp
{
   public:
    PageRankOpts(int argc, char **argv, double _tolerance, int _iters)
        : CmdLineApp(argc, argv)
    {
        argstr_ += "i:t:";
        opts.tolerance = _tolerance;
        opts.iterations = _iters;
        add_help_message(
            'i',
            "i",
            "The number of iterations of PageRank to run. Defaults to " +
                std::to_string(opts.iterations));
        add_help_message(
            't',
            "tolerance",
            "the tolerance to use for terminating PR. Defaults to " +
                std::to_string(opts.tolerance));
    }

    void handle_args(signed char opt, char *opt_arg) override
    {
        switch (opt)
        {
            case 'i':
                opts.iterations = (int)atoi(opt_arg);
                break;
            case 't':
                opts.tolerance = std::stod(opt_arg);
                break;
            default:
                CmdLineApp::handle_args(opt, opt_arg);
        }
    }
};

class SSSPOpts : public CmdLineApp
{
   public:
    SSSPOpts(int argc, char **argv, edgeweight_t _delta)
        : CmdLineApp(argc, argv)
    {
        argstr_ += "D:";
        opts.delta_value = _delta;
        add_help_message('D',
                         "delta",
                         "The delta value to use for SSSP. Defaults to " +
                             std::to_string(opts.delta_value));
    }

    void handle_args(signed char opt, char *opt_arg) override
    {
        switch (opt)
        {
            case 'D':
                opts.delta_value = std::stoi(opt_arg);
                break;
            default:
                CmdLineApp::handle_args(opt, opt_arg);
        }
    }
};

#endif
