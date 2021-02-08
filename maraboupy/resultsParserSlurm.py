
import os
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np
import json

TIMEOUT_VAL = 12 * 3600

resultsFiles = list()
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
        resultDict = json.load(f)
        if "cfg_runTitle" in resultDict:
            runTitle = resultDict["cfg_runTitle"].split("---")[0]
        else:
            runTitle = resultDict["cfg_runSuffix"].split("---")[0]
        if resultDict["Result"].upper() != "TIMEOUT":
            originalQueryStats = resultDict["subResults"][-1]["originalQueryStats"]
            finalQueryStats = resultDict["subResults"][-1]["finalQueryStats"]        
            totalRuntime = resultDict["totalRuntime"]
        else:
            originalQueryStats = dict()
            finalQueryStats = dict()
            totalRuntime = TIMEOUT_VAL
        sampleIndex = resultDict["cfg_sampleIndex"]
                        
        if runTitle not in perRunTypeResults_total:
            perRunTypeResults_total[runTitle] = dict()
        perRunTypeResults_total[runTitle][sampleIndex] = {"totalRuntime" : totalRuntime,
                                                          "originalQueryStats": originalQueryStats,
                                                          "finalQueryStats": finalQueryStats}


plt.figure()
LOGSCALE = True
if LOGSCALE:
    plt.xscale('log')
    plt.yscale('log')
assert len(perRunTypeResults_total.keys()) == 2
maskCOIDict = perRunTypeResults_total["MaskCOICfg"]
vanillaDict = perRunTypeResults_total["VanillaCfg"]
mutual = list(set(vanillaDict.keys()) & set(maskCOIDict.keys()))
#assert maskCOIDict.keys() == vanillaDict.keys()
x = [vanillaDict[sample]["totalRuntime"] for sample in mutual]
y = [maskCOIDict[sample]["totalRuntime"] for sample in mutual]
plt.scatter(x, y, marker='x')
plt.plot([1 if LOGSCALE else 0, TIMEOUT_VAL], [1 if LOGSCALE else 0, TIMEOUT_VAL], color='red')
plt.title("CNN abstraction vs. Vanilla Marabou")
plt.xlabel("Vanilla")
plt.ylabel("CNN abstraction")
plt.savefig("ComapreProperties.png")

#plt.figure()
#fig, axs = plt.subplots(len(perRunTypeResults_total.keys()))
#for (i, title), vals in zip(enumerate(perRunTypeResults_total.keys()), perRunTypeResults_total.values()):
#    axs[i].plot(vals, np.zeros_like(vals))
#    axs[i].set_title(title)
#plt.show()
            
                
    
