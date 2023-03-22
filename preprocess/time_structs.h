#include <stdlib.h>

#include <cstdint>
#include <sstream>
#include <string>

#include "common.h"

using namespace std;

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
static auto dump_time_info = [](time_info &tinfo)
{
    cout << "Insert time: " << tinfo.insert_time << endl;
    cout << "Read time: " << tinfo.read_time << endl;
    cout << "Rollback time: " << tinfo.rback_time << endl;
    cout << "Num inserted: " << tinfo.num_inserted << endl;
    cout << "Num rollbacks: " << tinfo.num_rollbacks << endl;
    cout << "Num failures: " << tinfo.num_failures << endl;
};

class InsertOpts
{
   protected:
    int argc_;
    char **argv_;
    std::string argstr_ =
        "d:p:l:e:n:f:t:rum:";  //! Construct this after you
                               //! finish the rest of this thing
    std::vector<std::string> help_strings_;

    std::string db_name;

    enum GraphType graph_type
    {
    };
    std::string logdir;
    std::string db_path;  // This contains the path to the db dir.
    std::string dataset;
    double num_edges{};
    double num_nodes{};

    bool directed = true;
    bool read_optimize = false;
    int num_threads = 1;

    void add_help_message(char opt,
                          const std::string &opt_arg,
                          const std::string &text)

    {
        char buf[100];
        snprintf(
            buf, 100, " -%c %-20s: %-54s", opt, opt_arg.c_str(), text.c_str());
        help_strings_.push_back(buf);
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
                db_name = optarg;
                break;
            case 'p':
                db_path = optarg;
                if ('/' == db_path.back())
                    db_path.pop_back();  // delete trailing '/'
                break;
            case 'l':
                logdir = optarg;
                if ('/' == logdir.back())
                    logdir.pop_back();  // delete trailing '/'
                break;
            case 'e':
                num_edges = strtod(optarg, NULL);
                break;
            case 'n':
                num_nodes = strtod(optarg, NULL);
                break;
            case 'f':
                dataset = optarg;
                break;
            case 't':
            {
                graph_type = handle_graph_type(opt_arg);
                break;
            }
            case 'r':
                read_optimize = true;
                break;
            case 'u':
                directed = false;
                break;
            case 'm':
                cout << opt << " " << optarg << endl;
                num_threads = static_cast<int>(strtod(optarg, NULL));
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

    [[nodiscard]] std::string get_db_name() const { return db_name; }
    [[nodiscard]] std::string get_db_path() const { return db_path; }
    [[nodiscard]] std::string get_logdir() const { return logdir; }
    [[nodiscard]] std::string get_dataset() const { return dataset; }
    [[nodiscard]] double get_num_nodes() const { return num_nodes; }
    [[nodiscard]] double get_num_edges() const { return num_edges; }
    [[nodiscard]] bool is_read_optimize() const { return read_optimize; }
    [[nodiscard]] bool is_directed() const { return directed; }
    [[nodiscard]] int get_num_threads() const { return num_threads; }
    [[nodiscard]] GraphType get_graph_type() const { return graph_type; }
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

    [[nodiscard]] graph_opts make_graph_opts() const

    {
        graph_opts opts;
        opts.create_new = true;
        opts.optimize_create = true;
        opts.is_directed = is_directed();
        opts.read_optimize = is_read_optimize();
        opts.is_weighted = false;
        opts.db_dir = get_db_path();
        opts.db_name = get_db_name();
        opts.type = get_graph_type();
        opts.stat_log = get_logdir() + "/" + get_db_name();

        return opts;
    }
};