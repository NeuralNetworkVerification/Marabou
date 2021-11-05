import os
import json
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.lines as mlines
import statistics
import argparse

font = {'size'   : 24}
coparePoliciesMarkerSize = 300
cactiMarkerSize = 1300
legendMarkerSize = 20

matplotlib.rc('font', **font)

# Save json with data as name.
def dumpJson(data, name, dumpDir=''):
    if dumpDir and not dumpDir.endswith('/'):
        dumpDir += '/'
    os.makedirs(os.path.abspath(dumpDir), exist_ok=True)
    if not name.endswith(".json"):
        name += ".json"
    with open(dumpDir + name, "w") as f:
        json.dump(data, f, indent = 4)

# Load json named name.
def loadJson(name, loadDir=''):
    if loadDir and not loadDir.endswith('/'):
        loadDir += '/'
    if not name.endswith(".json"):
        name += ".json"
    with open(os.path.abspath(loadDir) + name, "r") as f:
        return json.load(f)

# Set figure graph.
def setFigSize(w=12, h=9):
    figure = plt.gcf()
    figure.set_size_inches(w, h)

# plot Figures.
def cactusTotal(queries, policy, loadDir, dumpDir, gtimeout=3600):
    plt.figure()
    plt.grid(True)    
    lines = []
    #markers = ['*', 's', 'o', 'P', 'X', '^', 'p', 'D', '+']
    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:brown', 'navy', 'tab:cyan', 'tab:olive']
    markers = lambda net, dist, ab: '${}{}({})$'.format(net, str(dist)[-1],ab)
    #markers = {'A':'$A$',  'B': '$B$', 'C': '$C$'}
    #for (query, net, dist), color, marker in zip(queries, colors, markers):
    avgVanilla = []
    avgPolicy = []
    medianVanilla = []
    medianPolicy = []
    numSolvedVanilla = []
    numSolvedPolicy = []
    compareProp = []
    for i, (query, net, dist) in enumerate(queries):
        color = colors[i]
        marker = markers(net, dist, 'CA')
        label = '{} {}'.format(net, dist)
        graph = loadJson('/' + query + '/CactusTotalRuntime_pltstep_{}.json'.format(policy), loadDir=loadDir)
        plt.step(graph['x'], graph['y'], label=label, where='post', color=color, linewidth=2)
        #lines.append(mlines.Line2D([], [], markerfacecolor=color, markeredgecolor='black', marker=marker, markersize=legendMarkerSize, label=label, color=color))
        lines.append(mlines.Line2D([], [], label=label, color=color, linewidth=2))
        
        if graph['y'][-1] > 0:
            avgPolicy.append(round(graph['x'][-1] / graph['y'][-1]))
        numSolvedPolicy.append(graph['y'][-1])
        if len(graph['x']) > 1:
            medianPolicy.append(round(statistics.median([b - a for a,b in zip(graph['x'][:-1], graph['x'][1:])])))
        else:
            medianPolicy.append(-1)

        marker = markers(net, dist, 'VM')        
        label = 'net {} {}'.format(dist, 'Vanilla')
        graph = loadJson('/' + query + '/CactusTotalRuntime_pltstep_{}.json'.format('Vanilla'), loadDir=loadDir)
        plt.step(graph['x'], graph['y'], label=label, where='post', color=color, linestyle='-.', linewidth=2)

        if graph['y'][-1] > 0:
            avgVanilla.append(round(graph['x'][-1] / graph['y'][-1]))
        numSolvedVanilla.append(graph['y'][-1])
        if len(graph['x']) > 1:        
            medianVanilla.append(round(statistics.median([b - a for a,b in zip(graph['x'][:-1], graph['x'][1:])])))
        else:
            medianVanilla.append(-1)

        compareProp.append(loadJson('/' + query + '/CompareProperties-Vanilla_vs_{}'.format(policy), loadDir=loadDir))

    for i, (query, net, dist) in enumerate(queries):        
        graph = loadJson('/' + query + '/CactusTotalRuntime_pltstep_{}.json'.format(policy), loadDir=loadDir)
        marker = markers(net, dist, 'CA')
        plt.scatter(graph['x'][-1], graph['y'][-1], s=coparePoliciesMarkerSize, marker='o', alpha=0.4, facecolor=colors[i], edgecolor='black')
        #plt.scatter(graph['x'][-1], graph['y'][-1], s=cactiMarkerSize, marker=marker, alpha=1, facecolor='black', edgecolor='black')

        graph = loadJson('/' + query + '/CactusTotalRuntime_pltstep_{}.json'.format('Vanilla'), loadDir=loadDir) 
        marker = markers(net, dist, 'VM')
        plt.scatter(graph['x'][-1], graph['y'][-1], s=coparePoliciesMarkerSize, marker='d', alpha=0.4, facecolor=colors[i], edgecolor='black')        
        #plt.scatter(graph['x'][-1], graph['y'][-1], s=cactiMarkerSize, marker=marker, alpha=1, facecolor='black', edgecolor='black')
        
    lines.append(mlines.Line2D([], [], label='Vanilla', color='black', linestyle='-.', marker='d', markersize=legendMarkerSize, linewidth=2))
    lines.append(mlines.Line2D([], [], label='CNN-ABS', color='black', linestyle='-', marker='o', markersize=legendMarkerSize, linewidth=2))    

    jsonDict = dict()
    jsonDict['avgPolicy'] = {query[0] : avg for query,avg in zip(queries, avgPolicy)}
    jsonDict['numSolvedPolicy'] = {query[0] : numSolved for query, numSolved in zip(queries, numSolvedPolicy)}
    jsonDict['medianPolicy'] = {query[0] : median for query,median in zip(queries, medianPolicy)}
    jsonDict['avgVanilla'] = {query[0] : avg for query,avg in zip(queries, avgVanilla)}
    jsonDict['numSolvedVanilla'] = {query[0] : numSolved for query, numSolved in zip(queries, numSolvedVanilla)}
    jsonDict['medianVanilla'] = {query[0] : median for query,median in zip(queries, medianVanilla)}    
    #dumpJson(jsonDict, 'CompareVanilla_statistics', dumpDir=dumpDir)
    dumpJson(jsonDict, 'Fig14', dumpDir=dumpDir)
    plt.xlabel("Total Runtime [sec]")
    plt.ylabel("Instances Solved")
    plt.legend(handles=lines, loc='upper left', ncol=4, bbox_to_anchor=(-0.17,1.3))
    setFigSize()
    #plt.savefig(dumpDir + "CactusTotal.png", bbox_inches="tight", dpi=100)
    plt.savefig(dumpDir + "Fig7.png", bbox_inches="tight", dpi=100)
    plt.close()

    plt.figure()
    dictTimeout = {cnt : 0 for cnt in ['totalSat', 'totalUnsat', 'timeoutPolicySatVanilla', 'timeoutPolicyUnsatVanilla', 'timeoutVanillaSatPolicy', 'timeoutVanillaUnsatPolicy']}
    for graph in compareProp:
        x, y, marker, color = graph.values()
        print(x, y, marker, color)
        for _x, _y, _m, _c in zip(x, y, marker, color):
            if _x > gtimeout:
                _x = gtimeout
            if _y > gtimeout:
                _y = gtimeout                
            if _m == 'X':
                dictTimeout['totalSat'] += 1
            if _m == 'o':
                dictTimeout['totalUnsat'] += 1
            if _m == '*':
                continue
            elif _m == 'X' and _c != 'red':
                _m = '*'
                _c = 'red'
                assert (_x == gtimeout or _y == gtimeout) and _y != _x
                if _x == gtimeout:
                    dictTimeout['timeoutVanillaSatPolicy'] += 1
                else:
                    dictTimeout['timeoutPolicySatVanilla'] += 1
            elif _m == 'o' and _c != 'green':
                _m = 's'
                _c = 'green'
                assert (_x == gtimeout or _y == gtimeout) and _y != _x
                if _x == gtimeout:
                    dictTimeout['timeoutVanillaUnsatPolicy'] += 1
                else:
                    dictTimeout['timeoutPolicyUnsatVanilla'] += 1                
            plt.scatter(_x, _y, marker=_m, facecolor='none', edgecolor=_c, s=400)

    dumpJson(dictTimeout, 'Fig8CountResultTypes', dumpDir=dumpDir)
    bottom = 1
    top = gtimeout
    axisTop = 2 * gtimeout
    plt.plot([bottom, top], [bottom, top], color='orange', linestyle='-', linewidth=2)
    plt.plot([bottom, top], [top   , top], color='orange', linestyle=':', linewidth=2)
    plt.plot([top,    top], [bottom, top], color='orange', linestyle=':', linewidth=2)
    plt.xlim([bottom, axisTop])
    plt.ylim([bottom, axisTop])    
            
    plt.xscale('log')
    plt.yscale('log')    
    plt.xlabel("Vanilla Runtime [sec]")
    plt.ylabel("Single Class Runtime [sec]")
    plt.text(80,4000, 'TIMEOUT={}s'.format(gtimeout), color='orange')
    plt.text(4000, 5,'TIMEOUT={}s'.format(gtimeout), color='orange', rotation=270)
    lines = [mlines.Line2D([], [], markerfacecolor='none', markeredgecolor=c, marker=m, markersize=legendMarkerSize, label=l, color=c) for l,m,c in [('SAT-SAT','X', 'r'), ('SAT-TIMEOUT','*', 'r'), ('UNSAT-UNSAT','o', 'g'), ('UNSAT-TIMEOUT','s', 'g')]]#, ('TIMEOUT-TIMEOUT','d', 'orange')]]
    plt.legend(handles=lines)  
    setFigSize()
    plt.grid(True)
    #plt.savefig(dumpDir + "CompareProperties.png", bbox_inches="tight", dpi=100)
    plt.savefig(dumpDir + "Fig8.png", bbox_inches="tight", dpi=100)
    plt.close()

# Plot policy comparison figures.
def comparePolicies(loadDir, dumpDir):
    policies = ['AllSamplesRank', 'Centered', 'MajorityClassVote', 'Random', 'SampleRank', 'SingleClassRank']
    policyLabel = {'AllSamplesRank': 'All Samples', 'Centered':'Centered', 'MajorityClassVote':'Majority Class Vote', 'Random':'Random', 'SampleRank':'Sample Rank', 'SingleClassRank':'Single Class'}
    lines = []
    plt.figure()
    markers = ['*', 's', 'o', 'P', 'X', '^', 'p', 'D'][:len(policies)]
    colors = ['tab:blue', 'tab:orange', 'tab:green', 'tab:red', 'tab:purple', 'tab:brown', 'tab:pink', 'tab:cyan', 'tab:olive'][:len(policies)]
    avgRuntime = []
    numSolved = []
    medians = []
    for policy, color, marker in zip(policies, colors, markers):
        graph = loadJson('/ComparePolicies/CactusTotalRuntime_pltstep_{}.json'.format(policy), loadDir=loadDir)
        plt.step(graph['x'], graph['y'], label=policyLabel[policy], where='post', color=color)
        plt.scatter(graph['x'][-1], graph['y'][-1], s=coparePoliciesMarkerSize, marker=marker, alpha=1, facecolor='none', edgecolor=color, linewidth=4)
        lines.append(mlines.Line2D([], [], markerfacecolor='none', markeredgecolor=color, marker=marker, markersize=legendMarkerSize, label=policyLabel[policy], color=color))
        if graph['y'][-1] > 0:
            avgRuntime.append(round(graph['x'][-1] / graph['y'][-1]))
        numSolved.append(graph['y'][-1])
        if len(graph['x']) > 1:
            medians.append(round(statistics.median([b - a for a,b in zip(graph['x'][:-1], graph['x'][1:])])))
        else:
            medians.append(-1)
    jsonDict = dict()
    jsonDict['avgRuntime'] = {policy : avg for policy,avg in zip(policies, avgRuntime)}
    jsonDict['solvedInstances'] = {policy : solved for policy,solved in zip(policies, numSolved)}
    jsonDict['medianRuntime'] = {policy : median for policy,median in zip(policies, medians)}
    #dumpJson(jsonDict, 'ComparePolicies_statistics', dumpDir=dumpDir)
    dumpJson(jsonDict, 'Fig13', dumpDir=dumpDir)
    plt.xlabel("Total Runtime [sec]")
    plt.ylabel("Instances Solved")
    plt.legend(handles=lines)
    setFigSize()
    plt.grid(True)
    ylim = plt.ylim()
    #plt.savefig(dumpDir + "CactusTotal_ComparePolicies.png", bbox_inches="tight", dpi=100)
    plt.savefig(dumpDir + "Fig6a.png", bbox_inches="tight", dpi=100)
    plt.close()

    for policy, color, marker in zip(policies, colors, markers):
        graph = loadJson('/ComparePolicies/CactusTotalRuntime_pltstep_InstancesSolvedByAll_{}.json'.format(policy), loadDir=loadDir)
        plt.step(graph['x'], graph['y'], label=policyLabel[policy], where='post', color=color)
        plt.scatter(graph['x'][-1], graph['y'][-1], s=coparePoliciesMarkerSize, marker=marker, alpha=1, facecolor='none', edgecolor=color, linewidth=4)
    plt.xlabel("Total Runtime [sec]")
    plt.ylabel("Instances Solved")
    plt.legend(handles=lines)
    setFigSize()
    plt.grid(True)
    plt.ylim(ylim)
    plt.xticks(plt.xticks()[0][1:-2])
    #plt.savefig(dumpDir + "CactusTotal_ComparePolicies_InstancesSolvedByAll.png", bbox_inches="tight", dpi=100)
    plt.savefig(dumpDir + "Fig6b.png", bbox_inches="tight", dpi=100)
    plt.close()    

def histUtil(queries, policy, name, key, density=True):
    
    valueHistogram = dict() #value -> repeats    
    for i, (query, net, dist) in enumerate(queries):
        graph = loadJson('/' + query + '/{}-{}.json'.format(name, policy), loadDir=loadDir)
        for value, repeat in zip(graph[key], graph['repeats']):
            valueHistogram[value] = repeat + (valueHistogram[value] if value in valueHistogram else 0)
    pairs = sorted(valueHistogram.items())
    values  = [p[0] for p in pairs]    
    repeats = [p[1] for p in pairs]
    repeats = [float(r) / float(sum(repeats)) for r in repeats]
    return values, repeats

def histUtilNumRuns(queries, policy, name):
    classifyResult = {k : 0 for k in ['LP UNSAT', 'Full Abstraction', 'Partial Abstraction', 'Full Network']}
    for i, (query, net, dist) in enumerate(queries):
        graph = loadJson('/' + query + '/{}-{}.json'.format(name, policy), loadDir=loadDir)
        for value, repeat in zip(graph['numRuns'], graph['repeats']):
            full = graph["totalAbsRefineBatches"]
            if value == 0:
                classifyResult['LP UNSAT'] += repeat
            elif value == 1:
                classifyResult['Full Abstraction'] += repeat
            elif value == full:
                classifyResult['Full Network'] += repeat
            else:
                classifyResult['Partial Abstraction'] += repeat                
    return classifyResult

def histograms(queries, policy, loadDir, dumpDir):
    lines = []
    totalColor = 'tab:cyan'
    satColor   = 'tab:red'
    unsatColor = 'tab:green'
    lines.append(mlines.Line2D([], [], markerfacecolor='none', color=unsatColor , markeredgecolor=unsatColor , markersize=legendMarkerSize, markeredgewidth=1, linestyle=':', linewidth=2, label='UNSAT', marker='o'))
    lines.append(mlines.Line2D([], [], markerfacecolor='none', color=satColor   , markeredgecolor=satColor   , markersize=legendMarkerSize, markeredgewidth=1, linestyle=':', linewidth=2, label='SAT'  , marker='X'))
    lines.append(mlines.Line2D([], [], markerfacecolor='none', color=totalColor, markeredgecolor=totalColor, markersize=legendMarkerSize, markeredgewidth=1, linestyle='-', linewidth=2, label='Total', marker='*'))
    ratios, repeats = histUtil(queries, policy, 'VariableRatioHistogram', 'ratio')
    ratiosSAT, repeatsSAT = histUtil(queries, policy, 'VariableRatioHistogramSAT', 'ratio')
    ratiosUNSAT, repeatsUNSAT = histUtil(queries, policy, 'VariableRatioHistogramUNSAT', 'ratio')
    #assert sum(repeats) == (sum(repeatsSAT) + sum(repeatsUNSAT))
    assert sum(repeats) == 1
    plt.figure()
    plt.plot(ratios     , repeats     , marker='*', markerfacecolor='none', color=totalColor , markeredgecolor=totalColor, markersize=20, markeredgewidth=2, linestyle='-', linewidth=6)    
    plt.plot(ratiosUNSAT, repeatsUNSAT, marker='o', markerfacecolor='none', color=unsatColor  , markeredgecolor=unsatColor , markersize=10, markeredgewidth=2, linestyle=':', linewidth=6)
    plt.plot(ratiosSAT  , repeatsSAT  , marker='X', markerfacecolor='none', color=satColor    , markeredgecolor=satColor   , markersize=20, markeredgewidth=2, linestyle=':', linewidth=6)
    plt.legend(handles=lines)
    plt.xlabel("Abstract Net Neurons out of Original Num.")
    plt.ylabel("Fraction out of all relevant instances")
    setFigSize()
    plt.grid(True)
    #plt.savefig(dumpDir + "VariableRatioHistogram.png", bbox_inches="tight", dpi=100)
    plt.savefig(dumpDir + "Fig9.png", bbox_inches="tight", dpi=100)
    plt.close()

#    dumpJson(histUtilNumRuns(queries, policy, 'NumRunsHistogram'), 'numRunsClassification', dumpDir=dumpDir)
#    dumpJson(histUtilNumRuns(queries, policy, 'NumRunsHistogramSAT'), 'numRunsClassificationSAT', dumpDir=dumpDir)
#    dumpJson(histUtilNumRuns(queries, policy, 'NumRunsHistogramUNSAT'), 'numRunsClassificationUNSAT', dumpDir=dumpDir)
    
    dumpJson(histUtilNumRuns(queries, policy, 'NumRunsHistogram'), 'Fig15Total', dumpDir=dumpDir)
    dumpJson(histUtilNumRuns(queries, policy, 'NumRunsHistogramSAT'), 'Fig15SAT', dumpDir=dumpDir)
    dumpJson(histUtilNumRuns(queries, policy, 'NumRunsHistogramUNSAT'), 'Fig15UNSAT', dumpDir=dumpDir)

# Get Average results of runs.
def getAverageResult(queries, policy, loadDir, dumpDir):
    solvedPolicy = []
    solvedVanilla = []
    ratio = []
    solvedInstancesPolicy = 0
    solvedInstancesVanilla = 0
    for i, (query, net, dist) in enumerate(queries):
        solved = loadJson('/' + query + '/solvedSamples.json', loadDir=loadDir)
        resultsPolicy  = solved['totalRuntimes'][policy]
        resultsVanilla = solved['totalRuntimes']['Vanilla']
        solvedInstancesPolicy  += len(set(resultsPolicy.keys()))
        solvedInstancesVanilla  += len(set(resultsVanilla.keys()))        
        for sample in set(resultsVanilla.keys()) & set(resultsPolicy.keys()):
            ratio.append(float(resultsPolicy[sample]) / float(resultsVanilla[sample]))
            solvedPolicy.append(resultsPolicy[sample])
            solvedVanilla.append(resultsVanilla[sample])

    jsonDict = dict()
    jsonDict['avgRatio'] = statistics.mean(ratio) if ratio else ''
    jsonDict['medianRatio'] = statistics.median(ratio) if ratio else ''
    jsonDict['ratioSolvedInstances'] = solvedInstancesPolicy / solvedInstancesVanilla
    dumpJson(jsonDict, 'TotalStatistics', dumpDir=dumpDir)

parser = argparse.ArgumentParser(description='Create graphs for paper.')
parser.add_argument("--dataDir", type=str, default="", help="directory containing results data")
parser.add_argument("--outputDir", type=str, default="", help="directory to output paper graphs")
parser.add_argument("--onlyComparePolicies", dest='onlyComparePolicies', action='store_true' , help="Only plot graphs of policy comparison")
parser.add_argument("--onlyCnnAbsVsVanilla", dest='onlyCnnAbsVsVanilla', action='store_true' , help="Only plot graphs of CNNAbs Vs Vanilla")
parser.add_argument("--gtimeout", type=int, default=3600, help="GTIMEOUT value")
args = parser.parse_args()
    
queries = [('A_0-03', 'A', 0.03), ('A_0-02', 'A', 0.02), ('A_0-01', 'A', 0.01), ('B_0-03', 'B', 0.03), ('B_0-02', 'B', 0.02), ('B_0-01', 'B', 0.01), ('C_0-03', 'C', 0.03), ('C_0-02', 'C', 0.02), ('C_0-01', 'C', 0.01)]
policy = 'SingleClassRank'

dumpDir = args.outputDir if args.outputDir.endswith('/') or not args.outputDir else args.outputDir + '/'
loadDir = args.dataDir   if args.dataDir.endswith('/')   or not args.dataDir   else args.dataDir + '/'

if not args.onlyComparePolicies:
    cactusTotal(queries, policy, loadDir, dumpDir, gtimeout=args.gtimeout)
    histograms(queries, policy, loadDir, dumpDir)
    getAverageResult(queries, policy, loadDir, dumpDir)

if not args.onlyCnnAbsVsVanilla:
    comparePolicies(loadDir, dumpDir)
