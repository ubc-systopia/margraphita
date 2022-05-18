#!/bin/bash
set -x
set -e

source paths.sh
usage() {
echo "Usage: $0 [-d log_dir"
echo "log_dir : the absolute path to the directory where the benchmark results will be stored"
exit 1;}

#TYPES=( "adj" "std" "ekey19)
TYPES=( "adj" )
DATASETS=( "cit-Patents" )
RESULT=/home/jackgong/margraphita/outputs/on_mars
COUNTS=10

while getopts ":d:h" o; do
    case "${o}" in
        (d)
            unset RESULT
            RESULT=$OPTARG
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

mkdir -p ${RESULT}/pr
mkdir -p ${RESULT}/tc
mkdir -p ${RESULT}/bfs

#for ((scale=13; scale <=15; scale ++ )); do
#    DATASETS+=( "s${scale}_e8" )
#done

run_pagerank()
{
#dataset_pr=( "s18_e8" "s19_e8" "s20_e8" )
#for ds in "${dataset_pr[@]}"
for ds in "${DATASETS[@]}"
do
    for type in "${TYPES[@]}"
    do
        ##Regular run
        for trials in $(seq 1 8) #5 runs.
        do

        echo  "--------------------------------"  >>  ${RESULT}/pr/${ds}.txt
        date +"%c" >> ${RESULT}/pr/${ds}.txt
        ${RELEASE_PATH}/benchmark/pagerank -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001 -z cache_size=10GB &>> ${RESULT}/pr/${ds}.txt
        done

        #Run to collect WT_STATS
        #mkdir -p ${RESULT}/pr/${type}_rd_${ds}
        #${STATS_PATH}/benchmark/pagerank -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001
        #Run with perf
        #perf record --call-graph dwarf -o ${PERF_OUT_DIR}/pr/${type}_rd_${ds}_perf.dat ${PROFILE_PATH}/benchmark/pagerank -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001
    done
done
}

run_bfs()
{
for ds in "${DATASETS[@]}"
do
    types_bfs=( "std" )
    for type in "${types_bfs[@]}"
    do
    # The bfs binary handles running the tests 10 times for 10 different random nodes.
    echo  "--------------------------------" >> ${RESULT}/bfs/${ds}.txt
    date +"%c" >> ${RESULT}/bfs/${ds}.txt
    ${RELEASE_PATH}/benchmark/bfs -m "${type}_rd_${ds}" -b BFS -a $DB_DIR/${ds} -s $ds -f ${RESULT}/bfs -r -d -l $type &>> ${RESULT}/bfs/${type}_rd_${ds}.txt

    #Run to collect WT_STATS
    #mkdir -p ${RESULT}/bfs/${type}_rd_${ds} #needed to store the wt_log
    #${STATS_PATH}/benchmark/bfs -m "${type}_rd_${ds}" -b BFS -a $DB_DIR/${ds} -s $ds -f ${RESULT}/bfs -r -d -l $type &>> ${RESULT}/bfs/${type}_rd_${ds}.txt

    #Run with perf
    #perf record --call-graph dwarf -o ${PERF_OUT_DIR}/bfs/${type}_rd_${ds}_perf.dat ${PROFILE_PATH}/benchmark/bfs -m "${type}_rd_${ds}" -b BFS -a $DB_DIR/${ds} -s $ds -f ${RESULT}/bfs -r -d -l $type
    done

done
}

run_tc()
{
types_tc=( "ekey" "std" )
#dataset_tc=( "s18_e8" "s19_e8" "s20_e8" )
for type in "${types_tc[@]}"
do
    for ds in "${DATASETS[@]}"
    do
        for trials in $(seq 1 8)
        do
        echo  "--------------------------------"  >> ${RESULT}/tc/${ds}.txt
        date +"%c" >> ${RESULT}/tc/${ds}.txt
        ${RELEASE_PATH}/benchmark/tc -m "${type}_rd_${ds}" -b TC -a $DB_DIR/${ds} -s $ds -f ${RESULT}/tc -r -d -l $type &>> ${RESULT}/tc/${type}_rd_${ds}.txt
        done

        #Run to collect WT_STATS
        #mkdir -p ${RESULT}/tc/${type}_rd_${ds}
        #${STATS_PATH}/benchmark/tc -m "${type}_rd_${ds}" -b TC -a $DB_DIR/${ds} -s $ds -f ${RESULT}/tc -r -d -l $type &>> ${RESULT}/tc/${type}_rd_${ds}.txt

        #Run with perf
        #perf record --call-graph dwarf -o ${PERF_OUT_DIR}/tc/${type}_rd_${ds}_perf.dat ${PROFILE_PATH}/benchmark/tc -m "${type}_rd_${ds}" -b TC -a $DB_DIR/${ds} -s $ds -f ${RESULT}/tc -r -d -l $type
    done
done
}

#run_pagerank
run_bfs
#run_tc
