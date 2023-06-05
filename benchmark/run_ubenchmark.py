import os, subprocess, sys, time

parmat = "/home/puneet/scratch/PaRMAT/Release/PaRMAT"
graph_dir="/drives/hdd_main"
db_dir="/graphs"
num_samples = 1000
'''
We assume that the graphs have already been generated at : graph_dir + /s{i}_e8/graph_s{i}_e8
The database is assumed to be present where the bulk ingested DB was created :
    $DB_DIR/s{i}_e8
'''

for i in range(20, 21):
    vertices = 2**i
    edges = vertices*8
    print(f"Running for the s{i} graph with {vertices} vertices and {edges} edges:\n")
    cmd = f"./ubenchmark {vertices} {edges} {graph_dir}/s{i}_e8/graph_s{i}_e8 {db_dir}/s{i}_e8 adj_rd_s{i}_e8"
    print(cmd)
    os.system(cmd)
