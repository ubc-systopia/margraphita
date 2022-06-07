#!/bin/bash
set -x

# This file is a very simple way to track the changes in benchmark performance with each changeset.
# I want to use this to track which optimization is actually beneficial to us.

source paths.sh
#check if this file exists and assert that it matches $HOME
if test -f "paths.sh"; then
    if grep -Fq "$HOME" paths.sh; then
        source paths.sh
    else
        echo "Please update paths.sh to reflect your settings."
    fi
else
    echo "paths.sh not found."
fi

# exit 0

#commit all changed files in the working directory. This should open vim for commit message
# git add -u
# git commit -a

 echo "---------NOW MAKING A RELEASE BUILD------------"
 mkdir ${RELEASE_PATH}
 cd ${RELEASE_PATH}
 cmake ../.. && make -j16

# echo "---------NOW MAKING A PROFILE BUILD------------"

# mkdir ${PROFILE_PATH}
# cd ${PROFILE_PATH}
# cmake -DCMAKE_BUILD_TYPE=DEBUG ../.. && make -j16

# echo "---------NOW MAKING A STATS BUILD------------"

# mkdir ${STATS_PATH}
# cd ${STATS_PATH}
# cmake -DCMAKE_BUILD_TYPE=DEBUG -Dstat=true ../.. && make -j16

# #Now create the log directory
#commit_id=`git rev-parse --short HEAD`

# #create directory where the scripts will insert logs
dir="${HOME}/margraphita/outputs/${commit_id}"
# mkdir -p $dir

# #pass this dir to bulk_insert
# #/bin/bash insert_kron.sh -l ${dir}

# #now pass this commit_id to run_benchmarks.sh
#/bin/bash run_benchmarks.sh -d ${dir}
