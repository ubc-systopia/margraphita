#ifndef _CSV_LOG_H
#define CSV_LOG_H
#include <string>

#include "benchmark_definitions.h"
#include "common.h"

void print_csv_info(std::string name,
                    int starting_node,
                    bfs_info *info,
                    double time_from_outside,
                    std::string csv_logdir)
{
    fstream FILE;
    std::string _name = csv_logdir + "/" + name + "_bfs.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name, benchmark, starting node_id, num_visited, "
                "sum_out_deg, total_time_taken_usecs, time_from_main\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",bfs," << starting_node << "," << info->num_visited << ","
         << info->sum_out_deg << "," << info->time_taken << ","
         << time_from_outside << "\n";

    FILE.close();
}

void print_csv_info(std::string name, tc_info &info, std::string csv_logdir)
{
    fstream FILE;
    std::string _name = csv_logdir + "/" + name + "_tc.csv";
    if (access(_name.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(_name, ios::out | ios::app);
        FILE << "#db_name, benchmark, cycle_count, cycle_time_usec, "
                "trust_count, trust_time_usecs\n";
    }
    else
    {
        FILE.open(_name, ios::out | ios::app);
    }

    FILE << name << ",tc," << info.cycle_count << "," << info.cycle_time << ","
         << info.trust_count << "," << info.trust_time << "\n";

    FILE.close();
}

void print_to_csv(std::string name,
                  std::vector<double> &times,
                  std::string csvfile)
{
    fstream FILE;
    if (access(csvfile.c_str(), F_OK) == -1)
    {
        // The file does not exist yet.
        FILE.open(csvfile, ios::out | ios::app);
        FILE << "#db_name,bmark,map_t,i0,i1,i2,i3,i4,i5,i6,i7,i8,i9"
             << "\n ";
    }
    else
    {
        FILE.open(csvfile, ios::out | ios::app);
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

template <typename T>
void print_map(int N, T *pr_map, int p_next)
{
    ofstream FILE;
    FILE.open("pr_out.txt", ios::out | ios::ate);
    for (int i = 0; i < N; i++)
    {
        FILE << pr_map[i].id << "\t" << pr_map[i].p_rank[p_next] << "\n";
    }
    FILE.close();
}
#endif