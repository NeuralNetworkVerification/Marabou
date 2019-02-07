#!/bin/bash

PYTHON36_PATH=/barrett/data/zeljic/py3.6/bin/activate
artifact=/barrett/data/zeljic/cav/artifact

results=$data/results
args=$artifact/arguments
exp=$artifact/experiments
sols=$artifact/solvers

source $PYTHON36_PATH
cp $sols/planet/planet_coav.zip Planet.zip
cp $sols/dncmarabou/dncmarabou.zip DnCMarabou.zip
cp $args/collisionDetection.args args
python3 experiment_builder.py @args
mv CollisionDetection $exp 
deactivate

rm args DnCMarabou.zip Planet.zip

