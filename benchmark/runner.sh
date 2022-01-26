#!/bin/bash
#set -x

# This file is a very simple way to track the changes in benchmark performance with each changeset.
# I want to use this to track which optimization is actually beneficial to us. 

#commit all changed files in the working directory. This should open vim for commit message
git add -u
git commit -a


#Now create the log directory
commit_id=`git rev-parse HEAD`

#create directory where the scripts will insert logs
dir="${HOME}/scratch/margraphita/outputs/${commit_id}"
mkdir -p $dir

#now pass this commit_id to run_benchmarks.sh
/bin/bash run_benchmarks.sh #-d $dir
