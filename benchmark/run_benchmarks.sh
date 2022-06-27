#!/bin/bash
set -x
set -e

source ../../paths.sh
usage() {
echo "Usage: $0 [-d log_dir -r run_type]"
echo "log_dir : the absolute path to the directory where the benchmark results will be stored"
echo "run_type: One of all, regular, stats, perf. defaults to all."
exit 1;}

TYPES=( "adj" "std" "ekey" )
DATASETS=( "s10_e8" ) #"cit-Patents" )
RESULT=$GRAPH_PROJECT_DIR/outputs/on_mars
RUNTYPE=all
COUNTS=10

while getopts ":d:r:h" o; do
    case "${o}" in
        (d)
            unset RESULT
            RESULT=$OPTARG
            echo "Using ${RESULT} as the log directory."
            ;;
        (r)
            unset RUNTYPE
            RUNTYPE=$OPTARG
            echo "Running a $RUNTYPE benchmark."
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

for ((scale=11; scale <=20; scale ++ )); do
    DATASETS+=( "s${scale}_e8" )
done

run_pagerank()
{
    mkdir -p ${RESULT}/pr/wt_stats
    mkdir -p ${RESULT}/pr/wt_stats/iter_get
    mkdir -p ${RESULT}/pr/wt_stats/iter_map
    for ds in "${DATASETS[@]}"
    do
        for type in "${TYPES[@]}"
        do
            ##Regular run
            if [[ $RUNTYPE == "all" || $RUNTYPE == "regular" ]]; then
                for trials in $(seq 1 8) #5 runs.
                do
                #Non-iterator; map-based
                echo "${type} $(date)" >> ${RESULT}/pr/old_pr_${type}_rd_${ds}.txt
                ${RELEASE_PATH}/benchmark/pagerank -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001 &>> ${RESULT}/pr/old_pr_${type}_rd_${ds}.txt
                echo  "--------------------------------"  >>  ${RESULT}/pr/old_pr_${type}_rd_${ds}.txt

                # Iterator with mmap for node values
                echo "${type} $(date)" >> ${RESULT}/pr/iter_map_${type}_rd_${ds}.txt
                ${RELEASE_PATH}/benchmark/pr_iter_map -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001 >> ${RESULT}/pr/iter_map_${type}_rd_${ds}.txt
                echo  "--------------------------------" >> ${RESULT}/pr/iter_map_${type}_rd_${ds}.txt

                ## Iterator that calls get_out_degree for each in_node
                echo "${type} $(date)" >> ${RESULT}/pr/iter_getoutdeg_${type}_rd_${ds}.txt
                ${RELEASE_PATH}/benchmark/pr_iter_getout -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001 >> ${RESULT}/pr/iter_getoutdeg_${type}_rd_${ds}.txt
                echo  "--------------------------------" >> ${RESULT}/pr/iter_getoutdeg_${type}_rd_${ds}.txt
                done
            fi

            #Run to collect WT_STATS
            if [[ $RUNTYPE == "all" || $RUNTYPE == "stats" ]]; then
                ${PROFILE_PATH}/benchmark/pagerank -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr/wt_stats/ -r -d -l $type -i 10 -t 0.0001
                wt_stat_timestamp=$(date +%d.%H)
                mv ${RESULT}/pr/wt_stats/${type}_rd_${ds}/WiredTigerStat.${wt_stat_timestamp} ${RESULT}/pr/wt_stats/${type}_rd_${ds}/pagerank_${type}_rd_${ds}.${wt_stat_timestamp}.md

                ${PROFILE_PATH}/benchmark/pr_iter_map -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr/wt_stats/iter_map/ -r -d -l $type -i 10 -t 0.0001
                wt_stat_timestamp=$(date +%d.%H)
                mv ${RESULT}/pr/wt_stats/iter_map/${type}_rd_${ds}/WiredTigerStat.${wt_stat_timestamp} ${RESULT}/pr/wt_stats/iter_map/${type}_rd_${ds}/pagerank_${type}_rd_${ds}.${wt_stat_timestamp}.md

                ${PROFILE_PATH}/benchmark/pr_iter_getout -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr/wt_stats/iter_get/ -r -d -l $type -i 10 -t 0.0001
                wt_stat_timestamp=$(date +%d.%H)
                mv ${RESULT}/pr/wt_stats/iter_get/${type}_rd_${ds}/WiredTigerStat.${wt_stat_timestamp} ${RESULT}/pr/wt_stats/iter_get/${type}_rd_${ds}/pagerank_${type}_rd_${ds}.${wt_stat_timestamp}.md
            fi

            #Run with perf
            if [[ $RUNTYPE == "all" || $RUNTYPE == "perf" ]]; then
                perf record --call-graph fp -o ${RESULT}/pr/${type}_rd_${ds}_pr_perf.dat ${STATS_PATH}/benchmark/pagerank -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001
                perf record --call-graph fp -o ${RESULT}/pr/${type}_rd_${ds}_pr_iter_map_perf.dat ${STATS_PATH}/benchmark/pr_iter_map -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001
                perf record --call-graph fp -o ${RESULT}/pr/${type}_rd_${ds}_pr_iter_getout_perf.dat ${STATS_PATH}/benchmark/pr_iter_getout -m ${type}_rd_${ds} -b PR -a $DB_DIR/${ds} -s $ds -f ${RESULT}/pr -r -d -l $type -i 10 -t 0.0001
            fi

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
    ${RELEASE_PATH}/benchmark/bfs -m "${type}_rd_${ds}" -b BFS -a $DB_DIR/${ds} -s $ds -f ${RESULT}/bfs -r -d -l $type &>> ${RESULT}/bfs/${type}_rd_${ds}.txt

    #Run to collect WT_STATS
    ${PROFILE_PATH}/benchmark/bfs -m "${type}_rd_${ds}" -b BFS -a $DB_DIR/${ds} -s $ds -f ${RESULT}/bfs -r -d -l $type &>> ${RESULT}/bfs/${type}_rd_${ds}.txt

    #Run with perf
    perf record -a --call-graph fp -o ${RESULT}/bfs/${type}_rd_${ds}_perf.dat ${STATS_PATH}/benchmark/bfs -m "${type}_rd_${ds}" -b BFS -a $DB_DIR/${ds} -s $ds -f ${RESULT}/bfs -r -d -l $type
    done

done
}

run_tc()
{
for type in "${TYPES[@]}"
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
        ${PROFILE_PATH}/benchmark/tc -m "${type}_rd_${ds}" -b TC -a $DB_DIR/${ds} -s $ds -f ${RESULT}/tc -r -d -l $type &>> ${RESULT}/tc/${type}_rd_${ds}.txt

        #Run with perf
        perf record -a --call-graph fp -o ${RESULT}/tc/${type}_rd_${ds}_perf.dat ${STATS_PATH}/benchmark/tc -m "${type}_rd_${ds}" -b TC -a $DB_DIR/${ds} -s $ds -f ${RESULT}/tc -r -d -l $type
    done
done
}

run_pagerank
run_bfs
run_tc
