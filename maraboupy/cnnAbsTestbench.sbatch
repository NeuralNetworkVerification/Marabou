#!/bin/bash
#SBATCH --job-name=cnnAbsTB
#SBATCH --cpus-per-task=8
#SBATCH --mem-per-cpu=1G
#SBATCH --output=/cs/labs/guykatz/matanos/Marabou/maraboupy/cnnAbsTB.out
#SBATCH --partition=long
#SBATCH --signal=B:SIGUSR1@300
#SBATCH --time=36:00:00

pwd; hostname; date

source /cs/labs/guykatz/matanos/py_env/bin/activate.csh
export PYTHONPATH=$PYTHONPATH:/cs/labs/guykatz/matanos/Marabou
export GUROBI_HOME=/cs/labs/guykatz/matanos/gurobi900/linux64
export PATH= $PATH:${GUROBI_HOME}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib
export GRB_LICENSE_FILE=/cs/share/etc/license/gurobi/gurobi.lic

stdbuf -o0 -e0 python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/cnn_abs_testbench.py

date
