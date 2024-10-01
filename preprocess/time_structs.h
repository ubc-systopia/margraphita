#include <getopt.h>

#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>

#include "common_util.h"

class time_info
{
   public:
    long double insert_time = 0;
    long double read_time = 0;
    long double rback_time = 0;
    size_t num_inserted = 0;
    long num_rollbacks = 0;
    long num_failures = 0;
};
[[maybe_unused]] static auto dump_time_info = [](time_info &tinfo)
{
    std::cout << "Insert time: " << tinfo.insert_time << std::endl;
    std::cout << "Read time: " << tinfo.read_time << std::endl;
    std::cout << "Rollback time: " << tinfo.rback_time << std::endl;
    std::cout << "Num inserted: " << tinfo.num_inserted << std::endl;
    std::cout << "Num rollbacks: " << tinfo.num_rollbacks << std::endl;
    std::cout << "Num failures: " << tinfo.num_failures << std::endl;
};

class InsertOpts
{
   protected:
    int argc_;
    char **argv_;
    std::string argstr_ =
        "d:p:l:e:n:f:t:rum:w";  //! Construct this after you
                                //! finish the rest of this thing
    std::vector<std::string> help_strings_;

    std::string db_name;
    graph_opts opts{.create_new = true,
                    .read_optimize = false,
                    .is_directed = true,
                    .is_weighted = false,
                    .optimize_create = true};
    enum GraphType graph_type
    {
    };
    std::string logdir;
    bool read_optimize = false;

    void add_help_message(char opt,
                          const std::string &opt_arg,
                          const std::string &text)

    {
        char buf[100];
        snprintf(
            buf, 100, " -%c %-20s: %-54s", opt, opt_arg.c_str(), text.c_str());
        help_strings_.emplace_back(buf);
    }

   public:
    InsertOpts(int argc, char **argv) : argc_(argc), argv_(argv)
    {
        add_help_message('d', "db", "Name of the WT DB");
        add_help_message('p', "path", "Path to WT DB");
        add_help_message('l', "log", "logdir name");
        add_help_message('e', "edges", "number of edges in the input graph");
        add_help_message('n', "nodes", "number of nodes in the input graph");
        add_help_message('f', "file", "graph file (edgelist) to insert");
        add_help_message('t',
                         "type",
                         "type of representation. This can be either std, adj, "
                         "ekey, or all");
        add_help_message('r', "ropt", "Make the WT database read optimized");
        add_help_message('u', "undirected", "The graph is undirected");
        add_help_message('m', "mt", "number of threads to use");
        add_help_message('w', "weighted", "The graph is weighted");
    }

    bool virtual parse_args()
    {
        if (argc_ <= 1)
        {
            return false;
        }
        signed char opt_c;
        extern char *optarg;
        bool parse_result = true;

        while ((opt_c = (signed char)getopt(argc_, argv_, argstr_.c_str())) !=
               -1)

        {
            handle_args(opt_c, optarg);
        }
        return parse_result;
    }

    bool virtual handle_args(signed char opt, char *opt_arg)
    {
        bool ret_val = true;
        switch (opt)
        {
            case 'd':
                opts.db_name = optarg;
                break;
            case 'p':
                opts.db_dir = optarg;
                if ('/' == opts.db_dir.back())
                    opts.db_dir.pop_back();  // delete trailing '/'
                break;
            case 'l':
                logdir = optarg;
                if ('/' == logdir.back())
                    logdir.pop_back();  // delete trailing '/'
                if (!opts.db_name.empty())
                    opts.stat_log = logdir + "/" + opts.db_name;
                break;
            case 'e':
                opts.num_edges = strtod(optarg, nullptr);
                break;
            case 'n':
                opts.num_nodes = strtod(optarg, nullptr);
                break;
            case 'f':
                opts.dataset = optarg;
                break;
            case 't':
            {
                opts.type = handle_graph_type(opt_arg);
                break;
            }
            case 'r':
                opts.read_optimize = true;
                break;
            case 'u':
                opts.is_directed = false;
                break;
            case 'm':
                std::cout << opt << " " << optarg << std::endl;
                opts.num_threads = static_cast<int>(strtod(optarg, nullptr));
                break;
            case 'w':
                opts.is_weighted = true;
                break;
            case ':':
            /* missing option argument */
            case '?':
            default:
                ret_val = false;
        }
        return ret_val;
    }

    static GraphType handle_graph_type(char *opt_arg)

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

    [[nodiscard]] std::string get_logdir() const
    {
        if (opts.stat_log.empty())
        {
            return logdir + "/" + opts.db_name;
        }
        else
        {
            return opts.stat_log;
        }
    }

    [[nodiscard]] std::string get_type_str() const
    {
        if (graph_type == GraphType::Std)
        {
            return "std";
        }
        else if (graph_type == GraphType::EKey)
        {
            return "ekey";
        }

        else if (graph_type == GraphType::Adj)
        {
            return "adj";
        }
        else
        {
            return "all";
        }
    }

    [[nodiscard]] const graph_opts &make_graph_opts()

    {
        opts.stat_log = get_logdir() + "/" + opts.db_name;
        return opts;
    }
};