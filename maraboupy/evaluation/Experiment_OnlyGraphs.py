
import executeFromJson
import subprocess
import argparse
import os
import itertools
from CnnAbs import CnnAbs

graphDirBase = 'CnnAbsVsVanilla'
parser = argparse.ArgumentParser(description='Create results of both experiments from existing results.')
parser.add_argument("--results", type=str, default='', required=True, help="Directory to put output results plots")
parser.add_argument("--logs", type=str, default='', required=True, help="Directory to in which result logs tree reside")

args = parser.parse_args()

if args.results:
    outputDir = os.path.abspath(args.results)
else:
    outputDir = CnnAbs.maraboupyPath + '/Results'
if args.logs:
    logsDir = os.path.abspath(args.logs)
else:
    logsDir = CnnAbs.maraboupyPath + '/logs_CnnAbs'

if logsDir.endswith('/'):
    logsDir = logsDir[:-1]

networks = ['A', 'B', 'C']
distances = [0.01, 0.02, 0.03]

for net, dist in itertools.product(networks, distances):

    diststr = str(dist)    
    suffix = '_'.join([net, diststr.replace('.','-')])
    graphDir = '_'.join([graphDirBase, suffix])
    
    print('network{}, dist={}'.format(net,dist))
    print('Creating Graphs - First Parser')
    cwd = os.getcwd()
    logDir = '{}/{}'.format(logsDir, graphDir)
    print('chdir {}'.format(logDir))
    os.chdir(logDir)
    resultsParserCmd ='python3 {}/evaluation/resultsParser.py --graph_dir_name {} --force'.format(CnnAbs.maraboupyPath, suffix)
    print('Executing {}'.format(resultsParserCmd))
    subprocess.run(resultsParserCmd.split(' '), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    os.chdir(cwd)

graphDir = 'ComparePolicies'
print('Creating Graphs - First Parser')
cwd = os.getcwd()
logDir = '{}/logs_CnnAbs/{}'.format(CnnAbs.maraboupyPath, graphDir)
print('chdir {}'.format(logDir))
os.chdir(logDir)
resultsParserCmd ='python3 {}/evaluation/resultsParser.py --graph_dir_name {} --force'.format(CnnAbs.maraboupyPath, graphDir)
print('Executing {}'.format(resultsParserCmd))
subprocess.run(resultsParserCmd.split(' '), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
os.chdir(cwd)
createGraphCmd = 'python3 {}/evaluation/CreateGraphs.py --dataDir {} --outputDir {} --onlyComparePolicies'.format(CnnAbs.maraboupyPath, CnnAbs.maraboupyPath + '/graphs', outputDir)
print('Executing {}'.format(createGraphCmd))
subprocess.run(createGraphCmd.split(' '), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    
createGraphCmd = 'python3 {}/evaluation/CreateGraphs.py --dataDir {} --outputDir {}'.format(CnnAbs.maraboupyPath, CnnAbs.maraboupyPath + '/graphs', outputDir)
print('Executing {}'.format(createGraphCmd))
subprocess.run(createGraphCmd.split(' '), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

print('Result Directory: {}'.format(outputDir))
