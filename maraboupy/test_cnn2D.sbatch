#!/bin/bash
#SBATCH --job-name=tests_cnn2D
#SBATCH --cpus-per-task=8
#SBATCH --output=./test_cnn2D.sbatch.out
#SBATCH --partition=long
#SBATCH --time=12:00:00
#SBATCH --signal=B:SIGUSR1@300

python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/examples/test_cnn2D.py /cs/labs/guykatz/matanos/Marabou/maraboupy/examples > ./test_cnn2D.out
