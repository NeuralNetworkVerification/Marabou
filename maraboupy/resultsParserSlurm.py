from __future__ import division
import os
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np
import json
import csv
import pandas as pd
from matplotlib.ticker import MaxNLocator
from datetime import datetime

def pctFunc(pct, data):
    absolute = int(np.round(pct/100.*np.sum(data)))
    return "{:1.1f}%\n({})".format(pct, absolute) if pct > 0 else ""

def countSame(x,y):
    xy = list(zip(x,y))
    xyCount = [xy.count(el) for el in xy]    
    return list(zip(xyCount, xy))

def cellColor(result, dark=False):
    if result.upper() == 'TIMEOUT':
        return 'yellow' if not dark else 'gold'
    elif result.upper() == 'UNSAT':
        return 'green' if not dark else 'darkgreen'
    elif result.upper() == 'SAT':
        return 'red' if not dark else 'darkred'
    return None

def markerChoice(result):
    if result.upper() == 'TIMEOUT':
        return "x"
    elif result.upper() == 'UNSAT':
        return "|" 
    elif result.upper() == 'SAT':
        return "_" 
    return None

def setFigSize(w=12, h=9):
    figure = plt.gcf()
    figure.set_size_inches(w, h)

def plotCompareProperties(xDict, yDict):
    xLabel = xDict['label']
    yLabel = yDict['label']
    plt.figure()
    LOGSCALE = True
    if LOGSCALE:
        plt.xscale('log')
        plt.yscale('log')

    intersectSet = set(resultDicts[0].keys())
    for resultDict in resultDicts :
        intersectSet = intersectSet & set(resultDict.keys())    
    mutual = list(intersectSet)
    mutual.remove('label')
    
    x = [xDict[sample]["totalRuntime"] for sample in mutual]
    y = [yDict[sample]["totalRuntime"] for sample in mutual]
    xyCountSame = countSame(x,y)
    plt.scatter(x, y, s=70, marker="x", alpha=0.3)
    for count, coor in countSame(x,y):
        if count > 1:
            plt.annotate(count, coor)
    bottom = 1 if LOGSCALE else 0
    top = TIMEOUT_VAL
    axisTop = 2 * TIMEOUT_VAL if LOGSCALE else TIMEOUT_VAL + 100
    plt.plot([bottom, top], [bottom, top], 'r:')
    plt.plot([bottom, top], [top   , top], 'r:')
    plt.plot([top,    top], [bottom, top], 'r:')
    plt.title("{} vs. {}, {} samples".format(yLabel, xLabel, len(mutual)))
    plt.xlabel(xLabel + ' [sec]')
    plt.ylabel(yLabel + ' [sec]')
    plt.xlim([bottom, axisTop])
    plt.ylim([bottom, axisTop])

    setFigSize()
    noWhiteXLabel = xLabel.replace(' ','')
    noWhiteYLabel = yLabel.replace(' ','')
    plt.savefig("CompareProperties-{}_vs_{}.png".format(noWhiteXLabel, noWhiteYLabel), dpi=100)
    plt.close()

def plotCOIRatio(resultDict):
    plt.figure()
    fig, (ax1, ax2) = plt.subplots(nrows=2, ncols=1)
    ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax2.xaxis.set_major_locator(MaxNLocator(integer=True))
    solved = [k for k,v in resultDict.items() if k != 'label' and v["result"] in ["UNSAT", "SAT", "TIMEOUT"]]
    x  = [resultDict[sample]["finalPartiallity"]["numRuns"]   for sample in solved]
    y1 = [resultDict[sample]["finalPartiallity"]["vars"]      for sample in solved]
    y2 = [resultDict[sample]["finalPartiallity"]["equations"] for sample in solved]
    c  = [cellColor(resultDict[sample]["result"]) for sample in solved]
    marker = [markerChoice(resultDict[sample]["result"]) for sample in solved]
    fig.suptitle("{} - Variables and Equations Ratio, {} Samples".format(resultDict["label"], len(solved)))
    ax2.set_xlabel("Number of Runs")
    ax1.set_ylabel("Eventual Variables Ratio")
    ax2.set_ylabel("Eventual Equation Ratio")
    ax1.scatter(x, y1, s=70, alpha=0.3, c=c)#, marker=marker)
    ax2.scatter(x, y2, s=70, alpha=0.3, c=c)#, marker=marker)

    for count, coor in countSame(x,y1):
        ax1.annotate(count, coor)
    for count, coor in countSame(x,y2):
        ax2.annotate(count, coor)

    setFigSize()
    noWhiteLabel = resultDict['label'].replace(' ','')
    plt.savefig("COIRatio-{}.png".format(noWhiteLabel), dpi=100)
    plt.close()    

####################################################################################################
####################################################################################################
####################################################################################################    
    
resultsFiles = list()
for root, dirs, files in os.walk(os.getcwd()):
    for f in files:
        if str(f) == "Results.json":
            fullpath = root + "/" + f
            resultsFiles.append(fullpath)

parser = argparse.ArgumentParser(description='Query log files')
parser.add_argument("--batch", type=str, default="", help="Limit to a specifc batch")            
args = parser.parse_args()

####################################################################################################
####################################################################################################
####################################################################################################

if os.path.isfile(os.getcwd() + "/plotSpec.json"):
    with open(os.getcwd() + "/plotSpec.json", "r") as f:
        plotSpec = json.load(f)
        comparePropertiesPairs = plotSpec["compareProperties"]
        COIRatioKeys  = plotSpec["COIRatio"] 
        runTitleToLabel = plotSpec["title2Label"]
        TIMEOUT_VAL = plotSpec["TIMEOUT_VAL"]
else:
    comparePropertiesPairs = [('VanillaCfg', 'MaskCOICfg')]
    COIRatioKeys = ['MaskCOICfg']
    runTitleToLabel = {'MaskCOICfg' : 'CNN Abstraction', 'VanillaCfg' : 'Vanilla Marabou'}
    TIMEOUT_VAL = 12 * 3600

####################################################################################################
####################################################################################################
####################################################################################################


results = dict()
for fullpath in resultsFiles:
    if args.batch and args.batch not in fullpath:
        continue
    with open(fullpath, "r") as f:
        resultDict = json.load(f)
        if "cfg_runTitle" in resultDict:
            runTitle = resultDict["cfg_runTitle"].split("---")[0]
        else:
            runTitle = resultDict["cfg_runSuffix"].split("---")[0]
        if resultDict["subResults"] and resultDict["subResults"][-1]["originalQueryStats"] and resultDict["subResults"][-1]["finalQueryStats"]:
            originalQueryStats = resultDict["subResults"][-1]["originalQueryStats"]
            finalQueryStats = resultDict["subResults"][-1]["finalQueryStats"]
            finalPartiallity = dict()            
            finalPartiallity["vars"] = finalQueryStats["numVars"] / originalQueryStats["numVars"]
            finalPartiallity["equations"] = finalQueryStats["numEquations"] / originalQueryStats["numEquations"]
            finalPartiallity["reluConstraints"] = finalQueryStats["numReluConstraints"] / originalQueryStats["numReluConstraints"]
            finalPartiallity["numRuns"] = len(resultDict["subResults"])
        else:
            originalQueryStats = dict()
            finalQueryStats = dict()
            finalPartiallity = dict(numRuns=1 if resultDict["subResults"] else -1, vars=-1, equations=-1, reluConstraints=-1)
            
        assert len(originalQueryStats) == len(finalQueryStats)
        successfulRuntime = -1 if resultDict["Result"].upper() == "TIMEOUT" or not ("successfulRuntime" in resultDict) else resultDict["successfulRuntime"]
        totalRuntime = TIMEOUT_VAL if resultDict["Result"].upper() == "TIMEOUT" else resultDict["totalRuntime"]
        sampleIndex = resultDict["cfg_sampleIndex"]
                        
        if runTitle not in results:
            results[runTitle] = dict()
        results[runTitle][sampleIndex] = {"result" : resultDict["Result"].upper(),
                                          "totalRuntime" : totalRuntime,
                                          "originalQueryStats": originalQueryStats,
                                          "finalQueryStats": finalQueryStats,
                                          "finalPartiallity" : finalPartiallity,
                                          "successfulRuntime":successfulRuntime}
        results[runTitle]["label"] = runTitleToLabel[runTitle]

resultDicts = list(results.values())

graphDir = os.getcwd() + "/__Graphs_" + datetime.now().strftime("%d-%m-%y___%H-%M-%S")
if not os.path.exists(graphDir):
    os.mkdir(graphDir)
os.chdir(graphDir)

####################################################################################################
####################################################################################################
####################################################################################################

tableLabels = [resultDict['label'] + ' [sec]' for resultDict in resultDicts]
plt.figure()
fig, ax = plt.subplots(nrows=1, ncols=1)
fig.patch.set_visible(False)
ax.axis('off')
ax.axis('tight')

addPlus = lambda runtime: "{:.2f}".format(runtime) if runtime < TIMEOUT_VAL else (str(TIMEOUT_VAL) + "+")
totalSet = set()
for resultDict in resultDicts :
    totalSet = totalSet | set(resultDict.keys())
samplesTotal = list(totalSet)
samplesTotal.remove('label')

runtimesSuccessful = [[resultDict[s]["successfulRuntime"]  if s in resultDict else -1        for s in samplesTotal] for resultDict in resultDicts]
runtimesTotal   = [[addPlus(resultDict[s]["totalRuntime"])  if s in resultDict else -1        for s in samplesTotal] for resultDict in resultDicts]
numRuns   = [resultDict[sample]["finalPartiallity"]["numRuns"]   for sample in samplesTotal]
varsPartial      = [resultDict[sample]["finalPartiallity"]["vars"]      for sample in samplesTotal]
equationsPartial = [resultDict[sample]["finalPartiallity"]["equations"] for sample in samplesTotal]

runColors  = [[cellColor(resultDict[s]["result"])      if s in resultDict else None      for s in samplesTotal] for resultDict in resultDicts]

def plotResultSummery(name, tableLabels, sampleTotal, runColors, results):
    plt.figure()
    colors = np.transpose(np.array(runColors))
    df = pd.DataFrame(np.transpose(np.array(results)), columns=tableLabels, index=samplesTotal)
    table = ax.table(cellText=df.values, colLabels=df.columns, colWidths=[2.0 / len(resultDicts)] * len(resultDicts), rowLabels=df.index, loc='center', cellColours=colors)
    table.auto_set_font_size(False)
    table.set_fontsize(12)
    fig.canvas.draw()
    setFigSize()
    plt.savefig("ResultSummary_{}.png".format(name), bbox_inches="tight", dpi=100)
    plt.close()

plotResultSummery("totalRuntime",      tableLabels, sampleTotal, runColors, runtimesTotal)
plotResultSummery("SuccessfulRuntime", tableLabels, sampleTotal, runColors, runtimesSuccessful)
plotResultSummery("numRuns",           tableLabels, sampleTotal, runColors, numRuns)
plotResultSummery("RelativeVars",      tableLabels, sampleTotal, runColors, varsPartial)
plotResultSummery("equationsVars",     tableLabels, sampleTotal, runColors, equationsPartial)
    
runResults = [[resultDict[s]["result"]                 if s in resultDict else "MISSING" for s in samplesTotal] for resultDict in resultDicts]

with open('ResultSummary.csv', mode='w') as f:
        wr = csv.writer(f, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
        resultSubRows = [list(zip(runtimeTotal, runtimeSuccessful, runResult, numRuns, varsP, eqP)) for runtimeTotal, runResult, runtimeSuccessful, runsP, varsP, eqP in zip(runtimesTotal, runtimesSuccessful, runResults, numRuns, varsPartial, equationsPartial)]
        resultRows = [[item for rTuple in rTuples for item in rTuple] for rTuples in zip(*resultSubRows)]
        subRowLabels = ['[sec]', 'Result']
        
        wr.writerow([''] + [resultDict['label'] + ' ' + subRowLabel for resultDict in resultDicts for subRowLabel in subRowLabels])
        for sample, resultRow in zip(samplesTotal, resultRows):
            wr.writerow([sample] + resultRow)
            
####################################################################################################
####################################################################################################
####################################################################################################

cactusLabels = [resultDict['label'] + ' [sec]' for resultDict in resultDicts]
plt.figure()

runtimesSolved = [[resultDict[s]["totalRuntime"] for s in samplesTotal if (s in resultDict) and (resultDict[s]["result"] in ["SAT", "UNSAT"])] for resultDict in resultDicts]
[result.sort() for result in runtimesSolved]
sumRuntimes = [[sum(result[:i+1]) for i in range(len(result))] for result in runtimesSolved]
#[plt.stairs(sums, list(range(1,len(sums)+1)), label=label) for sums, label in zip(sumRuntimes, cactusLabels)] FIXME
[plt.plot(sums, list(range(1,len(sums)+1)), label=label) for sums, label in zip(sumRuntimes, cactusLabels)]
plt.xlabel("Runtime")
plt.ylabel("Instances solved")
plt.legend()
setFigSize()
plt.savefig("Cactus.png", bbox_inches="tight", dpi=100)
plt.close()
            
####################################################################################################
####################################################################################################
####################################################################################################
plt.figure()

# Pie chart, where the slices will be ordered and plotted counter-clockwise:
resultLabels = 'SAT', 'UNSAT', 'TIMEOUT'
runSampleNum = [[len([v for k,v in resultDict.items() if k != 'label' and v["result"] == l]) for l in resultLabels] for resultDict in resultDicts]

fig, axes = plt.subplots(nrows=len(resultDicts), ncols=1)
fig.suptitle("Run Results", fontsize=20)
plt.subplots_adjust(wspace=1)
for resultDict, ax , sampleNum in zip(resultDicts, axes, runSampleNum): 
    ax.pie(sampleNum, labels=[l if s > 0 else "" for l,s in zip(resultLabels, sampleNum)], colors=["darkred","darkgreen","gold"], autopct=lambda pct: pctFunc(pct, sampleNum), shadow=True, startangle=90, textprops=dict(color="k"))
    ax.set_title("{}, {} samples".format(resultDict['label'], len(resultDict)-1), fontdict=dict(fontsize=12))
    ax.axis('equal')

    setFigSize()
plt.savefig("ResultPie.png", dpi=100)
plt.close()

####################################################################################################
####################################################################################################
####################################################################################################

for (x,y) in comparePropertiesPairs:
    plotCompareProperties(results[x], results[y])

for k in COIRatioKeys:
    plotCOIRatio(results[k])


