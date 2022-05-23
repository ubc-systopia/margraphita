#!/bin/bash
set -x
source ../../paths.sh
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

while getopts "l:pbi" o; do
    case "${o}" in
        (l)
            log_dir=${OPTARG}
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
        (*)
            usage
            ;;
    esac
done
if [ $OPTIND -eq 1 ]; then echo "No options were passed. Using ${log_dir} as log dir."; fi
log_file=${log_dir}/kron_insert.txt

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
        ./preprocess.sh -d "${graph_dir}" -f "${graph}" -o "${DB_DIR}/${dataset}" -m $dataset -n $n_nodes -e $n_edges -t $type -l $log_dir -p $preprocess -b $bulk_load -i $index_create &> ${log_file}
         echo "---------------------------------------------" >> ${log_file}
    done

    # /usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=single_thread_insert_time.txt --append ./single_threaded_graphapi_insert ${scale} &>> single_threaded_insert_log.txt
    
done
