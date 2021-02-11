from __future__ import division
import os
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np
import json
import pandas as pd
from matplotlib.ticker import MaxNLocator

TIMEOUT_VAL = 12 * 3600

def pctFunc(pct, data):
    absolute = int(pct/100.*np.sum(data))
    return "{:1.1f}%\n({})".format(pct, absolute) if pct > 0 else ""

def countSame(x,y):
    xy = list(zip(x,y))
    xyCount = [xy.count(el) for el in xy]    
    return list(zip(xyCount, xy))

def cellColor(result):
    if result.upper() == 'TIMEOUT':
        return 'yellow'
    elif result.upper() == 'UNSAT':
        return 'green'
    elif result.upper() == 'SAT':
        return 'red'
    return None

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
            assert len(originalQueryStats) == len(finalQueryStats)
            totalRuntime = resultDict["totalRuntime"]
            numRuns = len(resultDict["subResults"])
            finalPartiallity = dict()
            finalPartiallity["vars"] = finalQueryStats["numVars"] / originalQueryStats["numVars"]
            finalPartiallity["equations"] = finalQueryStats["numEquations"] / originalQueryStats["numEquations"]
            finalPartiallity["reluConstraints"] = finalQueryStats["numReluConstraints"] / originalQueryStats["numReluConstraints"]
            finalPartiallity["numRuns"] = len(originalQueryStats)
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
xyCountSame = countSame(x,y)
plt.scatter(x, y, s=70, alpha=0.3)
for count, coor in countSame(x,y):
    plt.annotate(count, coor)
bottom = 1 if LOGSCALE else 0
top = TIMEOUT_VAL
plt.plot([bottom, top], [bottom, top], 'r:')
plt.plot([bottom, top], [top   , top], 'r:')
plt.plot([top,    top], [bottom, top], 'r:')
plt.title("CNN abstraction vs. Vanilla Marabou, {} samples".format(len(mutual)))
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
fig.suptitle("Run Results", fontsize=20)
plt.subplots_adjust(wspace=1)
ax1.pie(sizesMaskCOI, labels=[l if s > 0 else "" for l,s in zip(labels, sizesMaskCOI)], colors=["darkred","darkgreen","gold"], autopct=lambda pct: pctFunc(pct, sizesMaskCOI), shadow=True, startangle=90, textprops=dict(color="k"))
ax1.set_title("CNN abstraction, {} samples".format(len(maskCOIDict)), fontdict=dict(fontsize=12))
ax2.pie(sizesVanilla, labels=[l if s > 0 else "" for l,s in zip(labels, sizesVanilla)], colors=["darkred","darkgreen","gold"], autopct=lambda pct: pctFunc(pct, sizesVanilla), shadow=True, startangle=90, textprops=dict(color="k"))
ax2.set_title("Vanilla Marabou, {} samples".format(len(vanillaDict)), fontdict=dict(fontsize=12))

ax1.axis('equal')
ax2.axis('equal')

plt.savefig("ResultPie.png")

plt.figure()
fig, (ax1, ax2) = plt.subplots(nrows=2, ncols=1)
#ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
#ax2.xaxis.set_major_locator(MaxNLocator(integer=True))
solved = [k for k,v in maskCOIDict.items() if v["result"] in ["UNSAT", "SAT"]]
x  = [maskCOIDict[sample]["finalPartiallity"]["numRuns"]   for sample in solved]
y1 = [maskCOIDict[sample]["finalPartiallity"]["vars"]      for sample in solved]
y2 = [maskCOIDict[sample]["finalPartiallity"]["equations"] for sample in solved]
c  = ["darkred" if maskCOIDict[sample]["result"] == "SAT" else "darkgreen" for sample in solved]
fig.suptitle("CNN abstraction - Variables and Equations Ratio, {} Samples".format(len(solved)))
ax2.set_xlabel("Number of Runs")
ax1.set_ylabel("Eventuall Variables Ratio")
ax2.set_ylabel("Eventuall Equation Ratio")
ax1.scatter(x, y1, s=70, alpha=0.3, c=c)
ax2.scatter(x, y2, s=70, alpha=0.3, c=c)

for count, coor in countSame(x,y1):
    ax1.annotate(count, coor)
for count, coor in countSame(x,y2):
    ax2.annotate(count, coor)    
plt.savefig("COIRatio.png")

plt.figure()
fig, ax = plt.subplots(nrows=1, ncols=1)
fig.patch.set_visible(False)
ax.axis('off')
ax.axis('tight')

samplesTotal = list(set(vanillaDict.keys()) | set(maskCOIDict.keys()))
maskCOITimes  = [maskCOIDict[s]["totalRuntime"] if s in maskCOIDict else -1 for s in samplesTotal]
vanillaTimes  = [vanillaDict[s]["totalRuntime"] if s in vanillaDict else -1 for s in samplesTotal]
maskCOIColors = [cellColor(maskCOIDict[s]["result"]) if s in maskCOIDict else None for s in samplesTotal]
vanillaColors = [cellColor(vanillaDict[s]["result"]) if s in vanillaDict else None for s in samplesTotal]
colors = np.transpose(np.array([maskCOIColors, vanillaColors]))
df = pd.DataFrame(np.transpose(np.array([maskCOITimes, vanillaTimes])), columns=['CNN Abstraction', 'Vanilla'], index=samplesTotal)
ax.table(cellText=df.values, colLabels=df.columns, rowLabels=df.index, loc='center', cellColours=colors)

fig.tight_layout()
plt.savefig("PerSampleResult.png")

