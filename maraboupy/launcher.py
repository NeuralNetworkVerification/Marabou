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
    return ['Centered', 'AllSamplesRank', 'SingleClassRank', 'MajorityClassVote', 'Random', 'SampleRank']
    
def solvingPolicies():
    return absPolicies() + ['Vanilla']

def globalTimeOut():
    return 1,0,0

TIMEOUT_H, TIMEOUT_M, TIMEOUT_S = globalTimeOut()
gtimeout = TIMEOUT_H * 3600 + TIMEOUT_M * 60 + TIMEOUT_S
TIME_LIMIT = "{}:{:02d}:{:02d}".format(TIMEOUT_H, TIMEOUT_M + 2, TIMEOUT_S)

####################################################################################################
####################################################################################################
####################################################################################################

def experimentAbsPolicies(numRunsPerType, commonFlags, batchDirPath, slurm=False):
    runCmds = list()
    runTitles = list()
    title2Label = dict()

    for policy in solvingPolicies():
        title2Label["{}Cfg".format(policy)] = "{}".format(policy)
        for i in range(numRunsPerType):
            title = "{}Cfg---{}".format(policy, i)
            cmd = commonFlags + ["--policy", policy, "--sample", str(i)]
            if slurm:
                cmd += ["--run_title", title]
            runCmds.append(cmd)
            runTitles.append(title)

    if slurm:
        with open(batchDirPath + "/plotSpec.json", 'w') as f:
            policiesCfg = ["{}Cfg".format(policy) for policy in absPolicies()]
            jsonDict = {"Experiment"  : "CNN Abstraction Vs. Vanilla Marabou",
                        "TIMEOUT_VAL" : gtimeout,
                        "title2Label" : title2Label,
                        "COIRatio"    : policiesCfg,
                        "compareProperties": list(itertools.combinations(policiesCfg, 2)) + [('VanillaCfg', policy) for policy in policiesCfg],
                        "commonRunCommand" : " ".join(commonFlags),
                        "runCommands"  : [" ".join(cmd) for cmd in runCmds],
                        "xparameter" : "cfg_sampleIndex"}
            json.dump(jsonDict, f, indent = 4)

    return runCmds, runTitles

def experimentDifferentDistances(numRunsPerType, commonFlags, batchDirPath, slurm=False):
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

    if slurm:
        with open(batchDirPath + "/plotSpec.json", 'w') as f:
            policiesCfg = ["{}Cfg".format(policy) for policy in absPolicies()]
            jsonDict = {"Experiment"  : "Different property distances on the same sample",
                        "TIMEOUT_VAL" : gtimeout,
                        "title2Label" : title2Label,
                        "COIRatio"    : policiesCfg,
                        "compareProperties": list(itertools.combinations(policiesCfg, 2)) + [('VanillaCfg', policy) for policy in policiesCfg],
                        "commonRunCommand" : " ".join(commonFlags),
                        "runCommands"  : [" ".join(cmd) for cmd in runCmds],
                        "xparameter" : "cfg_propDist"}
            json.dump(jsonDict, f, indent = 4)

    return runCmds, runTitles

def runSingleRun(cmd, title, basePath, batchDirPath, pyFilePath):

    CPUS = 8
    MEM_PER_CPU = "8G"
    
    runDirPath = batchDirPath + "/" + title
    os.makedirs(runDirPath, exist_ok=True)
    os.chdir(runDirPath)

    sbatchCode = list()
    sbatchCode.append("#!/bin/bash")
    sbatchCode.append("#SBATCH --job-name={}".format(title))
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
    sbatchCode.append("")
    sbatchCode.append('echo "Ive been launched" > {}/Started'.format(runDirPath))        
    sbatchCode.append("stdbuf -o0 -e0 python3 {} {}".format(pyFilePath, " ".join(cmd)))
    sbatchCode.append("")
    sbatchCode.append("date")

    sbatchFile = runDirPath + "/" + "cnnAbsRun-{}sbatch".format(title)
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
    
    experiments = {"AbsPolicies"    : experimentAbsPolicies,
                   "DifferentDistances" : experimentDifferentDistances}
    parser = argparse.ArgumentParser(description='Launch Sbatch experiments')
    parser.add_argument("--exp",           type=str, default="AbsPolicies", choices=list(experiments.keys()), help="Which experiment to launch?", required=False)
    parser.add_argument("--runs_per_type", type=int, default=100, help="Number of runs per type.")
    parser.add_argument("--sample",        type=int, default=0, help="For part of experiments, specific sample choice")
    parser.add_argument("--net",           type=str, help="Network to verify", required=True)
    parser.add_argument("--pyFile",           type=str, help="Python script path", required=True)    
    parser.add_argument("--prop_distance", type=float, default=0.03,                    help="Distance checked for adversarial robustness (L1 metric)") 
    parser.add_argument('--slurm'   , dest='slurm', action='store_true',  help="Launch Cmds in Slurm")
    args = parser.parse_args()
    experiment = args.exp
    numRunsPerType = args.runs_per_type
    experimentFunc = experiments[experiment]
    net = args.net
    prop_distance = args.prop_distance
    pyFilePath = os.path.abspath(args.pyFile)
    slurm = args.slurm
    
    ####################################################################################################
    
    timestamp = datetime.now()
    batchId = "_".join(filter(None, [experiment, net.split('/')[-1].replace('.h5',''), timestamp.strftime("%d-%m-%y"), timestamp.strftime("%H-%M-%S")]))
    basePath = os.getcwd() + "/"
    batchDirPath = basePath + "logs/" + batchId
    if slurm:
        os.makedirs(batchDirPath, exist_ok=True)        
        with open(batchDirPath + "/runCmd.sh", 'w') as f:
            f.write(" ".join(["python3"]+ sys.argv) + "\n")
        
    commonFlags = ["--gtimeout", str(gtimeout)]
    if slurm:
        commonFlags += ["--batch_id", batchId]
    if experiment != 'DifferentDistances':
        commonFlags += ["--prop_distance", str(prop_distance)]
    else:
        commonFlags += ["--sample", str(args.sample)]
    commonFlags += ["--net", net]
        
    runCmds, runTitles = experimentFunc(numRunsPerType, commonFlags, batchDirPath, slurm=slurm)
    
    sbatchFiles = list()
    cmdJson = list()
    for cmd, title in zip(runCmds, runTitles):
        if slurm:
            runSingleRun(cmd, title, basePath, batchDirPath, pyFilePath)
        cmdJson.append("python3 {} {}".format(pyFilePath, " ".join(cmd)))
    with open("launcherCmdList.json", 'w') as f:
        json.dump(cmdJson, f, indent=4)
                
if __name__ == "__main__":
    main()    
