#!/bin/bash

# define variables
# $1 = nnet file
# $2 = property file
# $3 = summary file
# $4 = additional flags
input_net=$1
summary=$2

trap 'kill -QUIT $pid & wait $pid ; sleep 30 ; post' SIGTERM



# run solver
/export/stanford/barrettlab/andrew-cav-experiment_reluVal/reluVals/network_test2 2 $input_net 0 0 0 > $summary & pid=$!
# convert results
wait $pid
#post

