#!/bin/bash
# define variables

input_net=$1
input_property=$2
summary=$3
flags=$4

echo $input_net
echo $flags

trap 'kill -TERM $pid & wait $pid ; sleep 30  ; post' SIGTERM


export PYTHONPATH=${PYTHONPATH}:/home/cav/marabou/
python3 /home/cav/marabou/maraboupy/DnC.py -n $input_net -q $input_property --summary-file=$summary $flags & pid=$!
wait $pid
