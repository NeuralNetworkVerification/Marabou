
import executeFromJson
import subprocess
import argparse
import os
from CnnAbs import CnnAbs

graphDir = 'ComparePolicies'
parser = argparse.ArgumentParser(description='Launch Policy Comparison')
parser.add_argument("--instances", type=int, default=100, help="Number of instances per configuration")
parser.add_argument("--gtimeout", type=int, default=-1, help="Individual timeout for each verification run.")
args = parser.parse_args()
print('Launching Command JSON creation')
subprocess.run('python3 {0}/evaluation/launcher.py --pyFile {0}/CnnAbsTB.py --net {0}/evaluation/networkA.h5 --prop_dist 0.03 --propagate_from_file --runs_per_type {1} --batchDir {2}'.format(CnnAbs.maraboupyPath, args.instances, graphDir).split(' ') + (['--gtimeout', str(args.gtimeout)] if args.gtimeout != -1 else []))
print('Launching Command JSON')
executeFromJson.executeFile('{}/logs_CnnAbs/{}/launcherCmdList.json'.format(CnnAbs.maraboupyPath, graphDir))
print('Creating Graphs - First Parser')
cwd = os.getcwd()
logDir = '{}/logs_CnnAbs/{}'.format(CnnAbs.maraboupyPath, graphDir)
print('chdir {}'.format(logDir))
os.chdir(logDir)
resultsParserCmd ='python3 {}/evaluation/resultsParser.py --graph_dir_name {} --force'.format(CnnAbs.maraboupyPath, graphDir)
print('Executing {}'.format(resultsParserCmd))
subprocess.run(resultsParserCmd.split(' '))
os.chdir(cwd)
createGraphCmd = 'python3 {}/evaluation/CreateGraphs.py --dataDir {} --outputDir {} --onlyComparePolicies'.format(CnnAbs.maraboupyPath, CnnAbs.maraboupyPath + '/graphs', CnnAbs.maraboupyPath + '/Results')
print('Executing {}'.format(createGraphCmd))
subprocess.run(createGraphCmd.split(' '))

