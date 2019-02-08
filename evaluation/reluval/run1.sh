#!/bin/bash

# define variables
# $1 = nnet file
# $2 = property file
# $3 = summary file
# $4 = additional flags
input_net=$1
summary=$3
flags=$4

function post {
        source /barrett/data/zeljic/py3.6/bin/activate
      	python3 $scriptdir/scripts/result_parser.py $results_file $summary
	deactivate
	exit
}


trap 'kill -QUIT $pid & wait $pid ; sleep 30 ; post' SIGTERM

# the working directory is where 'resources' directory is
# useful paths:
scriptdir="$(dirname "$0")"
tmpbasedir="resources/tmp"
resultdir="resources/results/solver_result"


tmpdir=$(mktemp -d -p $tmpbasedir)
nnet_file=$tmpdir/nnet
results_file="$tmpdir/results_$(basename "$summary")"

# give permissions to scripts
chmod 777 $scriptdir/network_test

cp -T $input_net $nnet_file
# run solver
$scriptdir/network_test 1 $nnet_file 0 0 0 > $results_file & pid=$!
# convert results
wait $pid
post

