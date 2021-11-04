
## This file is use to parse the Result.json files, but also creates graphs that DO NOT appear in the paper.

from __future__ import division
import os
import sys
import argparse
import matplotlib.pyplot as plt
import numpy as np
import json
import csv
import pandas as pd
import re
from matplotlib.ticker import MaxNLocator
import matplotlib.lines as mlines
from datetime import datetime
import shutil
from itertools import chain
import CnnAbs

#Save JSON file from data.
def dumpJson(data, name):
    if not name.endswith(".json"):
        name += ".json"
    with open(name, "w") as f:
        json.dump(data, f, indent = 4)

#Graph printing function.        
def pctFunc(pct, data):
    absolute = int(np.round(pct/100.*np.sum(data)))
    return "{:1.1f}%\n({})".format(pct, absolute) if pct > 0 else ""

#Count appreances of tuple in two lists.
def countSame(x,y):
    xy = list(zip(x,y))
    xyCount = [xy.count(el) for el in xy]    
    return list(zip(xyCount, xy))

#Choose color for specific xresult.
def cellColor(result, dark=False):
    if result.upper() == 'GTIMEOUT':
        return 'yellow' if not dark else 'gold'
    if result.upper() == 'TIMEOUT':
        return 'orange' if not dark else 'darkorange'
    elif result.upper() == 'UNSAT':
        return 'green' if not dark else 'darkgreen'
    elif result.upper() == 'SAT':
        return 'red' if not dark else 'darkred'
    elif result.upper() == 'SPURIOUS':
        return 'fuchsia' if not dark else 'deeppink'
    elif result.upper() == 'ERROR':
        return 'gray' if not dark else 'darkgray' 
    else:
        return 'black'
    return None

#Choose marker for specific result.
def markerChoice(result):
    if result.upper() == 'TIMEOUT':
        return "x"
    elif result.upper() == 'UNSAT':
        return "|" 
    elif result.upper() == 'SAT':
        return "_"
    else:
        return "x"
    return None

# Set figure size.
def setFigSize(w=12, h=9):
    figure = plt.gcf()
    figure.set_size_inches(w, h)

# Plot graph - not used in paper
def plotCompareProperties(xDict, yDict, marker="x", newFig=True, singleFig=True, lastFig=True, ax=plt):
    xLabel = xDict['label']
    yLabel = yDict['label']
    if singleFig:
        plt.figure()
    LOGSCALE = True
    if LOGSCALE:
        if singleFig:
            ax.xscale('log')
            ax.yscale('log')
        else:
            ax.set_xscale('log')
            ax.set_yscale('log')

    intersectSet = set(resultDicts[0].keys())
    for resultDict in resultDicts :
        intersectSet = intersectSet & set(resultDict.keys())    
    mutual = list(intersectSet)
    mutual.remove('label')
    
    x = [xDict[xparam]["totalRuntime"] for xparam in mutual]
    y = [yDict[xparam]["totalRuntime"] for xparam in mutual]
    c = []
    markerArr = []
    chooseMarker = lambda mark: 'o' if mark == 'UNSAT' else ('X' if mark == 'SAT' else '*')
    for xparam in mutual:
        xResult = xDict[xparam]["result"].upper()
        yResult = yDict[xparam]["result"].upper()
        assert not (xResult == "SAT"   and yResult == "UNSAT")
        assert not (xResult == "UNSAT" and yResult == "SAT"  )
        if (xResult == yResult) or (yResult in ["TIMEOUT", "GTIMEOUT", "SPURIOUS", "ERROR"]):
            if yResult in ["SAT", "UNSAT"]:
                c.append(cellColor(xResult))
            else:
                c.append(cellColor('TIMEOUT'))
            markerArr.append(chooseMarker(xResult))
        else:
            assert (yResult in ["SAT", "UNSAT"]) and (xResult in ["TIMEOUT", "GTIMEOUT", "SPURIOUS", "ERROR"])
            c.append(cellColor('TIMEOUT'))
            markerArr.append(chooseMarker(yResult))

    xyCountSame = countSame(x,y)
    for _x, _y, _c, _m in zip(x, y, c, markerArr):
        ax.scatter(_x, _y, s=70, marker=_m, facecolor='none', edgecolor=_c)
    for count, coor in countSame(x,y):
        if count > 1:
            ax.annotate(count, coor)
    if newFig:
        bottom = 1 if LOGSCALE else 0
        top = TIMEOUT_VAL
        axisTop = 2 * TIMEOUT_VAL if LOGSCALE else TIMEOUT_VAL + 100
        ax.plot([bottom, top], [bottom, top], 'r:')
        ax.plot([bottom, top], [top   , top], 'r:')
        ax.plot([top,    top], [bottom, top], 'r:')
        if singleFig:
            plt.title("Total Runtime: {} vs. {}  ({} samples)".format(yLabel, xLabel, len(mutual)))        
            plt.xlabel(xLabel + ' Runtime [sec]')
            plt.ylabel(yLabel + ' Runtime  [sec]')
            plt.xlim([bottom, axisTop])
            plt.ylim([bottom, axisTop])
            plt.grid(True)
        else:
            ax.set_xlim([bottom, axisTop])
            ax.set_ylim([bottom, axisTop])            

    if lastFig:
        setFigSize()
        if singleFig:
            noWhiteXLabel = xLabel.replace(' ','')
            noWhiteYLabel = yLabel.replace(' ','')
            dumpJson({'x': x, 'y': y, 'marker':markerArr, 'color':c}, 'CompareProperties-{}_vs_{}'.format(noWhiteXLabel, noWhiteYLabel))
            plt.savefig("CompareProperties-{}_vs_{}.png".format(noWhiteXLabel, noWhiteYLabel), dpi=100)            
        else:
            ax.set_title("Total Runtime: All vs. Vanilla")
            ax.set_xlabel('Vanilla [sec]')
            ax.set_ylabel('Others [sec]')
            ax.grid(True)
            ax.figure.savefig("CompareProperties-All_vs_Vanilla.png", dpi=100)        
        plt.close()

# Count repeatitions in x.
def countValues(x):
    values = set(x)
    values.discard(-1)
    values = list(values)
    values.sort()
    repeats = [len([u for u in x if v == u]) for v in values]
    return values, repeats

# plot graph - does not appear in paper.
def plotCOIRatio(resultDict):
    plt.figure()
    fig, (ax1, ax2) = plt.subplots(nrows=2, ncols=1)
    ax1.xaxis.set_major_locator(MaxNLocator(integer=True))
    ax2.xaxis.set_major_locator(MaxNLocator(integer=True))    
    solved = [k for k,v in resultDict.items() if k != 'label' and v["result"] in ["UNSAT", "SAT"]]
    x  = [resultDict[xparam]["finalPartiallity"]["numRuns"]   for xparam in solved]
    totalAbsRefineBatches = next(resultDict[xparam]["finalPartiallity"]["outOf"] for xparam in solved if "outOf" in resultDict[xparam]["finalPartiallity"])
    y1 = [resultDict[xparam]["finalPartiallity"]["vars"]      for xparam in solved]
    y2 = [resultDict[xparam]["finalPartiallity"]["equations"] for xparam in solved]
    results = [resultDict[xparam]["result"] for xparam in solved]
    c  = [cellColor(resultDict[xparam]["result"]) for xparam in solved]
    marker = [markerChoice(resultDict[xparam]["result"]) for xparam in solved]
    fig.suptitle("{} - Variables and Equations Ratio, {} Xparams".format(resultDict["label"], len(solved)))
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
    plt.grid(True)
    plt.savefig("COIRatio-{}.png".format(noWhiteLabel), dpi=100)
    plt.close()

    plt.figure()
    uniqueVarRatio, repeatsVarRatio = countValues(y1)
    dumpJson({'ratio': uniqueVarRatio, 'repeats': repeatsVarRatio}, "VariableRatioHistogram-{}".format(noWhiteLabel))
    plt.plot(uniqueVarRatio, repeatsVarRatio, marker='o', markerfacecolor='none', color='blue', markeredgecolor='blue')    

    plt.xlabel('Remaining Variables Ratio In Successful Run - {}'.format(resultDict['label']))
    plt.ylabel('Num. Samples')
    plt.xlim([0,1])
    plt.grid(True)    
    plt.savefig("VariableRatioHistogram-{}.png".format(noWhiteLabel), dpi=100)
    plt.close()

    plt.figure()
    uniqueNumRuns, repeatsNumRuns = countValues(x)
    plt.plot(uniqueNumRuns, repeatsNumRuns, marker='x', markerfacecolor='none', color='red', markeredgecolor='red')
    dumpJson({'numRuns':uniqueNumRuns, 'repeats': repeatsNumRuns, 'totalAbsRefineBatches' : totalAbsRefineBatches}, "NumRunsHistogram-{}".format(noWhiteLabel))

    plt.xlabel('Number of solver Runs Till Success - {}'.format(resultDict['label']))
    plt.ylabel('Num. Samples')
    plt.grid(True)    
    plt.savefig("NumRunsHistogram-{}.png".format(noWhiteLabel), dpi=100)
    plt.close()

    uniqueVarRatio, repeatsVarRatio = countValues([var for var,result in zip(y1, results) if result == "SAT"])
    dumpJson({'ratio': uniqueVarRatio, 'repeats': repeatsVarRatio, "SAT":None, 'totalAbsRefineBatches' : totalAbsRefineBatches}, "VariableRatioHistogramSAT-{}".format(noWhiteLabel))
    uniqueVarRatio, repeatsVarRatio = countValues([var for var,result in zip(y1, results) if result == "UNSAT"])
    dumpJson({'ratio': uniqueVarRatio, 'repeats': repeatsVarRatio, "UNSAT":None, 'totalAbsRefineBatches' : totalAbsRefineBatches}, "VariableRatioHistogramUNSAT-{}".format(noWhiteLabel))
    uniqueNumRuns, repeatsNumRuns = countValues([var for var,result in zip(x, results) if result == "SAT"])
    dumpJson({'numRuns': uniqueNumRuns, 'repeats': repeatsNumRuns, "SAT":None, 'totalAbsRefineBatches' : totalAbsRefineBatches}, "NumRunsHistogramSAT-{}".format(noWhiteLabel))
    uniqueNumRuns, repeatsNumRuns = countValues([var for var,result in zip(x, results) if result == "UNSAT"])
    dumpJson({'numRuns': uniqueNumRuns, 'repeats': repeatsNumRuns, "UNSAT":None, 'totalAbsRefineBatches' : totalAbsRefineBatches}, "NumRunsHistogramUNSAT-{}".format(noWhiteLabel))                 
    

####################################################################################################
####################################################################################################
####################################################################################################    
    
resultsFiles = list()
cntTrace = 0
cntAssert = 0
cntViolation = 0
cntKill = 0
for root, dirs, files in os.walk(os.getcwd()):
    for f in files:
        fullpath = root + "/" + f        
        if str(f) == "Results.json":
            resultsFiles.append(fullpath)            
        if str(f).endswith(".out") or str(f).endswith(".log"):
            with open(fullpath, "r") as fRead:
                for line in fRead:
                    if "trace" in line.lower():
                        print("Found string 'trace' in {}, line is:\n{}".format(fullpath, line))
                        cntTrace += 1
                    if "assert" in line.lower():
                        print("Found string 'assert' in {}, line is:\n{}".format(fullpath, line))
                        cntAssert += 1                        
                    if "violation" in line.lower():
                        print("Found string 'violation' in {}, line is:\n{}".format(fullpath, line))
                        cntViolation += 1                        
                    if "kill" in line.lower():
                        print("Found string 'kill' in {}, line is:\n{}".format(fullpath, line))
                        cntKill += 1


if cntTrace > 0:
    print("\n\n ******** Found 'trace' {} times!".format(cntTrace))
if cntAssert > 0:
    print("\n\n ******** Found 'assert' {} times!".format(cntAssert))
if cntViolation > 0:
    print("\n\n ******** Found 'violation' {} times!".format(cntViolation))
if cntKill > 0:
    print("\n\n ******** Found 'kill' {} times!".format(cntKill))

parser = argparse.ArgumentParser(description='Parse log files')
parser.add_argument("--batch", type=str, default="", help="Limit to a specifc batch -- deprecated option.")
parser.add_argument("--graph_dir_name", type=str, default="", help="Name of the graph directories to dump in (under the graphs directory in top of tree.")
parser.add_argument('--force'   , dest='force', action='store_true',  help="Force overwrite of graph directory.")
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
        commonRunCommand = plotSpec["commonRunCommand"]
        runCommands = plotSpec["runCommands"]
        xparameter = plotSpec["xparameter"] if "xparameter" in plotSpec else "cfg_sampleIndex"
else:
    comparePropertiesPairs = [('VanillaCfg', 'MaskCOICfg')]
    COIRatioKeys = ['MaskCOICfg']
    runTitleToLabel = {'MaskCOICfg' : 'CNN Abstraction', 'VanillaCfg' : 'Vanilla Marabou'}
    TIMEOUT_VAL = 12 * 3600
    xparameter = "cfg_sampleIndex"

####################################################################################################
####################################################################################################
####################################################################################################

policies = set()
results = dict()
for fullpath in resultsFiles:
    if args.batch and args.batch not in fullpath:
        continue
    with open(fullpath, "r") as f:
        resultDict = json.load(f)
        if 'cfg_abstractionPolicy' in resultDict:
            policies.add(resultDict['cfg_abstractionPolicy'])
        if "cfg_runTitle" in resultDict:
            runTitle = resultDict["cfg_runTitle"].split("---")[0]
        else:
            r = re.compile("cfg_runTitle.*")
            matches = list(filter(r.match, resultDict.keys()))
            if matches:
                runTitle = resultDict[matches[0]].split("---")[0]
            elif "cfg_runSuffix" in resultDict:
                runTitle = resultDict["cfg_runSuffix"].split("---")[0]
            else:
                r = re.compile("cfg_runSuffix.*")
                matches = list(filter(r.match, resultDict.keys()))
                if matches:
                    runTitle = resultDict[matches[0]].split("---")[0]
                else:
                    raise Exception("No runtitle matches")
        if resultDict["subResults"] and resultDict["subResults"][-1]["originalQueryStats"] and resultDict["subResults"][-1]["finalQueryStats"]:
            originalQueryStats = resultDict["subResults"][-1]["originalQueryStats"]
            finalQueryStats = resultDict["subResults"][-1]["finalQueryStats"]
            finalPartiallity = dict()            
            finalPartiallity["vars"] = finalQueryStats["numVars"] / originalQueryStats["numVars"]
            finalPartiallity["equations"] = finalQueryStats["numEquations"] / originalQueryStats["numEquations"]
            finalPartiallity["reluConstraints"] = finalQueryStats["numReluConstraints"] / originalQueryStats["numReluConstraints"]
            finalPartiallity["numRuns"] = len(resultDict["subResults"])
            finalPartiallity["outOf"] = resultDict["subResults"][-1]["outOf"] + 1
        else:
            originalQueryStats = dict()
            finalQueryStats = dict()
            finalPartiallity = dict(numRuns=1 if resultDict["subResults"] else (0 if resultDict["Result"] == "UNSAT" else -1), vars=-1, equations=-1, reluConstraints=-1)
            
        assert len(originalQueryStats) == len(finalQueryStats)
        successfulRuntime = -1 if resultDict["Result"].upper() in ["TIMEOUT", "GTIMEOUT", "SPURIOUS", "ERROR"] or not ("successfulRuntime" in resultDict) else resultDict["successfulRuntime"]
        #totalRuntime = TIMEOUT_VAL if resultDict["Result"].upper() == "TIMEOUT" else resultDict["totalRuntime"]
        totalRuntime = TIMEOUT_VAL if not "totalRuntime" in resultDict else resultDict["totalRuntime"]
        if xparameter in resultDict:
            param = resultDict[xparameter]
        else:
            r = re.compile(xparameter + ".*")
            matches = list(filter(r.match, resultDict.keys()))
            param = resultDict[matches[0]]
                        
        if runTitle not in results:
            results[runTitle] = dict()
        results[runTitle][param] = {"result" : resultDict["Result"].upper(),
                                          "totalRuntime" : totalRuntime,
                                          "originalQueryStats": originalQueryStats,
                                          "finalQueryStats": finalQueryStats,
                                          "finalPartiallity" : finalPartiallity,
                                          "successfulRuntime":successfulRuntime}
        results[runTitle]["label"] = runTitleToLabel[runTitle]

resultDicts = list(results.values())

origDir = os.getcwd()

graphDirName = args.graph_dir_name if args.graph_dir_name else "__Graphs_" + datetime.now().strftime("%d-%m-%y___%H-%M-%S")
graphDir = '/'.join([CnnAbs.CnnAbs.basePath, "graphs", graphDirName])
if os.path.exists(graphDir):
    if args.force:
        shutil.rmtree(graphDir, ignore_errors=True)
    else:
        raise Exception("Trying to overwrite graph directory")
os.makedirs(graphDir, exist_ok=True)


shutil.copy2("runCmd.sh", graphDir)

os.chdir(graphDir)

####################################################################################################
####################################################################################################
####################################################################################################

tableLabels = [resultDict['label'] + ' [sec]' for resultDict in resultDicts]

addPlus = lambda runtime: "{:.2f}".format(runtime) if runtime < TIMEOUT_VAL else (str(TIMEOUT_VAL) + "+")
totalSet = set()
for resultDict in resultDicts :    
    totalSet = totalSet | set(resultDict.keys())
xparamTotal = list(totalSet)
xparamTotal.remove('label')
xparamTotal.sort()

#notVanilla = 0 if 'vanilla' not in resultDicts[0]['label'].lower() else 1
#mutualSet = set(resultDicts[0].keys())
#mutualSetNoVanilla = set(resultDicts[notVanilla].keys())
#for resultDict in resultDicts :
#    if not 'vanilla' in resultDict['label'].lower():
#        mutualSetNoVanilla = mutualSetNoVanilla & set(resultDict.keys())
#    mutualSet = mutualSet & set(resultDict.keys())
#xparamMutual = list(mutualSet)
#xparamMutual.remove('label')
#xparamMutual.sort()
#xparamMutualNoVanilla = list(mutualSetNoVanilla)
#xparamMutualNoVanilla.remove('label')
#xparamMutualNoVanilla.sort()

solvedSamples = {policy : set() for policy in policies}
totalRuntimes = {policy : dict() for policy in policies}
for resultDict in resultDicts:
    for policy in policies:
        if policy.lower() in resultDict['label'].lower():
            for sample in resultDict.keys():
                if sample != 'label' and resultDict[sample]['result'] in ['SAT', 'UNSAT']:
                    solvedSamples[policy].add(sample)
                    totalRuntimes[policy][sample] = resultDict[sample]['totalRuntime']

for policy in policies:                    
    solvedSamples[policy] = list(solvedSamples[policy])
totalSolved = list(set(chain(*solvedSamples.values())))
mutualSetNoVanilla = set(solvedSamples['Random'])
mutualSet = set(solvedSamples['Random'])
for policy, solved in solvedSamples.items():
    if policy != 'Vanilla':
        mutualSetNoVanilla = mutualSetNoVanilla & set(solved)
    mutualSet = mutualSet & set(solved)
solvedSamples['mutualNoVanilla'] = list(mutualSetNoVanilla)
solvedSamples['mutual'] = list(mutualSet)
solvedSamples['total'] = totalSolved
solvedSamples['totalRuntimes'] = totalRuntimes
dumpJson(solvedSamples, 'solvedSamples')

runtimesSuccessful = [[resultDict[s]["successfulRuntime"]  if s in resultDict else -1        for s in xparamTotal] for resultDict in resultDicts]
runtimesTotal   = [[addPlus(resultDict[s]["totalRuntime"])  if s in resultDict else -1        for s in xparamTotal] for resultDict in resultDicts]
numRuns          = [[resultDict[xparam]["finalPartiallity"]["numRuns"]   for xparam in xparamTotal] for resultDict in resultDicts]
varsPartial      = [[resultDict[xparam]["finalPartiallity"]["vars"]      for xparam in xparamTotal] for resultDict in resultDicts]
equationsPartial = [[resultDict[xparam]["finalPartiallity"]["equations"] for xparam in xparamTotal] for resultDict in resultDicts]

runColors  = [[cellColor(resultDict[s]["result"])      if s in resultDict else None      for s in xparamTotal] for resultDict in resultDicts]

# Plot summary table - not used in paper.
def plotResultSummery(name, tableLabels, xparamTotal, runColors, results):
    plt.figure()
    fig, ax = plt.subplots(nrows=1, ncols=1)
    fig.patch.set_visible(False)
    ax.axis('off')
    ax.axis('tight')
    colors = np.transpose(np.array(runColors))
    df = pd.DataFrame(np.transpose(np.array(results)), columns=tableLabels, index=xparamTotal)
    table = ax.table(cellText=df.values, colLabels=df.columns, colWidths=[2.0 / len(resultDicts)] * len(resultDicts), rowLabels=df.index, loc='center', cellColours=colors)
    table.auto_set_font_size(False)
    table.set_fontsize(12)
    fig.canvas.draw()
    setFigSize()
    plt.savefig("ResultSummary_{}.png".format(name), bbox_inches="tight", dpi=100)
    plt.close()

plotResultSummery("totalRuntime",      tableLabels, xparamTotal, runColors, runtimesTotal)
plotResultSummery("SuccessfulRuntime", tableLabels, xparamTotal, runColors, runtimesSuccessful)
plotResultSummery("numRuns",           tableLabels, xparamTotal, runColors, numRuns)
plotResultSummery("RelativeVars",      tableLabels, xparamTotal, runColors, varsPartial)
plotResultSummery("equationsVars",     tableLabels, xparamTotal, runColors, equationsPartial)
    
runResults = [[resultDict[s]["result"]                 if s in resultDict else "MISSING" for s in xparamTotal] for resultDict in resultDicts]

with open('ResultSummary.csv', mode='w') as f:
        wr = csv.writer(f, delimiter=',', quotechar='"', quoting=csv.QUOTE_MINIMAL)
        resultSubRows = [list(zip(runtimeTotal, runtimeSuccessful, runResult, numRuns, varsP, eqP)) for runtimeTotal, runResult, runtimeSuccessful, runsP, varsP, eqP in zip(runtimesTotal, runtimesSuccessful, runResults, numRuns, varsPartial, equationsPartial)]
        resultRows = [[item for rTuple in rTuples for item in rTuple] for rTuples in zip(*resultSubRows)]
        subRowLabels = ['[sec]', 'Result']
        
        wr.writerow([''] + [resultDict['label'] + ' ' + subRowLabel for resultDict in resultDicts for subRowLabel in subRowLabels])
        for xparam, resultRow in zip(xparamTotal, resultRows):
            wr.writerow([xparam] + resultRow)
            
####################################################################################################
####################################################################################################
####################################################################################################

cactusLabels = [resultDict['label'] for resultDict in resultDicts]
resultDicts = [rd for _, rd in sorted(zip(cactusLabels, resultDicts))]
cactusLabels.sort()
cactusMarkers = ['*', 's', 'o', 'P', 'X', '^', 'p', 'D'][:len(cactusLabels)]
cactusColors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:brown', 'tab:pink', 'tab:cyan']

plt.figure()
runtimesTotalSolved = [[resultDict[s]["totalRuntime"] for s in xparamTotal if (s in resultDict) and (resultDict[s]["result"] in ["SAT", "UNSAT"])] for resultDict in resultDicts]
[result.sort() for result in runtimesTotalSolved]
sumRuntimes = [[sum(result[:i+1]) for i in range(len(result))] for result in runtimesTotalSolved]
lines = []
#[plt.plot(sums, list(range(1,len(sums)+1)), label=label) for sums, label in zip(sumRuntimes, cactusLabels)]
for sums, label, marker, color in zip(sumRuntimes, cactusLabels, cactusMarkers, cactusColors):
    solved = [0] + list(range(1,len(sums)+1))
    sums = [0] + sums
    dumpJson({'x': sums, 'y': solved}, 'CactusTotalRuntime_pltstep_{}'.format(label))
    p = plt.step(sums, solved, label=label, where="post", c=color)
    #plt.scatter(sums[-1], solved[-1], s=70, marker=marker, alpha=0.3, c=color)
    plt.scatter(sums[-1], solved[-1], s=70, marker=marker, alpha=1, facecolor='none', edgecolor=color)
    lines.append(mlines.Line2D([], [], markerfacecolor='none', markeredgecolor=color, marker=marker, markersize=10, label=label, color=color))
    #print("label={}".format(label))
    #print("sums={}".format(sums))
    #print("solved={}".format(solved))        
plt.xlabel("Total Runtime [sec]")
plt.ylabel("Instances solved")
plt.legend(handles=lines)
setFigSize()
plt.grid(True)
plt.savefig("CactusTotal.png", bbox_inches="tight", dpi=100)
plt.close()

####################################################################################################
####################################################################################################
####################################################################################################

plt.figure()
runtimesMutualSolved = [[resultDict[s]["totalRuntime"] for s in solvedSamples['mutualNoVanilla'] if (s in resultDict) and ('vanilla' not in resultDict['label'].lower()) and (resultDict[s]["result"] in ["SAT", "UNSAT"])] for resultDict in resultDicts]
[result.sort() for result in runtimesMutualSolved]
sumRuntimes = [[sum(result[:i+1]) for i in range(len(result))] for result in runtimesMutualSolved]
lines = []
for sums, label, marker, color in zip(sumRuntimes, cactusLabels, cactusMarkers, cactusColors):
    if 'vanilla' in label.lower():
        continue
    solved = [0] + list(range(1,len(sums)+1))
    sums = [0] + sums
    dumpJson({'x': sums, 'y': solved}, 'CactusTotalRuntime_pltstep_InstancesSolvedByAll_{}'.format(label))
    p = plt.step(sums, solved, label=label, where="post", c=color)
    plt.scatter(sums[-1], solved[-1], s=70, marker=marker, alpha=1, facecolor='none', edgecolor=color)
    lines.append(mlines.Line2D([], [], markerfacecolor='none', markeredgecolor=color, marker=marker, markersize=10, label=label, color=color))
plt.xlabel("Total Runtime [sec]")
plt.ylabel("Instances solved")
plt.legend(handles=lines)
setFigSize()
plt.grid(True)
plt.savefig("CactusTotal_InstancesSolvedByAll.png", bbox_inches="tight", dpi=100)
plt.close()

####################################################################################################
####################################################################################################
####################################################################################################

#cactusLabels = [resultDict['label'] + ' [sec]' for resultDict in resultDicts]
plt.figure()
runtimesSuccessfulSolved = [[resultDict[s]["successfulRuntime"] for s in xparamTotal if (s in resultDict) and (resultDict[s]["result"] in ["SAT", "UNSAT"])] for resultDict in resultDicts]
[result.sort() for result in runtimesSuccessfulSolved]
sumRuntimes = [[sum(result[:i+1]) for i in range(len(result))] for result in runtimesSuccessfulSolved]
#[plt.plot(sums, list(range(1,len(sums)+1)), label=label) for sums, label in zip(sumRuntimes, cactusLabels)]
lines = []
for sums, label, marker, color in zip(sumRuntimes, cactusLabels, cactusMarkers, cactusColors):
    solved = [0] + list(range(1,len(sums)+1))
    sums = [0] + sums
    p = plt.step(sums, solved, label=label, where="post", c=color)
    #plt.scatter(sums[-1], solved[-1], s=70, marker=marker, alpha=0.3, c=color)
    plt.scatter(sums[-1], solved[-1], s=70, marker=marker, alpha=1, facecolor='none', edgecolor=color)
    lines.append(mlines.Line2D([], [], markerfacecolor='none', markeredgecolor=color, marker=marker, markersize=10, label=label, color=color))
    #print("label={}".format(label))
    #print("sums={}".format(sums))
    #print("solved={}".format(solved))        
plt.xlabel("Successful Runtime [sec]")
plt.ylabel("Instances solved")
plt.legend(handles=lines)
setFigSize()
plt.grid(True)
plt.savefig("CactusSuccessful.png", bbox_inches="tight", dpi=100)
plt.close()
            
####################################################################################################
####################################################################################################
####################################################################################################
plt.figure()

# Pie chart, where the slices will be ordered and plotted counter-clockwise:
resultLabels = 'SAT', 'UNSAT', 'TIMEOUT', "GTIMEOUT", "SPURIOUS", "ERROR"
runSampleNum = [[len([v for k,v in resultDict.items() if k != 'label' and v["result"] == l]) for l in resultLabels] for resultDict in resultDicts]

fig, axes = plt.subplots(nrows=len(resultDicts), ncols=1)
fig.suptitle("Run Results", fontsize=20)
plt.subplots_adjust(wspace=1)
for resultDict, ax , sampleNum in zip(resultDicts, axes, runSampleNum): 
    ax.pie(sampleNum, labels=[l if s > 0 else "" for l,s in zip(resultLabels, sampleNum)], colors=["darkred","darkgreen", "darkorange", "gold", "deeppink", "lightgray"], autopct=lambda pct: pctFunc(pct, sampleNum), shadow=True, startangle=90, textprops=dict(color="k"))
    ax.set_title("{}, {} samples".format(resultDict['label'], len(resultDict)-1), fontdict=dict(fontsize=12))
    ax.axis('equal')

    setFigSize()
plt.grid(True)    
plt.savefig("ResultPie.png", dpi=100)
plt.close()

####################################################################################################
####################################################################################################
####################################################################################################

for (x,y) in comparePropertiesPairs:
    plotCompareProperties(results[x], results[y])
    
vanillaKey = [x for x in results.keys() if x.lower().startswith("vanilla")][0]
randomKey = [x for x in results.keys() if x.lower().startswith("random")][0]
otherKeys = [key for key in results.keys() if (key != vanillaKey) and (key != randomKey)]
fig, ax = plt.subplots(1)
for i, otherPolicy in enumerate(otherKeys):
    plotCompareProperties(results[vanillaKey], results[otherPolicy], newFig=(i==0), singleFig=False, lastFig=(i==len(otherKeys)-1), ax=ax)

for k in COIRatioKeys:
    plotCOIRatio(results[k])

with open("originalPath", mode='w') as f:
    f.write(origDir)

with open("commonRunCommand.log", mode='w') as f:
    f.write("python3 cnn_abs_testbench.py " + commonRunCommand)

with open("runCommands.log", mode='w') as f:
    for cmd in runCommands:
        f.write("python3 cnn_abs_testbench.py " + cmd)
