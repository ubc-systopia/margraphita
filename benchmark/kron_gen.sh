#!/bin/bash
set -x

git clone git@github.com:dmargo/smooth_kron_gen.git
cd smooth_kron_gen
make

for ((scale=10; scale <=20; scale ++ )); do
	myvar=$(bc <<<"8*2^$scale")
	filename="graph_s${scale}_e8"
	dirname="s${scale}_e8"
	mkdir -p $dirname

	echo " Now creating scale $scale, saved in $dirname/$filename"
	./kron_generator $myvar $scale 0 0 > $dirname/$filename
done
