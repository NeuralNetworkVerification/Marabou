import subprocess
import sys
import os

solver =sys.argv[1]
benchmark_sets = sys.argv[2]



for benchmark_set in os.listdir(benchmark_sets):
    if "benchmark_set" not in benchmark_set or "marabou" in benchmark_set:
        continue
    print ("Solving benchmark_set: {}".format(benchmark_set))
    with open(benchmark_sets + benchmark_set, "r") as in_file:
        num_tasks = len(in_file.readlines())
    with open(benchmark_sets + benchmark_set, "r") as in_file:
        i = 1
        for line in in_file.readlines():
            arg1 = line.split()[0]
            arg2 = line.split()[1]
            arg3 = line.split()[2]
            arg4 = line.split()[3]
            name = arg4[:-4]
            f = open(name + ".txt", "w")
            e = open(name + ".err", "w")
            #print ("using solver {}".format(solver + arg1))
            subprocess.run(["runlim", solver + arg1, arg2, arg3, "0", "0", ">", arg4], stdout=f, stderr=e)
            print ("{} out of {} done!".format(i, num_tasks))
            i += 1
