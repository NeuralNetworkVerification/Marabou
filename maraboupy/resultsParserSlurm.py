from __future__ import division
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
            numRuns = len(resultDict["subResults"])
            finalPartiallity = dict()
            finalPartiallity["vars"] = finalQueryStats["numVars"] / originalQueryStats["numVars"]
            finalPartiallity["equations"] = finalQueryStats["numEquations"] / originalQueryStats["numEquations"]
            finalPartiallity["reluConstraints"] = finalQueryStats["numReluConstraints"] / originalQueryStats["numReluConstraints"]            
        else:
            originalQueryStats = dict()
            finalQueryStats = dict()
            totalRuntime = TIMEOUT_VAL
            numRuns = 0
            finalPartiallity = dict()
        sampleIndex = resultDict["cfg_sampleIndex"]
                        
        if runTitle not in perRunTypeResults_total:
            perRunTypeResults_total[runTitle] = dict()
        perRunTypeResults_total[runTitle][sampleIndex] = {"result" : resultDict["Result"].upper(),
                                                          "totalRuntime" : totalRuntime,
                                                          "originalQueryStats": originalQueryStats,
                                                          "finalQueryStats": finalQueryStats,
                                                          "finalPartiallity" : finalPartiallity}


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
                
plt.figure()

# Pie chart, where the slices will be ordered and plotted counter-clockwise:
labels = 'SAT', 'UNSAT', 'TIMEOUT'
print("vanillaDict.values()={}".format(len(vanillaDict.values())))
print("maskCOIDict.values()={}".format(len(maskCOIDict.values())))
sizesVanilla = [len([v for v in vanillaDict.values() if v["result"] == l]) for l in labels]
sizesMaskCOI = [len([v for v in maskCOIDict.values() if v["result"] == l]) for l in labels]
#explode = (0, 0.1, 0, 0)  # only "explode" the 2nd slice (i.e. 'Hogs')

fig, (ax1, ax2) = plt.subplots(nrows=1, ncols=2)
plt.subplots_adjust(wspace=1)
ax1.pie(sizesMaskCOI, labels=labels, colors=["red","green","yellow"], autopct=lambda p : "{:1.1f}%".format(p), shadow=True, startangle=90)
ax1.set_title("CNN abstraction")
ax2.pie(sizesVanilla, labels=labels, colors=["red","green","yellow"], autopct=lambda p : "{:1.1f}%".format(p), shadow=True, startangle=90)
ax2.set_title("Vanilla Marabou")

ax1.axis('equal')
ax2.axis('equal')

plt.savefig("resultPie.png")
