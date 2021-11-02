import os
import subprocess
from CnnAbs import CnnAbs

def launchSlurm(saveDir, network, sample, distance):
    saveDir = os.path.abspath(saveDir)
    sbatchCode = list()
    sbatchCode.append('#!/bin/bash')
    sbatchCode.append('#SBATCH --job-name=DumpAll')
    sbatchCode.append('#SBATCH --cpus-per-task=1')
    sbatchCode.append('#SBATCH --mem-per-cpu=8G')
    sbatchCode.append('#SBATCH --output=/cs/labs/guykatz/matanos/Marabou/maraboupy/evaluation/network{}/{}/SbatchDumpAllBounds{}{}{}.out'.format(network, sample, network, sample, str(distance).replace('.','-')))
    sbatchCode.append('#SBATCH --partition=long')
    sbatchCode.append('#SBATCH --signal=B:SIGUSR1@300')
    sbatchCode.append('#SBATCH --time=1:00:00')
    sbatchCode.append('#SBATCH -C avx2')
    sbatchCode.append('')
    sbatchCode.append('pwd; hostname; date')
    sbatchCode.append('')
    sbatchCode.append('csh /cs/labs/guykatz/matanos/Marabou/../py_env/bin/activate.csh')
    sbatchCode.append('export PYTHONPATH=$PYTHONPATH:/cs/labs/guykatz/matanos/Marabou/maraboupy:/cs/labs/guykatz/matanos/Marabou')
    sbatchCode.append('export GUROBI_HOME=/cs/labs/guykatz/matanos/gurobi911/linux64')
    sbatchCode.append('export PATH=$PATH:${GUROBI_HOME}/bin')
    sbatchCode.append('export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${GUROBI_HOME}/lib')
    sbatchCode.append('export GRB_LICENSE_FILE=/cs/share/etc/license/gurobi/gurobi.lic')
    sbatchCode.append('')
    sbatchCode.append('')
    sbatchCode.append('echo "Ive been launched" > /cs/labs/guykatz/matanos/Marabou/maraboupy/evaluation/StartedDumpAllBounds')
    sbatchCode.append('stdbuf -o0 -e0 python3 /cs/labs/guykatz/matanos/Marabou/maraboupy/evaluation/dumpAllBounds.py --net network{} --sample {} --distance {}'.format(network, sample, distance))
    sbatchCode.append('')
    sbatchCode.append('date')

    os.makedirs(saveDir, exist_ok=True)
    sbatchFile = saveDir + '/' + 'dumpAllBounds{}{}{}.sbatch'.format(network, sample, str(distance).replace('.','-'))
    with open(sbatchFile, "w") as f:
        for line in sbatchCode:
            f.write(line + "\n")

    #cwd = os.getcwd()
    #os.chdir(saveDir)
    print("Running : {}".format(" ".join(["sbatch", sbatchFile])))
    subprocess.run(["sbatch", sbatchFile])
    #os.chdir(cwd)

for network in ['A', 'B', 'C']:
    for sample in range(100):
        for distance in [0.01, 0.02, 0.03]:
            saveDir = CnnAbs.dumpBoundsDir + '/network{}/{}'.format(network, sample)
            if not os.path.exists(saveDir + '/{}.json'.format(str(distance).replace('.','-'))):
                launchSlurm(saveDir, network, sample, distance)
            else:
                print('Skipping {}/{}.json'.format(saveDir, str(distance).replace('.','-')))
