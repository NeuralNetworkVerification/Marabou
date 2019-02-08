import subprocess
import sys
import os

solver =sys.argv[1]
benchmark_sets = sys.argv[2]

for benchmark_set in os.listdir(benchmark_sets):
    if "benchmark_set" not in benchmark_set or "reluval" in benchmark_set:
        continue
    print ("Solving benchmark_set: {}".format(benchmark_set))
    with open(benchmark_sets + benchmark_set, "r") as in_file:
        for line in in_file.readlines():
            arg1 = line.split()[0]
            arg2 = line.split()[1]
            arg3 = line.split()[2]
            arg4 = line.split('"')[1]
            name = line.split()[2][:-4]
            f = open(name + ".log", "w")
            e = open(name + ".err", "w")
            subprocess.run(["runlim", solver, arg1, arg2, arg3, arg4], stdout=f, stderr=e)
