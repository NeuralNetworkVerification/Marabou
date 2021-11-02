#!/bin/bash
#SBATCH --job-name=DumpAll
#SBATCH --cpus-per-task=1
#SBATCH --mem-per-cpu=8G
#SBATCH --output=/cs/labs/guykatz/matanos/Marabou/maraboupy/evaluation/SbatchDumpAllBoundsC.out
#SBATCH --partition=long
#SBATCH --signal=B:SIGUSR1@300
#SBATCH --time=1:00:00
#SBATCH -C avx2

pwd; hostname; date

csh /cs/labs/guykatz/matanos/Marabou/../py_env/bin/activate.csh
export PYTHONPATH=$PYTHONPATH:/cs/labs/guykatz/matanos/Marabou/maraboupy:/cs/labs/guykatz/matanos/Marabou
export GUROBI_HOME=/cs/labs/guykatz/matanos/gurobi911/linux64
export PATH=$PATH:${GUROBI_HOME}/bin
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib
export GRB_LICENSE_FILE=/cs/share/etc/license/gurobi/gurobi.lic


echo "Ive been launched" > /cs/labs/guykatz/matanos/Marabou/maraboupy/evaluation/StartedDumpAllBoundsC
stdbuf -o0 -e0 python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/evaluation/dumpAllBounds.py --net networkC

date
