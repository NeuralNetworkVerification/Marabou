
import os
import sys
import argparser
import matplotlib.pyplot as plt
import numpy as np

resultFiles = list()
for root, dirs, files in os.walk(os.getcwd()):
    for f in files:
        if str(f) == "Results.out":
            fullpath = root + "/" + f
            resultsFiles.append(fullpath)
            print(fullpath)


parser = argparse.ArgumentParser(description='Query log files')
parser.add_argument("--batch", type=str, default="", help="Limit to  aspecifc batch")            
args = parser.parse_args()

perRunTypeResults_total = dict()
for fullpath in resultsFiles:
    if args.batch and args.batch not in fullpath:
        continue
    with open(fullpath, "r") as f:
        for line in f.readlines():
            lineClean = ''.join(line.split())
            if lineClean.startswith("RUNSUFFIX="):
                suffix = line.lstrip("RUNSUFFIX=").split("#")[0]
            elif lineClean.startswith("TOTAL"):
                total_runtime = line.split("=")[1].split("#")[0]
        if suffix not in perRunTypeResults_total:
            perRunTypeResults_total[suffix] = [total_runtime]
        else:
            perRunTypeResults_total[suffix].append(total_runtime)

plt.figure()
fig, axs = plt.subplots(len(perRunTypeResults_total.keys()))
for (i, suffix), vals in zip(enumerate(perRunTypeResults_total.keys()), perRunTypeResults_total.values()):
    axs[i].plot(vals, np.zeros_like(vals))
    axs[i].set_title(suffix)
plt.show()
            
                
    
