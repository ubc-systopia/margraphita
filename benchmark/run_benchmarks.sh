#!/bin/bash
set -x
set -e

usage() { 
echo "Usage: $0 [-d log_dir"  
echo "log_dir : the absolute path to the directory where the benchmark results will be stored"
exit 1;}

TYPES=( "adj" "std" "ekey" )
DATASETS=( "cit-Patents" "s10_e8" )
RESULT=$HOME/scratch/margraphita/outputs #This is the directory where log files will get generated. If not passed in args, this will default to $HOME/scratch/margraphita/outputs/
DATADIR=/home/puneet/gen_graphs
COUNTS=10

while getopts "dh" o; do
    case "${o}" in
        (d)
            unset RESULT
            RESULT=${OPTARG%/}
            echo "Using ${RESULT} as the log directory."
            ;;
        (h)
            usage
            exit
            ;;
        (*)
            usage
            exit
            ;;
    esac
done

if [ $OPTIND -eq 1 ]; then echo "No options were passed. Using ${RESULTS} as log dir."; fi
echo $RESULT

for ((scale=11; scale <=20; scale ++ )); do
    DATASETS+=( "s${scale}_e8" )
done

run_pagerank()
{
for ds in "${DATASETS[@]}"
do
    for type in "${TYPES[@]}"
    do
        for trials in $(seq 1 10) #5 runs.
        do
        # PR does 10 iterations internally. I am doing 5 iterations of the whole
        # run.
        echo  "--------------------------------"  >>  ${RESULT}/pr/${ds}.txt
        date +"%c" >> ${RESULT}/pr/${ds}.txt
        ./pagerank -m ${type}_rd_${ds} -b PR -a $DATADIR/${ds} -s $ds -r -d -l $type -i 10 -t 0.0001 &>> ${RESULT}/pr/${ds}.txt
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
    echo  "--------------------------------" >> ${RESULT}/bfs/${ds}.txt
    date +"%c" >> ${RESULT}/bfs/${ds}.txt
    ./bfs -m "${type}_rd_${ds}" -b BFS -a $DATADIR/${ds}/"$type_rd_${ds}" -s $ds -r -d -l $type &>> ${RESULT}/bfs/${type}_rd_${ds}.txt
    done
done
}

run_tc()
{
for type in "${TYPES[@]}"
do
    for ds in "${DATASETS[@]}"
    do
        for trials in $(seq 1 10)
        do
        echo  "--------------------------------"  >> ${RESULT}/tc/${ds}.txt
        date +"%c" >> ${RESULT}/tc/${ds}.txt
        ./tc -m "$type_rd_${ds}" -b TC -a $DATADIR/${ds}/"$type_rd_${ds}" -s $ds -r -d -l $type &>> ${RESULT}/tc/${type}_rd_${ds}.txt
        done
    done
done
}

run_pagerank
run_bfs
run_tc
