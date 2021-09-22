import os
import sys
import subprocess
from datetime import datetime
import json
import itertools
import argparse
import numpy as np
#from CnnAbs import *
#tf.compat.v1.enable_v2_behavior()

####################################################################################################
####################################################################################################
####################################################################################################

def absPolicies():
    return ['Centered', 'AllClassRank', 'SingleClassRank', 'MajorityClassVote', 'Random']
    
def solvingPolicies():
    return absPolicies() + ['Vanilla']

def globalTimeOut():
    return 3,0,0

TIMEOUT_H, TIMEOUT_M, TIMEOUT_S = globalTimeOut()
TIME_LIMIT = "{}:{:02d}:{:02d}".format(TIMEOUT_H, TIMEOUT_M, TIMEOUT_S)

def experimentCNNAbsVsVanilla(numRunsPerType, commonFlags, batchDirPath):
    
    runCmds = list()
    runTitles = list()

    for i in range(numRunsPerType):
        title = "MaskCOICfg---{}".format(i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i)])
        runTitles.append(title)
        
    for i in range(numRunsPerType):
        title = "VanillaCfg---{}".format(i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i), "--no_coi", "--no_mask"])
        runTitles.append(title)
        
    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        jsonDict = {"Experiment"  : "CNN Abstraction Vs. Vanilla Marabou",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : {'MaskCOICfg' : 'CNN Abstraction', 'VanillaCfg' : 'Vanilla Marabou'},
                    "COIRatio"    : ['MaskCOICfg'],
                    "compareProperties": [('VanillaCfg', 'MaskCOICfg')]}
        json.dump(jsonDict, f, indent = 4)

    return runCmds, runTitles

####################################################################################################
####################################################################################################
####################################################################################################

def experimentAbsPolicies(numRunsPerType, commonFlags, batchDirPath):
    runCmds = list()
    runTitles = list()
    title2Label = dict()

    for policy in solvingPolicies():
        title2Label["{}Cfg".format(policy)] = "{}".format(policy)
        for i in range(numRunsPerType):
            title = "{}Cfg---{}".format(policy, i)
            runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i), "--policy", policy])
            runTitles.append(title)

#    title2Label["VanillaCfg"] = 'Vanilla Marabou'
#    for i in range(numRunsPerType):
#        title = "VanillaCfg---{}".format(i)
#        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(i), "--no_coi", "--no_mask"])
#        runTitles.append(title)

    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        policiesCfg = ["{}Cfg".format(policy) for policy in absPolicies()]
        jsonDict = {"Experiment"  : "CNN Abstraction Vs. Vanilla Marabou",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : title2Label,
                    "COIRatio"    : policiesCfg,
                    "compareProperties": list(itertools.combinations(policiesCfg, 2)) + [('VanillaCfg', policy) for policy in policiesCfg],
                    "commonRunCommand" : " ".join(commonFlags),
                    "runCommands"  : [" ".join(cmd) for cmd in runCmds],
                    "xparameter" : "cfg_sampleIndex"}
        json.dump(jsonDict, f, indent = 4)

    return runCmds, runTitles

def experimentDifferentDistances(numRunsPerType, commonFlags, batchDirPath):
    runCmds = list()
    runTitles = list()
    title2Label = dict()

    propDist = [round(x, 3) for x in np.linspace(0.025, 0.2, num=numRunsPerType)]
    

    for policy in solvingPolicies():
        title2Label["{}Cfg".format(policy)] = "{}".format(policy)
        for i in range(numRunsPerType):
            title = "{}Cfg---{}".format(policy, str(propDist[i]).replace('.','-'))
            runCmds.append(commonFlags + ["--run_title", title, "--prop_distance", str(propDist[i]), "--policy", policy])
            runTitles.append(title)

    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        policiesCfg = ["{}Cfg".format(policy) for policy in absPolicies()]
        jsonDict = {"Experiment"  : "Different property distances on the same sample",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : title2Label,
                    "COIRatio"    : policiesCfg,
                    "compareProperties": list(itertools.combinations(policiesCfg, 2)) + [('VanillaCfg', policy) for policy in policiesCfg],
                    "commonRunCommand" : " ".join(commonFlags),
                    "runCommands"  : [" ".join(cmd) for cmd in runCmds],
                    "xparameter" : "cfg_propDist"}
        json.dump(jsonDict, f, indent = 4)

    return runCmds, runTitles


def experimentFindMinProvable(numRunsPerType, commonFlags, batchDirPath):
    
    runCmds = list()
    runTitles = list()
    title2Label = dict()

    policy = "FindMinProvable"
    sample = 1
    title2Label["{}Cfg".format(policy)] = "experiment - {}".format(policy)
    for i in range(numRunsPerType):
        title = "{}Cfg---{}".format(policy, i)
        runCmds.append(commonFlags + ["--run_title", title, "--sample", str(sample), "--policy", policy, "--no_full"])
        runTitles.append(title)

    with open(batchDirPath + "/plotSpec.json", 'w') as f:
        policiesCfg = ["{}Cfg".format(policy)]
        jsonDict = {"Experiment"  : "Find Maxmimal Removable Set of Neurons",
                    "TIMEOUT_VAL" : TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S,
                    "title2Label" : title2Label,
                    "COIRatio"    : policiesCfg,
                    "compareProperties": list()}
        json.dump(jsonDict, f, indent = 4)

    return runCmds, runTitles

def runSingleRun(cmd, title, basePath, batchDirPath, maskIndex=""):

    CPUS = 8
    MEM_PER_CPU = "8G"
    
    runDirPath = batchDirPath + "/" + title
    if not os.path.exists(runDirPath):
        os.mkdir(runDirPath)
    os.chdir(runDirPath)

    sbatchCode = list()
    sbatchCode.append("#!/bin/bash")
    sbatchCode.append("#SBATCH --job-name={}".format(("Seq" + maskIndex if maskIndex else "") + title))
    sbatchCode.append("#SBATCH --cpus-per-task={}".format(CPUS))
    sbatchCode.append("#SBATCH --mem-per-cpu={}".format(MEM_PER_CPU))
    sbatchCode.append("#SBATCH --output={}/cnnAbsTB_{}{}.out".format(runDirPath, title, ("_" + maskIndex) if maskIndex else ""))
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
    sbatchCode.append("")
    sbatchCode.append('echo "Ive been launched" > {}/Started'.format(runDirPath))        
    sbatchCode.append("stdbuf -o0 -e0 python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/CnnAbsTB.py {}".format(" ".join(cmd)))
    sbatchCode.append("")
    sbatchCode.append("date")

    sbatchFile = runDirPath + "/" + "cnnAbsRun-{}{}.sbatch".format(title, ("_" + maskIndex) if maskIndex else "")
    with open(sbatchFile, "w") as f:
        for line in sbatchCode:
            f.write(line + "\n")

    os.chdir(basePath)            
    print("Running : {}".format(" ".join(["sbatch", sbatchFile])))
    subprocess.run(["sbatch", sbatchFile]) 

####################################################################################################
####################################################################################################
####################################################################################################

def main():

    validationNets = ["mnist_{}_{}".format(layers, i) for i in [1, 2, 4] for layers in ["base", "long"]]
    
    experiments = {"CNNAbsVsVanilla": experimentCNNAbsVsVanilla,
                   "AbsPolicies"    : experimentAbsPolicies,
                   "FindMinProvable": experimentFindMinProvable,
                   "DifferentDistances" : experimentDifferentDistances}
    parser = argparse.ArgumentParser(description='Launch Sbatch experiments')
    parser.add_argument("--exp", type=str, default="AbsPolicies", choices=list(experiments.keys()), help="Which experiment to launch?", required=False)
    parser.add_argument("--runs_per_type", type=int, default=100, help="Number of runs per type.")
    parser.add_argument("--dump_queries", action="store_true", default=False, help="Only dump queries, don't solve.")
    parser.add_argument("--use_dumped_queries", action="store_true", default=False, help="Solve with dumped queries (abstraction only, not Vanilla)")
    parser.add_argument("--sample", type=int, default=0, help="For part of experiments, specific sample choice")
    parser.add_argument("--dump_suffix", type=str, default="", help="Suffix at ending the dumpQueries directory", required=False)
    parser.add_argument("--validation", type=str, choices=validationNets, default="", help="Use validation net", required=False)
    parser.add_argument("--cnn_size"  , type=str, default="", help="Use specific cnn size", required=False)
    parser.add_argument("--slurm_seq", action="store_true",                        default=True,                  help="Run next mask if this one fails.")
    parser.add_argument("--rerun_sporious", action="store_true",                        default=True,                  help="Rerun Sporious CEX.")
    parser.add_argument("--prop_distance",  type=float,                                 default=0.03,                    help="Distance checked for adversarial robustness (L1 metric)")
    args = parser.parse_args()
    experiment = args.exp
    numRunsPerType = args.runs_per_type
    experimentFunc = experiments[experiment]
    dumpQueries = args.dump_queries
    useDumpedQueries = args.use_dumped_queries
    dumpSuffix = args.dump_suffix
    validation = args.validation
    cnn_size = args.cnn_size
    slurm_seq = args.slurm_seq
    rerun_sporious = args.rerun_sporious
    prop_distance = args.prop_distance
    
    assert (not validation) or (not cnn_size)
    
    ####################################################################################################
    
    timestamp = datetime.now()
    batchId = "_".join(filter(None, ["slurm", timestamp.strftime("%d-%m-%y"), experiment, cnn_size, validation, timestamp.strftime("%H-%M-%S")]))
    basePath = "/cs/labs/guykatz/matanos/Marabou/maraboupy/"
    if not os.path.exists(basePath + "logs/"):
        os.mkdir(basePath + "logs/")
    batchDirPath = basePath + "logs/" + batchId
    if not os.path.exists(batchDirPath):
        os.mkdir(batchDirPath)
    if not dumpSuffix.startswith("dumpQueries"):
        dumpDir = "dumpQueries" + (("_" + dumpSuffix) if dumpSuffix else "")
    else:
        dumpDir = dumpSuffix
    dumpDirPath = basePath + "logs/{}/".format(dumpDir)
    if (dumpQueries or useDumpedQueries) and not os.path.exists(dumpDirPath):
        os.mkdir(dumpDirPath)

    with open(batchDirPath + "/runCmd.sh", 'w') as f:
        f.write(" ".join(["python3"]+ sys.argv) + "\n")
        
    #commonFlags = ["--run_on", "cluster", "--batch_id", batchId, "--sporious_strict", "--num_cpu", str(CPUS), "--bound_tightening", "lp", "--symbolic", "deeppoly", "--prop_distance", str(0.02), "--timeout", str(1000)]
    #clusterFlags = ["--run_on", "cluster", "--num_cpu", str(CPUS)]
    clusterFlags = []
    #commonFlags = clusterFlags + ["--batch_id", batchId, "--sporious_strict", "--bound_tightening", "lp", "--symbolic", "sbt", "--prop_distance", str(0.02), "--prop_slack", str(-0.1), "--timeout", str(1000), "--dump_dir", dumpDirPath]
    commonFlags = clusterFlags + ["--batch_id", batchId]
    if dumpQueries or useDumpedQueries or dumpSuffix:
        commonFlags += ["--dump_dir", dumpDirPath]    
    if experiment != 'DifferentDistances':
        commonFlags += ["--prop_distance", str(prop_distance)]
    else:
        commonFlags += ["--sample", str(args.sample)]
    if cnn_size:
        commonFlags += ["--cnn_size", cnn_size]
    if validation:
        commonFlags += ["--validation", validation]
    if dumpQueries:
        commonFlags.append("--dump_queries")
    if useDumpedQueries:
        commonFlags.append("--use_dumped_queries")
    if slurm_seq:
        commonFlags += ["--slurm_seq", "--mask_index", str(0)]
    if rerun_sporious:
        commonFlags.append("--rerun_sporious")
        
        
    runCmds, runTitles = experimentFunc(numRunsPerType, commonFlags, batchDirPath)
    
    sbatchFiles = list()
    for cmd, title in zip(runCmds, runTitles):
        runSingleRun(cmd, title, basePath, batchDirPath, maskIndex=(str(0) if slurm_seq else ""))

        
if __name__ == "__main__":
    main()    
