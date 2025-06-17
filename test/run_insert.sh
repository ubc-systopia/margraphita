#!/bin/bash

TEST="/home/puneet/scratch/margraphita/build/testing"

#Build project with UseProps=True and OrderNodes=True
cd $TEST
cmake -DCMAKE_BUILD_TYPE=Release -DB64=True -DUseProps=True -DOrderNodes=True ../..
make -j
cd test

./test_adj_list 1 1 | tee DotaLeague_withNodes_OrderNodes_SortEdges_ReadOpt.txt

./test_adj_list 1 0 | tee DotaLeague_withNodes_OrderNodes_SortEdges_NoReadOpt.txt

./test_adj_list 0 1 | tee DotaLeague_withNodes_OrderNodes_NoSortEdges_ReadOpt.txt

./test_adj_list 0 0 | tee DotaLeague_withNodes_OrderNodes_NoSortEdges_NoReadOpt.txt

cd $TEST
cmake -DCMAKE_BUILD_TYPE=Release -DB64=True -DUseProps=True -DOrderNodes=False ../..
make -j
cd test

./test_adj_list 1 1 | tee DotaLeague_withNodes_NoOrderNodes_SortEdges_ReadOpt.txt

./test_adj_list 1 0 | tee DotaLeague_withNodes_NoOrderNodes_SortEdges_NoReadOpt.txt

./test_adj_list 0 1 | tee DotaLeague_withNodes_NoOrderNodes_NoSortEdges_ReadOpt.txt

./test_adj_list 0 0 | tee DotaLeague_withNodes_NoOrderNodes_NoSortEdges_NoReadOpt.txt

cd $TEST
cmake -DCMAKE_BUILD_TYPE=Release -DB64=True -DUseProps=False -DOrderNodes=True ../..
make -j
cd test

./test_adj_list 1 1 | tee DotaLeague_withoutNodes_OrderNodes_SortEdges_ReadOpt.txt

./test_adj_list 1 0 | tee DotaLeague_withoutNodes_OrderNodes_SortEdges_NoReadOpt.txt

./test_adj_list 0 1 | tee DotaLeague_withoutNodes_OrderNodes_NoSortEdges_ReadOpt.txt

./test_adj_list 0 0 | tee DotaLeague_withoutNodes_OrderNodes_NoSortEdges_NoReadOpt.txt

cd $TEST
cmake -DCMAKE_BUILD_TYPE=Release -DB64=True -DUseProps=False -DOrderNodes=False ../..
make -j
cd test

./test_adj_list 1 1 | tee DotaLeague_withoutNodes_OrderNodes_SortEdges_ReadOpt.txt

./test_adj_list 1 0 | tee DotaLeague_withoutNodes_OrderNodes_SortEdges_NoReadOpt.txt

./test_adj_list 0 1 | tee DotaLeague_withNodes_OrderNodes_NoSortEdges_ReadOpt.txt

./test_adj_list 0 0 | tee DotaLeague_withNodes_OrderNodes_NoSortEdges_NoReadOpt.txt