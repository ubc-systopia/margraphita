#!/bin/bash
set -e
set -x

usage() { 
echo "Usage: $0 [-f <dataset path> -o <output path> -m <dataset name> -n<edge count> -e<edge count> -t <type> -i"  
echo "Dataset path : the absolute path to the dataset that is being inserted."
echo "Output path: absolute path to the **directory** that should contain the output
files"
echo "Dataset name : used to construct the DB name and to name the output files"
echo "type is one of std, ekey, adj. Use all for all types of tables"
echo "i - if this flag is passed, indices are created."
echo "if cit-Patents is in ~/datasets/cit-Patents/cit-Patents.txt"
echo " -f ~/datasets/cit-Patents/cit-Patents.txt"
echo " -o ~/datasets/cit-Patents"
echo " -m cit-Patents"
echo " -t std"
exit 1;}

index_create=0

if [ -z "$*" ]; then echo "No args provided"; usage; fi
while getopts "f:o:m:e:n:t:i" o; do
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
        (t)
            type=${OPTARG}
            ;;
        (i)
            index_create=1
            ;;
        (*)
            usage
            ;;
    esac
done

##remove all lines that begin with a comment
sort --parallel=10 -k 1,2 ${filename} | parallel --pipe sed '/^#/d' > ${output}/${dataset}_sorted.txt
mv ${filename} ${filename}_orig
mv ${output}/${dataset}_sorted.txt ${filename} #overwrite the original file
# with the sorted, no comment version

# ##Split the edges file
split --number=l/10 ${filename} "${output}/${dataset}_edges"

#Create a nodes file
sed -e 's/\t/\n/g; s/\r//g' ${filename} | sort -u --parallel=10 > ${output}/${dataset}_nodes
split --number=l/10 ${output}/${dataset}_nodes ${output}/${dataset}_nodes

#Now create an empty DBs for all three representations for  insertion
#Using pagerank for this. Too lazy to do any better :(
# Here rd in the DB Name indicates the r(read optimize) and d(directed) flags 
if [[ $type == "std" || $type == "all" ]]
then 
 ./pagerank -n -m std_rd_${dataset} -b PR -a ${output} -s ${dataset} -o -r -d -l std -e
 ./pagerank -n -m std_d_${dataset} -b PR -a ${output} -s ${dataset} -o -d -l std -e
fi

if [[ $type == "adj" || $type == "all" ]]
then 
 ./pagerank -n -m adj_rd_${dataset} -b PR -a ${output} -s ${dataset} -r -o -d -l adj -e
 ./pagerank -n -m adj_d_${dataset} -b PR -a ${output} -s ${dataset} -o -d -l adj -e
fi

if [[ $type == "ekey" || $type == "all" ]]
then 
 ./pagerank -n -m ekey_rd_${dataset} -b PR -a ${output} -s ${dataset} -o -r -d -l ekey -e
 ./pagerank -n -m ekey_d_${dataset} -b PR -a ${output} -s ${dataset} -o -d -l ekey -e
fi

# # Here : n (new graph) m (name of db) b(benchmark-- useless in this context but
# # needed) s(dataset name *NOT* path) o(optimize create) r(read_optimize) d(directed)
# # e (exit on create -- special switch to do this. )

# #Now insert into the database
# # touch insert_time.txt
 echo "#Inserting ${dataset}" >> insert_time.txt
 echo "Command,Real time,User Timer,Sys Time,Major Page Faults,Max Resident Set" >> insert_time.txt
 echo "#${dataset}:" >> insert_time.txt
 echo "#${dataset}:" >> insert_log.txt
/usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./bulk_insert -d ${dataset} -e ${edgecnt} -n ${nodecnt} -f ${output}/${dataset} -t ${type} -p ${output} -r &>> insert_log.txt

/usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./bulk_insert -d ${dataset} -e ${edgecnt} -n ${nodecnt} -f ${output}/${dataset} -t ${type} -p ${output} &>> insert_log.txt

if [ $index_create -eq 1 ]
then

usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./pagerank -m std_rd_${dataset}  -b PR -a ${output} -s ${dataset} -r -d -l std -x &> insert_log.txt
usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./pagerank -m std_d_$./{dataset}  -b PR -a ${output} -s ${dataset} -d -l std -x &> insert_log.txt

usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./pagerank -m adj_rd_${dataset}  -b PR -a ${output} -s ${dataset} -r -d -l adj -x &> insert_log.txt
usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./pagerank -m adj_d_${dataset}  -b PR -a ${output} -s ${dataset} -d -l adj -x &> insert_log.txt

usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./pagerank -m ekey_rd_${dataset}  -b PR -a ${output} -s ${dataset} -r -d -l ekey -x &> insert_log.txt
usr/bin/time --format="%C,%e,%U,%S,%F,%M" --output-file=insert_time.txt --append ./pagerank -m ekey_d_${dataset}  -b PR -a ${output} -s ${dataset} -d -l ekey -x &> insert_log.txt

fi
# echo -ne '\007'
# exit 1
#CIT: -n 3774768 -e 16518948 
#Twitter: -n 41652230 -e 1468365182 
