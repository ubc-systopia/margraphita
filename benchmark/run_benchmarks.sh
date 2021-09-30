#!/bin/bash
set -x
set -e

TYPES=( "adj" "std"  "ekey" )
DATASETS=( "cit-Patents" ) #"twitter" )
DRIVE=( "hdd" "ssd")
BENCHMARKS=( "pagerank" "bfs" "tc" )
RESULT=/home/puneet/scratch/margraphita/outputs
COUNTS=10
run_pagerank()
{
for ds in "${DATASETS[@]}"
do
    for type in "${TYPES[@]}"
    do
        for trials in $(seq 1 $COUNTS)
        do
        ./pagerank -m ${type}_rd_${ds} -b PR -a ./db -s $ds -r -d -l $type -i 10 -t 0.0001 &>> ${RESULT}/pr/${ds}.txt
        done
    done
done
}

run_bfs()
{
for ds in "${DATASETS[@]}"
do
    for type in "${TYPES[@]}"
    do
    # The bfs binary handles running the tests 10 times for 10 different random nodes. 
        ./bfs -m "${type}_rd_${ds}" -b BFS -a ./db  -s $ds -r -d -l $type &>> ${RESULT}/bfs/${type}_rd_${ds}.txt
    done
done
}

# run_tc()
# {
# for type in "${TYPES[@]}"
# do
#     for ds in "${DATASETS[@]}"
#     do
#         ./tc -m "$type_rd_${ds}" -b BFS -a /ssd_graph/"$type_rd_${ds}" -s $ds -r -d -l $type
#     done
# done
# }

#run_pagerank
run_bfs
# run_tc
