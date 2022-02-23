#!/bin/bash
set -x
source paths.sh
if [ -z "$KRON_GRAPHS_PATH" ]
then
    echo "Please set paths in BUILD_DIR/benchmarks/paths.sh"
    exit -1
fi

usage() {
    echo "Usage: $0 -l log_dir "
    echo "If not provided, cwd is used."
    exit 1;
}
log_dir=$(pwd)

if [ -z "$*" ]; then echo "No log dir provided. Using cwd."; fi
while getopts ":l:" o; do
    case "${o}" in
        (l)
            log_dir=${OPTARG}
            ;;
        (*)
            usage
            ;;
    esac
done
log_file=${log_dir}/kron_insert.txt

TYPES=( "std" "adj" "ekey" )

for ((scale=10; scale <=27; scale ++ )); do
	n_edges=$(bc <<<"8*2^$scale")
    n_nodes=$(bc <<<"2^$scale")
	
    filename="graph_s${scale}_e8"
	dirname="${KRON_GRAPHS_PATH}/s${scale}_e8"

    date >> ${log_file}

    for type in "${TYPES[@]}"
    do
        echo " Now inserting scale $scale, saved in $dirname/$filename" >> ${log_file}
        ./preprocess.sh -f "${dirname}/${filename}" -o "${dirname}" -m "s${scale}_e8" -n $n_nodes -e $n_edges -t $type -l $log_dir &> ${log_file}
         echo "---------------------------------------------" >> ${log_file}
    done

    # /usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=single_thread_insert_time.txt --append ./single_threaded_graphapi_insert ${scale} &>> single_threaded_insert_log.txt
    
done
