#!/bin/bash
set -x
source ../../paths.sh

if [ -z "$KRON_GRAPHS_PATH"] || [-z "$KRON_GEN"] || [-z $KRON_GEN_PATH ]
then
    echo "Please set paths in BUILD_DIR/benchmarks/paths.sh"
    exit -1
fi

for ((scale=10; scale <=15; scale ++ )); do
	nEdges=$(bc <<<"8*2^$scale")
	nVertices=$(bc <<<"2^$scale")

	filename="graph_s${scale}_e8"
	dirname="${KRON_GRAPHS_PATH}/s${scale}_e8"
	mkdir -p $dirname

	echo " Now creating scale $scale, saved in $dirname/$filename"

	if [[ $KRON_GEN == 'smooth_kron' ]]
	then
		${KRON_GEN_PATH} $nEdges $scale 0 0 > $dirname/$filename
	elif [[ $KRON_GEN == 'parmat' ]]
	then
		${KRON_GEN_PATH} -nEdges $nEdges -nVertices $nVertices -noEdgeToSeld -noDuplicateEdges -output $dirname/$filename
	fi
done
