
import os
import sys
import subprocess
from datetime import datetime

batchId = "slurm_" + datetime.now().strftime("%d-%m-%y___%H-%M-%S")
basePath = "/cs/labs/guykatz/matanos/Marabou/maraboupy/"
if not os.path.exists(basePath + "logs/"):
    os.mkdir(basePath + "logs/")
batchDirPath = basePath + "logs/" + batchId
if not os.path.exists(batchDirPath):
    os.mkdir(batchDirPath)
runCmds = list()
runTitles = list()
runBriefs = list()
    
CPUS = 8
MEM_PER_CPU = "1G"
TIME_LIMIT = "12:00:00"

commonFlags = ["--run_on", "cluster", "--batch_id", batchId, "--sporious_strict"]
numRunsPerType = 20

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

sbatchFiles = list()
for cmd, title, brief in zip(runCmds, runTitles, runBriefs):

    runDirPath = batchDirPath + "/" + title
    if not os.path.exists(runDirPath):
        os.mkdir(runDirPath)
    os.chdir(runDirPath)

    sbatchCode = list()
    sbatchCode.append("#!/bin/bash")
    sbatchCode.append("#SBATCH --job-name=cnnAbsTB_{}_{}".format(batchId, title))
    sbatchCode.append("#SBATCH --cpus-per-task={}".format(CPUS))
    sbatchCode.append("#SBATCH --mem-per-cpu={}".format(MEM_PER_CPU))
    sbatchCode.append("#SBATCH --output={}/cnnAbsTB_{}.out".format(runDirPath, title))
    sbatchCode.append("#SBATCH --partition=long")
    sbatchCode.append("#SBATCH --signal=B:SIGUSR1@300")
    sbatchCode.append("#SBATCH --time={}".format(TIME_LIMIT))
    sbatchCode.append("#SBATCH --reservation 5781")    
    sbatchCode.append("")
    sbatchCode.append("pwd; hostname; date")
    sbatchCode.append("")
    sbatchCode.append("csh /cs/labs/guykatz/matanos/py_env/bin/activate.csh")
    sbatchCode.append("export PYTHONPATH=$PYTHONPATH:/cs/labs/guykatz/matanos/Marabou")
    sbatchCode.append("export GUROBI_HOME=/cs/labs/guykatz/matanos/gurobi900/linux64")
    sbatchCode.append("export PATH=$PATH:${GUROBI_HOME}/bin")
    sbatchCode.append("export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib")
    sbatchCode.append("export GRB_LICENSE_FILE=/cs/share/etc/license/gurobi/gurobi.lic")
    sbatchCode.append("")
    sbatchCode.append("### Description of this specific run is : {}".format(brief))
    sbatchCode.append("")    
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
