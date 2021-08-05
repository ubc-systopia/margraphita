#ifndef BULK_INSERT_H
#define BULK_INSERT_H
#include "common.h"
#include "reader.h"
#include <thread>
#include <atomic>

#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <chrono>
#include <unistd.h>

#define NUM_THREADS 10

extern std::unordered_map<int, std::vector<int>> in_adjlist;
extern std::unordered_map<int, std::vector<int>> out_adjlist;
extern std::mutex lock;

#endif