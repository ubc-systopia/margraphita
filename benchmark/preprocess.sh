#!/bin/bash
set -e

usage() { 
echo "Usage: $0 [-f <dataset path> -o <output path> -m <dataset name> -n<edge count> -e<edge count>"  
echo "Dataset path : the absolute path to the dataset that is being inserted."
echo "Output path: absolute path to the **directory** that should contain the output
files"
echo "Dataset name : used to construct the DB name and to name the output files"
echo "if cit-Patents is in ~/datasets/cit-Patents/cit-Patents.txt"
echo " -f ~/datasets/cit-Patents/cit-Patents.txt"
echo " -o ~/datasets/cit-Patents"
echo " -m cit-Patents"
exit 1;}

if [ -z "$*" ]; then echo "No args provided"; usage; fi
while getopts "f:o:m:n:e:" o; do
    case "${o}" in
        (f)
            filename=${OPTARG%/}
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
        (*)
            usage
            ;;
    esac
done

##remove all lines that begin with a comment
sed -i '/^#/d' ${filename}

##Split the edges file
split --number=l/10 ${filename} "${output}/${dataset}_edges"

##Process out degrees: count of unique entries in 1st column is the outdegree
#for each node.
cut -f1 ${filename} | sort | uniq -c > "temp_nodes_outdeg.txt"
#To remove leading whitespace
sed 's/^ *//' < "temp_nodes_outdeg.txt" > "${output}/${dataset}_nodes_outdeg_"
#split into 10 parts for MT ingest
#split --number=l/10 "${output}/${dataset}_nodes_outdeg_" "${output}/${dataset}_nodes_outdeg_"

##Process in degrees: count of unique entries in 2nd column is the indegree for
#each node
cut -f2 ${filename} | sort | uniq -c > "temp_nodes_indeg.txt"
#To remove leading whitespace
sed 's/^ *//' < "temp_nodes_indeg.txt" > "${output}/${dataset}_nodes_indeg_"

#Now create an empty DBs for all three representations for  insertion
#Using pagerank for this. Too lazy to do any better :(
# Here rd in the DB Name indicates the r(read optimize) and d(directed) flags 
./pagerank -n -m std_rd_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l std -e
./pagerank -n -m std_d_${dataset} -b PR -a ${filename} -s ${dataset} -o -d -l std -e

./pagerank -n -m adj_rd_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l adjlist -e
./pagerank -n -m adj_d_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l adjlist -e

./pagerank -n -m ekey_rd_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l edgekey -e
./pagerank -n -m ekey_d_${dataset} -b PR -a ${filename} -s ${dataset} -o -r -d -l edgekey -e

# Here : n (new graph) m (name of db) b(benchmark-- useless in this context but
# needed) s(dataset name *NOT* path) o(optimize create) r(read_optimize) d(directed)
# e (exit on create -- special switch to do this. )

#Now insert into the database
touch insert_time.txt
echo "#Inserting ${dataset}" >> insert_time.txt
echo "Command,Real time,User Timer,Sys Time,Major Page Faults,Max Resident Set" >> insert_time.txt
echo "#${dataset}:" >> insert_time.txt
echo "#${dataset}:" >> insert_log.txt
/usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./bulk_insert -d ${dataset} -e ${edgecnt} -n ${nodecnt} -f ${output}/${dataset} -r &> insert_log.txt

/usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./bulk_insert -d ${dataset} -e ${edgecnt} -n ${nodecnt} -f ${output}/${dataset} &>> insert_log.txt
echo "----------------------------" >> insert_time.txt