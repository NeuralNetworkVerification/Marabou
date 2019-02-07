#!/bin/bash
# define variables
input_net=$1
input_property=$2
summary=$3
flags=$4

function post {
      	python3 $scriptdir/scripts/post.py $results_file $summary
	exit
}
trap 'kill -TERM $pid & wait $pid ; sleep 30  ; post' SIGTERM

# the working directory is where 'resources' directory is
# useful paths:
scriptdir="$(dirname "$0")"
tmpbasedir="resources/tmp"
resultdir="resources/results/solver_result"

# $1 = nnet file
# $2 = property file
# $3 = summary file
# $4 = additional flags

tmpdir=$(mktemp -d -p $tmpbasedir)
property_file=$tmpdir/property
net_name=$tmpdir/nnet
results_file="$tmpdir/results_$(basename "$summary")"

echo $tmpdir

# convert
python3 $scriptdir/scripts/pre.py $input_net $input_property $scriptdir $net_name $property_file
# run solver
source /barrett/data/zeljic/py3/bin/activate
export PYTHONPATH=${PYTHONPATH}:/barrett/data/zeljic/cav/solvers/Marabou
python3 $scriptdir/DnC.py -n $net_name -q $property_file --summary-file=$results_file $flags & pid=$!
# convert results
wait $pid
post
deactivate
