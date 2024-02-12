#ifndef CSV_LOG_H
#define CSV_LOG_H
#include <fstream>
#include <string>

#include "benchmark_definitions.h"
#include "common_util.h"

void print_csv_info(const std::string &name,
                    int starting_node,
                    bfs_info *info,
                    double time_from_outside,
                    const std::string &csv_logdir)
{
    std::ofstream FILE;
    std::string _name = csv_logdir + "/" + name + "_bfs.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, std::ios::out | std::ios::app);
        FILE << "#db_name, benchmark, starting node_id, num_visited, "
                "sum_out_deg, total_time_taken_usecs, time_from_main\n";
    }
    else
    {
        FILE.open(_name, std::ios::out | std::ios::app);
    }

    FILE << name << ",bfs," << starting_node << "," << info->num_visited << ","
         << info->sum_out_deg << "," << info->time_taken << ","
         << time_from_outside << "\n";

    FILE.close();
}

void print_csv_info(const std::string &name,
                    tc_info &info,
                    const std::string &csv_logdir)
{
    std::ofstream FILE;
    std::string _name = csv_logdir + "/" + name + "_tc.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, std::ios::out | std::ios::app);
        FILE << "#db_name, benchmark, cycle_count, cycle_time_usec, "
                "trust_count, trust_time_usecs\n";
    }
    else
    {
        FILE.open(_name, std::ios::out | std::ios::app);
    }

    FILE << name << ",tc," << info.cycle_count << "," << info.cycle_time << ","
         << info.trust_count << "," << info.trust_time << "\n";

    FILE.close();
}

void print_csv_info(const std::string &name,
                    cc_info &info,
                    const std::string &csv_logdir)
{
    std::ofstream FILE;
    std::string _name = csv_logdir + "/" + name + "_cc.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, std::ios::out | std::ios::app);
        FILE << "#db_name, benchmark, component_count, time_taken_usec\n";
    }
    else
    {
        FILE.open(_name, std::ios::out | std::ios::app);
    }

    FILE << name << ",cc," << info.component_count << "," << info.time_taken
         << "\n";

    FILE.close();
}

void print_csv_info(const std::string &name,
                    sssp_info &info,
                    const std::string &csv_logdir)
{
    std::ofstream FILE;
    std::string _name = csv_logdir + "/" + name + "_sssp.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, std::ios::out | std::ios::app);
        FILE << "#db_name, benchmark, time_taken_usec\n";
    }
    else
    {
        FILE.open(_name, std::ios::out | std::ios::app);
    }

    FILE << name << ",sssp," << info.time_taken << "\n";

    FILE.close();
}

void print_csv_info(const std::string &name,
                    iter_info &info,
                    const std::string &csv_logdir)
{
    std::ofstream FILE;
    std::string _name = csv_logdir + "/" + name + "_sssp.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, std::ios::out | std::ios::app);
        FILE << "#db_name, benchmark, time_taken_usec\n";
    }
    else
    {
        FILE.open(_name, std::ios::out | std::ios::app);
    }

    FILE << name << ",sssp," << info.time_taken << "\n";

    FILE.close();
}

void print_to_csv(const std::string &name,
                  std::vector<long double> &times,
                  const std::string &csvfile)
{
    std::ofstream FILE;
    if (access(csvfile.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(csvfile, std::ios::out | std::ios::app);
        FILE << "#db_name,bmark,map_t,i0,i1,i2,i3,i4,i5,i6,i7,i8,i9"
             << "\n ";
    }
    else
    {
        FILE.open(csvfile, std::ios::out | std::ios::app);
    }

    FILE << name << ",pr,";
    for (int i = 0; i < (int)times.size(); i++)
    {
        FILE << times[i];
        if (i != (int)times.size() - 1)
        {
            FILE << ",";
        }
        else
        {
            FILE << "\n";
        }
    }
    FILE.close();
}

#endif