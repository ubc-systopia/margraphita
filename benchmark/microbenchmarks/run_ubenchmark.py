import os
import subprocess
import sys
import time

parmat = "/home/puneet/scratch/PaRMAT/Release/PaRMAT"
graph_dir = "/drives/hdd_main"
db_dir = "/graphs"
'''
We assume that the graphs have already been generated at : graph_dir + /s{i}_e8/graph_s{i}_e8
The database is assumed to be present where the bulk ingested DB was created :
    $DB_DIR/s{i}_e8
'''
print ("number of cpus:", os.cpu_count())
pid = os.getpid()
print ("cpu affinity:", os.sched_getaffinity(pid))
os.sched_setaffinity(pid, {0, 1})

for i in range(20, 21):
    vertices = 2 ** i
    edges = vertices * 8
    cmd = f"wc -l {graph_dir}/s{i}_e8/graph_s{i}_e8_nodes | cut -d' ' -f1"
    vertices = os.popen(cmd).read()

    # run the adjlist benchmark
    print(
        f"\nBenchmarking AdjList for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./adjlist_seek_scan {graph_dir}/s{i}_e8/graph_s{i}_e8 {db_dir}/s{i}_e8 adj_rd_s{i}_e8"
    print(f"{cmd}")
    os.system(cmd)
    # run adjlist_iter benchmark
    print(
        f"\nBenchmarking AdjList for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./adjlist_iter_seek_scan {graph_dir}/s{i}_e8/graph_s{i}_e8 {db_dir}/s{i}_e8 adj_rd_s{i}_e8"
    print(f"{cmd}")
    os.system(cmd)
    # run the std benchmark
    print(
        f"\nBenchmarking AdjList for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./std_seek_scan {graph_dir}/s{i}_e8/graph_s{i}_e8 {db_dir}/s{i}_e8 std_rd_s{i}_e8"
    print(f"{cmd}")
    os.system(cmd)
    # run the ekey benchmark
    print(
        f"\nBenchmarking AdjList for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./ekey_seek_scan {graph_dir}/s{i}_e8/graph_s{i}_e8 {db_dir}/s{i}_e8 ekey_rd_s{i}_e8"
    print(f"{cmd}")
    os.system(cmd)
    # run the CSR benchmark
    print(
        f"Benchmarking CSR for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./csr_seek_scan {vertices} {edges} {graph_dir}/s{i}_e8/graph_s{i}_e8_sorted"
    print(f"{cmd}\n")
    os.system(cmd)
    print('-' * 100)
