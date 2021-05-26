
import os
import sys
import subprocess
from datetime import datetime
import json
import itertools
import argparse
from cnn_abs import *
tf.compat.v1.enable_v2_behavior()

####################################################################################################
####################################################################################################
####################################################################################################

def globalTimeOut():
    return 4,0,0

def experimentCNNAbsVsVanilla(numRunsPerType, commonFlags, batchDirPath):
    TIMEOUT_H, TIMEOUT_M, TIMEOUT_S = globalTimeOut()
    
    runCmds = list()
    runTitles = list()
    runBriefs = list()    

    for i in range(numRunsPerType):
        title = "MaskCOICfg---{}".format(i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i)])
        runTitles.append(title)
        runBriefs.append("Run with CNN improvments")
        
    for i in range(numRunsPerType):
        title = "VanillaCfg---{}".format(i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i), "--no_coi", "--no_mask"])
        runTitles.append(title)
        runBriefs.append('Run with default ("vanilla") Marabou')
        
    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        jsonDict = {"Experiment"  : "CNN Abstraction Vs. Vanilla Marabou",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : {'MaskCOICfg' : 'CNN Abstraction', 'VanillaCfg' : 'Vanilla Marabou'},
                    "COIRatio"    : ['MaskCOICfg'],
                    "compareProperties": [('VanillaCfg', 'MaskCOICfg')]}
        json.dump(jsonDict, f, indent = 4)

    TIME_LIMIT = "{}:{:02d}:{:02d}".format(TIMEOUT_H, TIMEOUT_M, TIMEOUT_S)

    return runCmds, runTitles, runBriefs, TIME_LIMIT

####################################################################################################
####################################################################################################
####################################################################################################

def experimentAbsPolicies(numRunsPerType, commonFlags, batchDirPath):

    TIMEOUT_H, TIMEOUT_M, TIMEOUT_S = globalTimeOut()
    
    runCmds = list()
    runTitles = list()
    runBriefs = list()
    title2Label = dict()

    for policy in mnistProp.absPolicies:
        #title2Label["{}Cfg".format(policy)] = "Abstraction Policy - {}".format(policy)
        title2Label["{}Cfg".format(policy)] = "{}".format(policy)
        for i in range(numRunsPerType):
            title = "{}Cfg---{}".format(policy, i)
            runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i), "--policy", policy])
            runTitles.append(title)
            runBriefs.append("Run with abstraction policy {}.".format(policy))

    title2Label["VanillaCfg"] = 'Vanilla Marabou'
    for i in range(numRunsPerType):
        title = "VanillaCfg---{}".format(i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i), "--no_coi", "--no_mask"])
        runTitles.append(title)
        runBriefs.append('Run with default ("vanilla") Marabou')

    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        policiesCfg = ["{}Cfg".format(policy) for policy in mnistProp.absPolicies]
        jsonDict = {"Experiment"  : "CNN Abstraction Vs. Vanilla Marabou",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : title2Label,
                    "COIRatio"    : policiesCfg,
                    "compareProperties": list(itertools.combinations(policiesCfg, 2)) + [('VanillaCfg', policy) for policy in policiesCfg],
                    "commonRunCommand" : " ".join(commonFlags),
                    "runCommand"  : " ".join(runCmds)}
        json.dump(jsonDict, f, indent = 4)

    TIME_LIMIT = "{}:{:02d}:{:02d}".format(TIMEOUT_H, TIMEOUT_M, TIMEOUT_S)

    return runCmds, runTitles, runBriefs, TIME_LIMIT

def experimentFindMinProvable(numRunsPerType, commonFlags, batchDirPath):
    TIMEOUT_H, TIMEOUT_M, TIMEOUT_S = globalTimeOut()
    
    runCmds = list()
    runTitles = list()
    runBriefs = list()
    title2Label = dict()

    policy = "FindMinProvable"
    sample = 1
    title2Label["{}Cfg".format(policy)] = "experiment - {}".format(policy)
    for i in range(numRunsPerType):
        title = "{}Cfg---{}".format(policy, i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(sample), "--policy", policy, "--no_full"])
        runTitles.append(title)
        runBriefs.append("Run with abstraction policy {}.".format(policy))

    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        policiesCfg = ["{}Cfg".format(policy)]
        jsonDict = {"Experiment"  : "Find Maxmimal Removable Set of Neurons",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : title2Label,
                    "COIRatio"    : policiesCfg,
                    "compareProperties": list()}
        json.dump(jsonDict, f, indent = 4)

    TIME_LIMIT = "{}:{:02d}:{:02d}".format(TIMEOUT_H, TIMEOUT_M, TIMEOUT_S)

    return runCmds, runTitles, runBriefs, TIME_LIMIT


####################################################################################################
####################################################################################################
####################################################################################################

validationNets = ["mnist_{}_{}".format(layers, i) for i in [1, 2, 4] for layers in ["base", "long"]]

experiments = {"CNNAbsVsVanilla": experimentCNNAbsVsVanilla,
               "AbsPolicies"    : experimentAbsPolicies,
               "FindMinProvable": experimentFindMinProvable}
parser = argparse.ArgumentParser(description='Launch Sbatch experiments')
parser.add_argument("--exp", type=str, default="AbsPolicies", choices=list(experiments.keys()), help="Which experiment to launch?", required=True)
parser.add_argument("--runs_per_type", type=int, default=100, help="Number of runs per type.")
parser.add_argument("--dump_queries", action="store_true", default=False, help="Only dump queries, don't solve.")
parser.add_argument("--use_dumped_queries", action="store_true", default=False, help="Solve with dumped queries (abstraction only, not Vanilla)")
parser.add_argument("--sample", type=int, default=0, help="For part of experiments, specific sample choice")
parser.add_argument("--dump_suffix", type=str, default="", help="Suffix at ending the dumpQueries directory", required=False)
parser.add_argument("--validation", type=str, choices=validationNets, default="", help="Use validation net", required=False)
parser.add_argument("--cnn_size"  , type=str, default="", help="Use specific cnn size", required=False)
args = parser.parse_args()
experiment = args.exp
numRunsPerType = args.runs_per_type
experimentFunc = experiments[experiment]
dumpQueries = args.dump_queries
useDumpedQueries = args.use_dumped_queries
dumpSuffix = args.dump_suffix
validation = args.validation
cnn_size = args.cnn_size

####################################################################################################

batchId = "slurm_" + experiment + "_" + datetime.now().strftime("%d-%m-%y___%H-%M-%S")
basePath = "/cs/labs/guykatz/matanos/Marabou/maraboupy/"
if not os.path.exists(basePath + "logs/"):
    os.mkdir(basePath + "logs/")
batchDirPath = basePath + "logs/" + batchId
if not os.path.exists(batchDirPath):
    os.mkdir(batchDirPath)
dumpDirPath = basePath + "logs/dumpQueries{}/".format(("_" + dumpSuffix) if dumpSuffix else "")
if (dumpQueries or useDumpedQueries) and not os.path.exists(dumpDirPath):
    os.mkdir(dumpDirPath)
    
    
CPUS = 8
MEM_PER_CPU = "8G"
#commonFlags = ["--run_on", "cluster", "--batch_id", batchId, "--sporious_strict", "--num_cpu", str(CPUS), "--bound_tightening", "lp", "--symbolic", "deeppoly", "--prop_distance", str(0.02), "--timeout", str(1000)]
#clusterFlags = ["--run_on", "cluster", "--num_cpu", str(CPUS)]
clusterFlags = []
#commonFlags = clusterFlags + ["--batch_id", batchId, "--sporious_strict", "--bound_tightening", "lp", "--symbolic", "sbt", "--prop_distance", str(0.02), "--prop_slack", str(-0.1), "--timeout", str(1000), "--dump_dir", dumpDirPath]
commonFlags = clusterFlags + ["--batch_id", batchId, "--bound_tightening", "lp", "--symbolic", "sbt", "--prop_distance", str(0.03), "--prop_slack", str(0), "--timeout", str(1000), "--dump_dir", dumpDirPath]
if cnn_size:
    commonFlags += ["--cnn_size", cnn_size]
if validation:
    commonFlags += ["--validation", validation]
if dumpQueries:
    commonFlags.append("--dump_queries")
if useDumpedQueries:
    commonFlags.append("--use_dumped_queries")
    
runCmds, runTitles, runBriefs, TIME_LIMIT = experimentFunc(numRunsPerType, commonFlags, batchDirPath)

sbatchFiles = list()
for cmd, title, brief in zip(runCmds, runTitles, runBriefs):

    runDirPath = batchDirPath + "/" + title
    if not os.path.exists(runDirPath):
        os.mkdir(runDirPath)
    os.chdir(runDirPath)

    sbatchCode = list()
    sbatchCode.append("#!/bin/bash")
    sbatchCode.append("#SBATCH --job-name={}_{}".format(experiment, title))
    sbatchCode.append("#SBATCH --cpus-per-task={}".format(CPUS))
    sbatchCode.append("#SBATCH --mem-per-cpu={}".format(MEM_PER_CPU))
    sbatchCode.append("#SBATCH --output={}/cnnAbsTB_{}.out".format(runDirPath, title))
    sbatchCode.append("#SBATCH --partition=long")
    sbatchCode.append("#SBATCH --signal=B:SIGUSR1@300")
    sbatchCode.append("#SBATCH --time={}".format(TIME_LIMIT))
    sbatchCode.append("#SBATCH -C avx2")
    #sbatchCode.append("#SBATCH --reservation 5781")    
    sbatchCode.append("")
    sbatchCode.append("pwd; hostname; date")
    sbatchCode.append("")
    sbatchCode.append("csh /cs/labs/guykatz/matanos/py_env/bin/activate.csh")
    sbatchCode.append("export PYTHONPATH=$PYTHONPATH:/cs/labs/guykatz/matanos/Marabou")
    sbatchCode.append("export GUROBI_HOME=/cs/labs/guykatz/matanos/gurobi911/linux64")
    sbatchCode.append("export PATH=$PATH:${GUROBI_HOME}/bin")
    sbatchCode.append("export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib")
    sbatchCode.append("export GRB_LICENSE_FILE=/cs/share/etc/license/gurobi/gurobi.lic")
    sbatchCode.append("")
    sbatchCode.append("### Description of this specific run is : {}".format(brief))
    sbatchCode.append("")
    sbatchCode.append('echo "Ive been launched" > {}/Started'.format(runDirPath))        
    sbatchCode.append("stdbuf -o0 -e0 python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/cnn_abs_testbench.py {}".format(" ".join(cmd)))
    sbatchCode.append("")
    sbatchCode.append("date")

    sbatchFiles.append(runDirPath + "/" + "cnnAbsRun-{}.sbatch".format(title))
    with open(sbatchFiles[-1], "w") as f:
        for line in sbatchCode:
            f.write(line + "\n")

os.chdir(basePath)

for f in sbatchFiles:
    print("Running : {}".format(" ".join(["sbatch", f])))
    subprocess.run(["sbatch", f])
