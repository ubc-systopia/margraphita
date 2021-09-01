#!/bin/bash
set -x

for ((scale=10; scale <=27; scale ++ )); do
	n_edges=$(bc <<<"8*2^$scale")
    n_nodes=$(bc <<<"2^$scale")
	
    filename="graph_s${scale}_e8"
	dirname="/ssd_graph/s${scale}_e8"

    date >> kron_insert.txt
	echo " Now inserting scale $scale, saved in $dirname/$filename" >> kron_insert.txt
    ./preprocess.sh -f "${dirname}/${filename}" -o "${dirname}" -m "s${scale}_e8" -n $n_nodes -e $n_edges &> kron_insert.txt
    
    echo "---------------------------------------------" >> kron_insert.txt
	
done
