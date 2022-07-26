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

extern std::unordered_map<node_id_t, std::vector<node_id_t>> in_adjlist;
extern std::unordered_map<node_id_t, std::vector<node_id_t>> out_adjlist;
extern std::mutex lock;

#endif