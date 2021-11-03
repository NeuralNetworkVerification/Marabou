
import executeFromJson
import subprocess
import argparse
import os
from CnnAbs import CnnAbs

graphDir = 'ComparePolicies'
parser = argparse.ArgumentParser(description='Launch Policy Comparison')
parser.add_argument("--instances", type=int, default=100, help="Number of instances per configuration")
args = parser.parse_args()
print('Launching Command JSON creation')
subprocess.run('python3 {0}/evaluation/launcher.py --pyFile {0}/CnnAbsTB.py --net {0}/evaluation/networkA.h5 --prop_dist 0.03 --propagate_from_file --runs_per_type {1} --batchDir {2}'.format(CnnAbs.maraboupyPath, args.instances, graphDir).split(' '))
print('Launching Command JSON')
executeFromJson.executeFile('{}/logs_CnnAbs/{}/launcherCmdList.json'.format(CnnAbs.maraboupyPath, graphDir))
print('Creating Graphs - First Parser')
cwd = os.getcwd()
logDir = '{}/logs_CnnAbs/{}'.format(CnnAbs.maraboupyPath, graphDir)
print('chdir {}'.format(logDir))
os.chdir(logDir)
subprocess.run('python3 {}/evaluation/resultsParser.py --graph_dir_name {}'.format(CnnAbs.maraboupyPath, graphDir).split(' '))
os.chdir(cwd)
