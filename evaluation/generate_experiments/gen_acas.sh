#!/bin/bash

PYTHON36_PATH=/barrett/data/zeljic/py3.6/bin/activate
artifact=/barrett/data/zeljic/cav/artifact


scripts=$artifact/scripts
results=$data/results

nnets=$artifact/networks/acas/nnet
props=$artifact/properties
args=$artifact/arguments
exp=$artifact/experiments
sols=$artifact/solvers
pysc=$artifact/python_scripts

source $PYTHON36_PATH
cp $sols/marabou/marabou.zip Marabou.zip
cp $sols/planet/planet_acas.zip Planet.zip

for i in 1 2 3 4; do
    cp $sols/reluplex/reluplex_p$i.zip Reluplex.zip
    cp $args/acas_p${i}.args args
    python3 experiment_builder.py @args
    mv acas_p${i} $exp 
done
deactivate

rm Marabou.zip Reluplex.zip Planet.zip


#for arg in `ls $args/*.args`; do  
#    echo "Building $arg" 
#    python3 $scripts/experiment_builder.py @$arg
#done
