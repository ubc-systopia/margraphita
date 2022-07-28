#!/bin/bash
set -e
set -x

source ../../paths.sh

usage() {
echo "Usage: $0 [-d <graph_dir> -f <graph_filename> -o <output_path> -m <dataset_name> -n<edge count> -e<edge count> -t <type> -i -p -l <log_dir>"
echo "graph_dir, graph_filename: directory where graph_filename is stored"
echo "output_path: absolute path to the **directory** that should contain the output
files"
echo "dataset_name : used to construct the DB name and to name the output files"
echo "type is one of std, ekey, adj. Use all for all types of tables"
echo "i - if this flag is passed, indices are created."
echo "p - if this flag is passed we skip to bulk insert directly"
echo "l - save log file in the passed log_dir instead of the PWD"
echo "b - if this flag is passed, we skip insertion"
echo "if cit-Patents is in ~/datasets/cit-Patents/cit-Patents.txt"
echo " -o ~/datasets/cit-Patents"
echo " -m cit-Patents"
echo " -t std"
exit 1;}

index_create=0
RESULT=$(pwd)
preprocess=1
insert=1
NUM_THREADS=10

if [ -z "$*" ]; then echo "No args provided"; usage; fi
while getopts "d:f:l:o:m:e:n:t:p:i:b:" o; do
    case "${o}" in
        (d)
            graph_dir=${OPTARG%/}
            ;;
        (f)
            graph=${OPTARG%/}
            ;;
        (l)
            RESULT=${OPTARG%/}
            ;;
        (o)
            output=${OPTARG%/}
            ;;
        (m)
            dataset=${OPTARG%/}
            ;;
        (e)
            edgecnt=${OPTARG}
            ;;
        (n)
            nodecnt=${OPTARG}
            ;;
        (p)
            preprocess=${OPTARG}
            ;;
        (t)
            type=${OPTARG}
            ;;
        (i)
            index_create=${OPTARG}
            ;;
        (b)
            insert=${OPTARG}
            ;;
        (*)
            usage
            ;;
    esac
done

TIME_CMD=/usr/bin/time
NUM_LINES=$(wc -l ${graph} | cut -d' '  -f1)


if [ $preprocess -eq 1 ]
then
    ##remove all lines that begin with a comment
    #cp ${graph} ${graph}_orig
    sed -e 's/\t/\n/g' ${graph} | sort -u -g > ${graph}_nodes
    nodecnt=$(wc -l ${graph}_nodes | cut -d' '  -f1)
    edgecnt=$(wc -l ${graph})

    #CREATE NODEID MAP
    ${RELEASE_PATH}/preprocess/dense_vertexranges -n ${nodecnt} -e ${edgecnt} -f ${graph}

    #construct sorted graph by merging edges files
    sort -g ${graph}_edges* > ${graph}_sorted
fi

if [ $insert -eq 1 ]
then

    ${RELEASE_PATH}/preprocess/mt_graphapi_insert -d ${type}_rd_${dataset} -e ${edgecnt} -n ${nodecnt} -f ${graph} -t ${type} -p ${output} -r -l ${RESULT} -m ${NUM_THREADS} >> ${RESULT}/${dataset}_${type}_mtapi_insert_log.txt

fi

if [ $index_create -eq 1 ]
then
    if [[ $type == "std" || $type == "ekey" ]]
    then
	    $TIME_CMD --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append  ${RELEASE_PATH}/preprocess/init_db -m ${type}_rd_${dataset}  -b PR -a ${output} -s ${dataset} -r -d -l ${type} -x &> ${RESULT}/insert_log.txt
	    $TIME_CMD --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append  ${RELEASE_PATH}/preprocess/init_db -m ${type}_d_${dataset}  -b PR -a ${output} -s ${dataset} -d -l ${type} -x &> ${RESULT}/insert_log.txt

    fi
fi

#fi
# echo -ne '\007'
# exit 1
#CIT: -n 3774768 -e 16518948
#Twitter: -n 41652230 -e 1468365182
