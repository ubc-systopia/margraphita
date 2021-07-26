#!/bin/bash
set -x
set -e
usage() { echo "Usage: $0 [-f <dataset path> -o <output path> -n <dataset name>" 1>&2; exit 1; }

while getopts "f:o:n:" o; do
    case "${o}" in
        (f)
            filename=${OPTARG}
            ;;
        (o)
            output=${OPTARG}
            ;;
        (n)
            dataset=${OPTARG}
            ;;
        (*)
            usage
            ;;
    esac
done

##remove all lines that begin with a comment
sed -i '/^#/d' ${filename}

##Split the edges file
split --number=l/10 ${filename} "${output}_edges"

##Process out degrees: count of unique entries in 1st column is the outdegree
#for each node.
cut -f1 ${filename} | sort | uniq -c > "temp_nodes_outdeg.txt"
#To remove leading whitespace
sed 's/^ *//' < "temp_nodes_outdeg.txt" > "${output}_nodes_outdeg_"
#split into 10 parts for MT ingest
#split --number=l/10 "${output}_nodes_outdeg_" "${output}_nodes_outdeg_"

##Process in degrees: count of unique entries in 2nd column is the indegree for
#each node
cut -f2 ${filename} | sort | uniq -c > "temp_nodes_indeg.txt"
#To remove leading whitespace
sed 's/^ *//' < "temp_nodes_indeg.txt" > "${output}_nodes_indeg_"
#split into 10 parts for MT ingest
#split --number=l/10 "${output}_nodes_indeg_" "${output}_nodes_indeg_"

#Now create an empty DB to initialize insert
#using pagerank for this. Too lazy to do any better :(
./pagerank -n -m std_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l std -e

./pagerank -n -m adj_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l adjlist -e

./pagerank -n -m ekey_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l edgekey -e