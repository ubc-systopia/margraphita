#!/bin/bash
set -x
source $GRAPH_PROJECT_DIR/build/paths.sh
if [ -z "$KRON_GRAPHS_PATH" ] || [ -z "$DB_DIR" ]
then
    echo "Please set paths in BUILD_DIR/benchmarks/paths.sh"
    exit -1
fi

usage() {
    echo "Usage: $0 -l log_dir -p -b -i"
    echo "If not provided, cwd is used."
    echo "if provided, -p indicates that the graphs have already been preprocessed. We skip the preprocessing step"
    echo "if provided, -b indicates that the graphs have been inserted and we are only interested in index creation "
    echo "-i if passed, create indices"
    exit 1;
}

log_dir=$(pwd)
preprocess=1
bulk_load=1
index_create=0
use_api_for_insert=0
use_tx=1

while getopts "l:pbiat" o; do
    case "${o}" in
        (l)
            log_dir=${OPTARG}
            ;;
        (a)
            use_api_for_insert=1
            ;;
        (p)
            preprocess=0
            ;;
        (b)
            bulk_load=0
            echo "found"
            ;;
        (i)
            index_create=1
            ;;
        (t)
            use_tx=0
            ;;
        (*)
            usage
            ;;
    esac
done
if [ $OPTIND -eq 1 ]; then echo "No options were passed. Using ${log_dir} as log dir."; fi
log_file=${log_dir}/kron_insert.txt

n_edges=0
n_nodes=0
graph_dir=""
dataset=""

preprocess_func () {
    ./preprocess.sh -d $1 -f $2 -o "${DB_DIR}/${3}" -m $3 -n $4 -e $5 -t $6 -l $log_dir -p $preprocess -b $bulk_load -i $index_create &> ${log_file}
}

api_insert_func () {
    ./transaction_check.sh -d $1 -f $2 -o "${DB_DIR}/${3}" -m $3 -n $4 -e $5 -t $6 -l $log_dir -p $preprocess -b $bulk_load -i $index_create -x $use_tx
}

TYPES=( "std" "adj" "ekey" )

for ((scale=10; scale <=15; scale ++ )); do
	n_edges=$(bc <<<"8*2^$scale")
    n_nodes=$(bc <<<"2^$scale")
    date >> ${log_file}

    dataset="s${scale}_e8"
    graph_dir="${KRON_GRAPHS_PATH}/${dataset}"
    graph="${KRON_GRAPHS_PATH}/${dataset}/graph_${dataset}"

    for type in "${TYPES[@]}"
    do
        echo " Now inserting scale $scale, saved in $graph" >> ${log_file}
        if [ $use_api_for_insert -eq 1 ]
        then
            api_insert_func ${graph_dir} ${graph} ${dataset} ${n_nodes} ${n_edges} $type
        else
            preprocess_func ${graph_dir} ${graph} ${dataset} ${n_nodes} ${n_edges} $type
        fi
        echo "---------------------------------------------" >> ${log_file}
    done

    # /usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=single_thread_insert_time.txt --append ./single_threaded_graphapi_insert ${scale} &>> single_threaded_insert_log.txt

done

##check if you need to insert these
echo "Now inserting CIT"
dataset="cit-Patents"
graph_dir="${KRON_GRAPHS_PATH}/${dataset}"
graph="${KRON_GRAPHS_PATH}/${dataset}/${dataset}.txt"
n_nodes=3774768
n_edges=16518948
for type in "${TYPES[@]}"
    do
    preprocess_func ${graph_dir} ${graph} ${dataset} ${n_nodes} ${n_edges} $type
    done

# ./preprocess.sh -d /drives/hdd_main/cit-Patents -f /drives/hdd_main/cit-Patents/cit-Patents.txt -o home/puneet/scratch/margraphita/db/cit-Patents -m cit-Patents  -t std -l /home/puneet/scratch/margraphita/build/release/benchmark -p 1 -b 1 -i 0
