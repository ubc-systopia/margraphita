//
// Created by puneet on 10/02/25.
//

#ifndef GRAPHAPI_MEM_USAGE_H
#define GRAPHAPI_MEM_USAGE_H

#include <sys/resource.h>

#include <iostream>
namespace mem_util
{
class mem_usage
{
 private:
  struct rusage pre, post;

 public:
  mem_usage() = default;

  void before()
  {
    if (getrusage(RUSAGE_SELF, &pre) != 0)
    {
      std::cerr << "Error getting resource usage\n";
    }
  }
  void after()
  {
    if (getrusage(RUSAGE_SELF, &post) != 0)
    {
      std::cerr << "Error getting resource usage\n";
    }
  }

  void print_diff()
  {
    std::cout << "Memory usage: " << post.ru_maxrss - pre.ru_maxrss << " KB\n";
    std::cout << "User CPU time: " << post.ru_utime.tv_sec - pre.ru_utime.tv_sec
              << "s\n";
    std::cout << "System CPU time: "
              << post.ru_stime.tv_sec - pre.ru_stime.tv_sec << "s\n";
    std::cout << "Major Page Faults: " << post.ru_majflt - pre.ru_majflt
              << "\n";
    std::cout << "Minor Page Faults: " << post.ru_minflt - pre.ru_minflt
              << "\n";
    std::cout << "Block input operations: " << post.ru_inblock - pre.ru_inblock
              << "\n";
    std::cout << "Block output operations: " << post.ru_oublock - pre.ru_oublock
              << "\n";
    std::cout << "Voluntary context switches: " << post.ru_nvcsw - pre.ru_nvcsw
              << "\n";
    std::cout << "Involuntary context switches: "
              << post.ru_nivcsw - pre.ru_nivcsw << "\n";
  }

  void print_usage(struct rusage &usage)
  {
    // Peak resident set size (in kilobytes)
    std::cout << "Peak memory usage: " << usage.ru_maxrss << " KB\n";
    // User CPU time
    std::cout << "User CPU time: " << usage.ru_utime.tv_sec << "s\n";
    // System CPU time
    std::cout << "System CPU time: " << usage.ru_stime.tv_sec << "s\n";
    // Major and Minor Page Faults
    std::cout << "Major Page Faults: " << usage.ru_majflt << "\n";
    std::cout << "Minor Page Faults: " << usage.ru_minflt << "\n";
    // Block input and output operations
    std::cout << "Block input operations: " << usage.ru_inblock << "\n";
    std::cout << "Block output operations: " << usage.ru_oublock << "\n";
    // Context Switches
    std::cout << "Voluntary context switches: " << usage.ru_nvcsw << "\n";
    std::cout << "Involuntary context switches: " << usage.ru_nivcsw << "\n";
  }
};
}  // namespace mem_util

#endif  // GRAPHAPI_MEM_USAGE_H