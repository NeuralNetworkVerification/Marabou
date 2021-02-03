
import os
import sys
import argparser
import matplotlib.pyplot as plt
import numpy as np
import json

resultFiles = list()
for root, dirs, files in os.walk(os.getcwd()):
    for f in files:
        if str(f) == "Results.json":
            fullpath = root + "/" + f
            resultsFiles.append(fullpath)
            print(fullpath)


parser = argparse.ArgumentParser(description='Query log files')
parser.add_argument("--batch", type=str, default="", help="Limit to a specifc batch")            
args = parser.parse_args()

perRunTypeResults_total = dict()
for fullpath in resultsFiles:
    if args.batch and args.batch not in fullpath:
        continue
    with open(fullpath, "r") as f:
        resultDict = json.load(fullpath)
        runSuffix = resultDict["runSuffix"]
        totalRuntime = resultDict["totalRuntime"]
                        
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
            
                
    
