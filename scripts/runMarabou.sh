#!/bin/bash

marabou=$1
type=$2
network=$3
property=$4
summary=$5
config=$6
numArgs=$#

args=""
for (( i=7; i<=$#; i++ ))
do
    args+=" ${!i}"
done

echo $args


# change these
source /barrett/scratch/haozewu/py3.5/bin/activate
root="/barrett/scratch/haozewu/Targeted_attack/experiments/"

if [ "$type" == "acas" ]; then
    $marabou $network $property --summary-file=$summary $args
elif [ "$type" == "mnist" ]; then
    $marabou $network $property --summary-file=$summary $args
elif [ "$type" == "boeing" ]; then
    python3 "$root"scripts/runMarabouFromPyAPI.py -n $network -p $property -c $config -s $summary $args
elif [ "$type" == "kyle" ]; then
    python3 "$root"scripts/runMarabouFromPyAPI.py -n $network -p $property -c $config -s $summary $args
else
    echo "Unknown benchmark!"
fi

deactivate
