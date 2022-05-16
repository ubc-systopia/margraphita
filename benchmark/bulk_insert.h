#ifndef BULK_INSERT_H
#define BULK_INSERT_H
#include <getopt.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "reader.h"

#define NUM_THREADS 10

extern std::unordered_map<int64_t, std::vector<int64_t>> in_adjlist;
extern std::unordered_map<int64_t, std::vector<int64_t>> out_adjlist;
extern std::mutex lock;

#endif