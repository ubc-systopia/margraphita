#ifndef LOCK
#define LOCK

#include <omp.h>

class LockSet
{
   public:
    LockSet();
    ~LockSet();
    omp_lock_t* get_node_num_lock();
    omp_lock_t* get_edge_num_lock();
    omp_lock_t* get_node_degree_lock();

   private:
    omp_lock_t node_num_lock;
    omp_lock_t edge_num_lock;
    omp_lock_t node_degree_lock;
};

#endif