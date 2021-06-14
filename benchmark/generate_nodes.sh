#!/bin/bash

usage() { echo "Usage: $0 [-f <dataset path> -o <output base name>" 1>&2; exit 1; }

while getopts "f:o:" o; do
    case "${o}" in
        f)
            filename=${OPTARG}
            ;;
        o)
            output=${OPTARG}
            ;;
        *)
            usage
            ;;
    esac
done

##Split the edges file
split -n10 ${filename} "${output}_edges"

##Process out degrees: count of unique entries in 1st column is the outdegree
#for each node.
cut -f1 ${filename} | sort | uniq -c > "temp_nodes_outdeg.txt"
#To remove leading whitespace
sed 's/^ *//' < "temp_nodes_outdeg.txt" > "${output}_nodes_outdeg_"
#split into 10 parts for MT ingest
split -n10 "${output}_nodes_outdeg_" "${output}_nodes_outdeg_"

##Process in degrees: count of unique entries in 2nd column is the indegree for
#each node
cut -f2 ${filename} | sort | uniq -c > "temp_nodes_indeg.txt"
#To remove leading whitespace
sed 's/^ *//' < "temp_nodes_indeg.txt" > "${output}_nodes_indeg_"
#split into 10 parts for MT ingest
split -n10 "${output}_nodes_indeg_" "${output}_nodes_indeg_"