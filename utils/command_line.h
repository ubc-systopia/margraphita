#ifndef COMMAND_LINE_H
#define COMMAND_LINE_H

#include <getopt.h>

#include <iostream>
#include <string>
#include <vector>

class CmdLineBase
{
   protected:
    int argc_;
    char **argv_;
    std::string argstr_ =
        "m:a:nrdhwl:ob:s:ef:";  //! Construct this after you finish the rest of
                                //! this thing
    std::vector<std::string> help_strings_;

    std::string db_name;
    std::string graph_type;
    std::string benchmark;
    std::string dataset;
    std::string log_dir;
    bool create_new = false;
    bool read_optimize = false;
    bool directed = true;
    bool weighted = false;
    bool optimized_create = false;
    bool exit_on_create = false;

    std::string db_path;  // This contains the path to the db dir.

    void add_help_message(char opt, std::string opt_arg, std::string text)
    {
        char buf[100];
        snprintf(
            buf, 100, " -%c %-20s: %-54s", opt, opt_arg.c_str(), text.c_str());
        help_strings_.push_back(buf);
    }

   public:
    CmdLineBase(int argc, char **argv) : argc_(argc), argv_(argv)
    {
        add_help_message('m', "db_ name", "Name of the WT DB");
        add_help_message(
            'b', "benchmark", "Which benchmark to run -- PR, BFS, CC, TC");
        add_help_message('a', "path", "DB directory");
        add_help_message('s', "s", "the name of the dataset being used");
        add_help_message('n', "new", "create new DB (defalt = true");
        add_help_message('o',
                         "create_optimized",
                         "if set, then indices are not created while "
                         "inserting. Default false.");
        add_help_message(
            'r', "read_optimize", "Is the DB read optimized. Default true");
        add_help_message(
            'd', "directed", "Is the graph directed. Default true.");
        add_help_message(
            'w', "weighted", "is the graph weighted. default false");
        add_help_message('l',
                         "representation type",
                         "The representation of the graph. Can be one of "
                         "adjlist, std, edgekey");
        add_help_message(
            'e', "exit_on_create", "Use falg to create the DB and exit");
        add_help_message('f', "csv logdir", "output dir for log");
        add_help_message('h', "h", "Print this help message");
    }

    bool parse_args()
    {
        signed char opt_c;
        extern char *optarg;
        while ((opt_c = getopt(argc_, argv_, argstr_.c_str())) != -1)
        {
            handle_args(opt_c, optarg);
        }
        if (create_new && db_path == "")
        {
            std::cout << "There is no input graph specified for insertion into "
                         "the database. Use -h for help."
                      << std::endl;
            return false;
        }
        if (graph_type == "")
        {
            std::cout << "Please specify the graph representation being used "
                         "for this benchmark. Use -h for help."
                      << std::endl;
            return false;
        }
        return true;
    }
    void virtual handle_args(signed char opt, char *opt_arg)
    {
        switch (opt)
        {
            case 'm':
                db_name = std::string(opt_arg);
                break;
            case 'a':
                db_path = std::string(opt_arg);
                break;
            case 's':
                dataset = std::string(opt_arg);
                break;
            case 'h':
                print_help();
                break;
            case 'n':
                create_new = true;
                break;
            case 'f':
                log_dir = std::string(opt_arg);
                break;
            case 'w':
                weighted = true;
                break;
            case 'e':
                exit_on_create = true;
                break;
            case 'r':
                read_optimize = true;
                break;
            case 'd':
                directed = true;
                break;
            case 'l':
                graph_type = std::string(opt_arg);
                break;
            case 'o':
                optimized_create = true;
                break;
            case 'b':
                benchmark = std::string(opt_arg);
                break;
        }
    }

    void print_help()
    {
        for (std::string h : help_strings_) std::cout << h << std::endl;
        std::exit(0);
    }

    std::string get_db_name() const { return db_name; }
    std::string get_graph_type() const { return graph_type; }
    std::string get_db_path() const { return db_path; }
    std::string get_dataset() const { return dataset; }
    std::string get_benchmark() const { return benchmark; }
    std::string get_csv_logdir() const { return log_dir; }
    bool is_create_new() const { return create_new; }
    bool is_read_optimize() const { return read_optimize; }
    bool is_directed() const { return directed; }
    bool is_weighted() const { return weighted; }
    bool is_create_optimized() const { return optimized_create; }
    bool is_exit_on_create() const { return exit_on_create; }
};

class CmdLineApp : public CmdLineBase
{
    int num_trials = 16;
    int start_vertex_ = -1;

   public:
    CmdLineApp(int argc, char **argv) : CmdLineBase(argc, argv)
    {
        argstr_ += "cv:";  // add v: for start_vertex
        add_help_message(
            'c',
            "c",
            "perform c trials. Defaults to " + std::to_string(num_trials));
        add_help_message('v',
                         "v",
                         "Starting vertex id. If not provided, then a random "
                         "vertex is picked. ");
    }

    void handle_args(signed char opt, char *opt_arg) override
    {
        switch (opt)
        {
            case 'c':
                num_trials = atoi(opt_arg);
                break;
            case 'v':
                start_vertex_ = atoi(opt_arg);
                break;
            default:
                CmdLineBase::handle_args(opt, opt_arg);
        }
    }
    int get_num_trials() const { return num_trials; }
    int start_vertex() const { return start_vertex_; }
};

class PageRankOpts : public CmdLineApp
{
    double _tolerance;  // used for PR
    int _iterations;    // Used for PR
    bool create_indices = false;

   public:
    PageRankOpts(int argc, char **argv, double tolerance, int iters)
        : CmdLineApp(argc, argv), _tolerance(tolerance), _iterations(iters)
    {
        argstr_ += "i:t:x";
        add_help_message(
            'i',
            "i",
            "The number of iterations of PageRank to run. Defaults to " +
                std::to_string(iters));
        add_help_message(
            't',
            "t",
            "the tolerance to use for terminating PR. Defaults to " +
                std::to_string(tolerance));
        add_help_message('x',
                         "x",
                         "Used to specify if indices need to be created. "
                         "Defaults to false. ");
    }

    void handle_args(signed char opt, char *opt_arg) override
    {
        switch (opt)
        {
            case 'i':
                _iterations = atoi(opt_arg);
                break;
            case 't':
                _tolerance = std::stod(opt_arg);
                break;
            case 'x':
                create_indices = true;
                break;
            default:
                CmdLineApp::handle_args(opt, opt_arg);
        }
    }
    double tolerance() const { return _tolerance; }
    int iterations() const { return _iterations; }
    int is_index_create() const { return create_indices; }
};

#endif
