import os
import sys
import matplotlib.pyplot as plt
import numpy as np
from scipy.stats import gaussian_kde
import re
import argparse

# input line is "[filename] [epsilon] [result] [time]"

parser = argparse.ArgumentParser()
parser.add_argument('-r', "--run", type=str, default=None, help='runname')
parser.add_argument("--timeout", type=int, default=1200, help='the timeout')
parser.add_argument("--combined-filename", type=str, default=None, help="if not none, dump the combined summary file")
parser.add_argument("-p", '--plot', action='store_true', help="make plots")
parser.add_argument("--benchmark", type=str, default='all', help='benchmark sets to consider. (acas/mnist/boeing)')
parser.add_argument("--preprocessing", action='store_true', help="preprocessing")
parser.add_argument('-n', "--num-instances", type=int, default=None, help="Number of instances")
parser.add_argument("-t", "--threshold", type=int, default=30)
args = parser.parse_args()

benchmark = args.benchmark.split(',')
run = args.run
timeout = args.timeout
plot = args.plot
preprocessing = args.preprocessing
threshold = args.threshold

summaries = []
runs = []
for filename in os.listdir(run):
    if "benchmark_set_" in filename:
        print(filename)
        num_instances = 0
        temp_summary = {}
        runs.append(filename[14:-3])
        with open(os.path.join(run, filename), 'r') as benchmark_file:
            for line in benchmark_file.readlines():
                num_instances += 1
                summary_filename = line.split()[4]
                skip = False
                if benchmark[0] != 'all':
                    for b in benchmark:
                        if b not in summary_filename:
                            skip = True
                if skip:
                    continue
                if os.path.isfile(summary_filename):
                    with open(summary_filename, 'r') as in_file:
                        for line in in_file.readlines():
                            if line.split()[0] == "UNKNOWN" or float(line.split()[1]) >= timeout:
                                continue
                            temp_summary[os.path.basename(summary_filename).split('.')[0]] = (line.split()[0], int(float(line.split()[1])))
        summaries.append(temp_summary)                                        

if args.num_instances != None:
    num_instances = args.num_instances
summary = {}
for temp_summary in summaries:
    for queryname in temp_summary:
        if int(temp_summary[queryname][1]) < int(timeout):
            summary[queryname] = []

for queryname in summary:
    resultAdded = False
    for temp_summary in summaries:
        if queryname in temp_summary:
            if not resultAdded:
                summary[queryname] = [temp_summary[queryname][0]] + summary[queryname]
                resultAdded = True
            summary[queryname].append(temp_summary[queryname][1])
        else:
            summary[queryname].append(timeout)
    assert(resultAdded)
    if queryname in summaries[0] and queryname in summaries[1]:
        if (summaries[0][queryname][0] != summaries[1][queryname][0]):
            print(queryname, summaries[0][queryname][0], summaries[1][queryname][0])
        assert(summaries[0][queryname][0] == summaries[1][queryname][0])

SAT_times = []
UNSAT_times = []
easy_SATs = set()
easy_UNSATs = set()
for i in range(len(summaries)):
    SAT_times.append([])
    UNSAT_times.append([])

for queryname in summary:
    result = summary[queryname][0]
    startIndex = 0
    for i, time in enumerate(summary[queryname][1:]):
        assert( time <= timeout )
        if result == "SAT":
            SAT_times[i].append(time)
            if time < threshold and i >= startIndex:
                easy_SATs.add(queryname)
        elif result == "UNSAT":
            UNSAT_times[i].append(time)
            if time < threshold and i >= startIndex:
                easy_UNSATs.add(queryname)

filenames = []
SATs = []
UNSATs = []
less_than_10_seconds_SAT = []
less_than_10_seconds_UNSAT = []
total_times = []


print("Number of runs:{}".format(len(runs)))
for filename in runs:
    filenames.append(os.path.basename(filename[:-1]))

for i in range(len(summaries)):
    SAT_time = SAT_times[i]
    UNSAT_time = UNSAT_times[i]
    SATs.append(len([t for t in SAT_time if t < timeout]))
    UNSATs.append(len([t for t in UNSAT_time if t < timeout]))
    less_than_10_seconds_SAT.append(len([t for t in SAT_time if t < threshold]))
    less_than_10_seconds_UNSAT.append(len([t for t in UNSAT_time if t < threshold]))
    total_times.append(sum([x for x in (UNSAT_time + SAT_time) if x < timeout]) + timeout * (num_instances - SATs[-1] - UNSATs[-1]))
    assert((sum([x for x in (UNSAT_time + SAT_time) if x < timeout]) + timeout * (num_instances - SATs[-1] - UNSATs[-1])) ==
           sum(UNSAT_time + SAT_time) + timeout * (num_instances - len(SAT_time) - len(UNSAT_time)))

def tabSep(lst, delim='\t'):
    return delim.join(list(map(str, lst)))

print("\t\t\t{}".format(tabSep(filenames, '\t|')))
print("SAT:\t\t\t{}".format(tabSep(SATs)))
print("UNSAT:\t\t\t{}".format(tabSep(UNSATs)))
print("Solved({}):\t\t{}".format(num_instances, tabSep((np.array(SATs) + np.array(UNSATs)).tolist())))
print("Total SAT time:\t\t{}".format(tabSep([sum(lst) for lst in SAT_times])))
print("Total UNSAT time:\t{}".format(tabSep([sum(lst) for lst in UNSAT_times])))
print("Total time:\t\t{}".format(tabSep(total_times)))

print("<={}s (SAT):\t\t{}".format(threshold, tabSep(less_than_10_seconds_SAT)))
print("<={}s (UNSAT):\t\t{}".format(threshold, tabSep(less_than_10_seconds_UNSAT)))
print("Total <={} (SAT):\t{}".format(threshold, len(easy_SATs)))
print("Total <={} (UNSAT):\t{}".format(threshold, len(easy_UNSATs)))


print("\nSitmulations")

print("Simulation 1: Parallel Portfolio")
startIndex = 1
parallelSAT = [min(summary[queryname][startIndex:]) for queryname in summary if summary[queryname][0] == "SAT"]
parallelUNSAT = [min(summary[queryname][startIndex:]) for queryname in summary if summary[queryname][0] == "UNSAT"]
print("SAT: {}".format(len([t for t in parallelSAT if t < timeout])))
print("UNSAT: {}".format(len([t for t in parallelUNSAT if t < timeout])))
print("Solved({}): {}".format(num_instances, len([t for t in parallelSAT + parallelUNSAT if t < timeout])))
print("Total SAT time: {}".format(sum(parallelSAT)))
print("Total UNSAT time: {}".format(sum(parallelUNSAT)))
print("Total time: {}".format(sum(parallelSAT + parallelUNSAT) + timeout * (num_instances - len(parallelSAT + parallelUNSAT))))

if not plot:
    exit()

SAT1_time = SAT_times[0]
SAT2_time = SAT_times[1]
UNSAT1_time = UNSAT_times[0]
UNSAT2_time = UNSAT_times[1]
filename1 = filenames[0]
filename2 = filenames[1]

x=np.linspace(0,timeout,timeout+1)
plt.plot(x,x)
plt.scatter(SAT1_time, SAT2_time)
plt.xlim(0,timeout)
plt.ylim(0,timeout)
plt.xlabel(filename1)
plt.ylabel(filename2)
plt.gca().set_aspect('equal', adjustable='box')
plt.savefig(filename1 + "_" + filename2 + "_{}_SAT.png".format(benchmark[0]))
plt.clf()

x=np.linspace(0,timeout,timeout+1)
plt.plot(x,x)
plt.scatter(UNSAT1_time, UNSAT2_time)
plt.xlim(0,timeout)
plt.ylim(0,timeout)
plt.xlabel(filename1)
plt.ylabel(filename2)
plt.gca().set_aspect('equal', adjustable='box')
plt.savefig(filename1 + "_" + filename2 + "_{}_UNSAT.png".format(benchmark[0]))
plt.clf()

x=np.linspace(0,timeout,timeout+1)
plt.plot(x,x)
plt.scatter(SAT1_time + UNSAT1_time, SAT2_time + UNSAT2_time)
plt.xlim(0,timeout)
plt.ylim(0,timeout)
plt.xlabel(filename1)
plt.ylabel(filename2)
plt.gca().set_aspect('equal', adjustable='box')
plt.savefig(filename1 + "_" + filename2 + "_{}_ALL.png".format(benchmark[0]))
