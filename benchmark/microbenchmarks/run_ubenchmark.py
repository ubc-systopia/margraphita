import os, subprocess, sys, time

parmat = "/home/puneet/scratch/PaRMAT/Release/PaRMAT"
graph_dir="/drives/hdd_main"
db_dir="/graphs"
'''
We assume that the graphs have already been generated at : graph_dir + /s{i}_e8/graph_s{i}_e8
The database is assumed to be present where the bulk ingested DB was created :
    $DB_DIR/s{i}_e8
'''

for i in range(20, 27):
    vertices = 2 ** i
    edges = vertices * 8
    print(f"Benchmarking CSR for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./csr_seek_scan {vertices} {edges} {graph_dir}/s{i}_e8/graph_s{i}_e8"
    print(f"{cmd}\n")
    os.system(cmd)
    # run the adjlist benchmark
    print(f"\nBenchmarking AdjList for the s{i} graph with {vertices} vertices and {edges} edges:")
    cmd = f"./adjlist_seek_scan {graph_dir}/s{i}_e8/graph_s{i}_e8 {db_dir}/s{i}_e8 adj_rd_s{i}_e8"
    print(f"{cmd}")
    os.system(cmd)
    print('-' * 100)
