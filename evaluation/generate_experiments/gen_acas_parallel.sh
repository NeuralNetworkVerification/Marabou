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
cp $sols/dncmarabou/dncmarabou.zip DnCMarabou.zip

for i in 1 2 3 4; do
    cp $sols/reluval/reluval_4t_p$i.zip ReluVal.zip
    cp $args/acas_p${i}_4.args args
    python3 experiment_builder.py @args
    mv acas_p${i} $exp/acas_p${i}_4
done
deactivate

rm DnCMarabou.zip ReluVal.zip 


#for arg in `ls $args/*.args`; do  
#    echo "Building $arg" 
#    python3 $scripts/experiment_builder.py @$arg
#done
