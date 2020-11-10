#!/bin/bash
#SBATCH --job-name=cnn_abs
#SBATCH --cpus-per-task=8
#SBATCH --mem=4g
#SBATCH --output=./cnn_abs.sbatch.out
#SBATCH --time=1:00:00
#SBATCH --signal=B:SIGUSR1@300

python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/cnn_abs.py > ./cnn_abs.out
