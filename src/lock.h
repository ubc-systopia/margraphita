#ifndef LOCK
#define LOCK

#ifdef MACOSX
#include "/usr/local/opt/libomp/include/omp.h"
#else
#include <omp.h>
#endif

class LockSet
{
   public:
    LockSet();
    ~LockSet();
    omp_lock_t* get_node_num_lock();
    omp_lock_t* get_edge_num_lock();
    omp_lock_t* get_node_degree_lock();
    omp_lock_t* get_insert_range_lock();

   private:
    omp_lock_t node_num_lock;
    omp_lock_t edge_num_lock;
    omp_lock_t node_degree_lock;
    omp_lock_t insert_range_lock;
};

#endif