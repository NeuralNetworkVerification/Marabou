##!/bin/bash

PYTHON36_PATH=/barrett/data/zeljic/py3.6/bin/activate
artifact=/barrett/data/zeljic/cav/artifact

results=$data/results
args=$artifact/arguments
exp=$artifact/experiments
sols=$artifact/solvers

source $PYTHON36_PATH
cp $sols/planet/planet_twin.zip Planet.zip
cp $sols/dncmarabou/dncmarabou.zip DnCMarabou.zip
cp $args/twinLadder.args args
python3 experiment_builder.py @args
mv TwinLadder $exp 
deactivate

rm args DnCMarabou.zip Planet.zip


#for arg in `ls $args/*.args`; do  
#    echo "Building $arg" 
#    python3 $scripts/experiment_builder.py @$arg
#done
