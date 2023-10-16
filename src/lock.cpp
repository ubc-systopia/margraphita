#include "lock.h"

LockSet::LockSet()
{
    omp_init_lock(&node_num_lock);
    omp_init_lock(&edge_num_lock);
    omp_init_lock(&node_degree_lock);
    omp_init_lock(&insert_range_lock);
}

LockSet::~LockSet()
{
    omp_destroy_lock(&node_num_lock);
    omp_destroy_lock(&edge_num_lock);
    omp_destroy_lock(&node_degree_lock);
    omp_destroy_lock(&insert_range_lock);
}

omp_lock_t* LockSet::get_node_num_lock() { return &node_num_lock; }

omp_lock_t* LockSet::get_edge_num_lock() { return &edge_num_lock; }

omp_lock_t* LockSet::get_node_degree_lock() { return &node_degree_lock; }

omp_lock_t* LockSet::get_insert_range_lock() { return &insert_range_lock; }