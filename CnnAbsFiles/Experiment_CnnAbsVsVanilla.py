
# Experiment comparing Cnn Abs to Vanilla Marabou (reproducing the one that appeared in the paper).
import executeFromJson
import subprocess
import argparse
import os
import itertools
from CnnAbs import CnnAbs

graphDirBase = 'CnnAbsVsVanilla'
parser = argparse.ArgumentParser(description='Launch Policy Comparison')
parser.add_argument("--instances", type=int, default=100, help="Number of instances per configuration")
parser.add_argument("--gtimeout", type=int, default=-1, help="Individual timeout for each verification run.")
parser.add_argument("--results", type=str, default='', help="Directory to put output results plots")
args = parser.parse_args()

if args.results:
    outputDir = os.path.abspath(args.results)
else:
    outputDir = CnnAbs.basePath + '/Results'

networks = ['A', 'B', 'C']
distances = [0.01, 0.02, 0.03]

for net, dist in itertools.product(networks, distances):

    diststr = str(dist)    
    suffix = '_'.join([net, diststr.replace('.','-')])
    graphDir = '_'.join([graphDirBase, suffix])
    
    print('network{}, dist={}'.format(net,dist))
    print('Launching Command JSON creation')
    subprocess.run('python3 {0}/evaluation/launcher.py --pyFile {0}/CnnAbsTB.py --net {0}/evaluation/network{3}.h5 --prop_dist {4} --propagate_from_file --runs_per_type {1} --batchDir {2}'.format(CnnAbs.basePath, args.instances, graphDir, net, diststr).split(' ') + (['--gtimeout', str(args.gtimeout)] if args.gtimeout != -1 else []), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    print('Launching Command JSON')
    executeFromJson.executeFile('{}/logs_CnnAbs/{}/launcherCmdList.json'.format(CnnAbs.basePath, graphDir))
    print('Creating Graphs - First Parser')
    cwd = os.getcwd()
    logDir = '{}/logs_CnnAbs/{}'.format(CnnAbs.basePath, graphDir)
    print('chdir {}'.format(logDir))
    os.chdir(logDir)
    resultsParserCmd ='python3 {}/evaluation/resultsParser.py --graph_dir_name {} --force'.format(CnnAbs.basePath, suffix)
    print('Executing {}'.format(resultsParserCmd))
    subprocess.run(resultsParserCmd.split(' '), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)
    os.chdir(cwd)
    
createGraphCmd = 'python3 {}/evaluation/CreateGraphs.py --dataDir {} --outputDir {} --onlyCnnAbsVsVanilla --gtimeout {}'.format(CnnAbs.basePath, CnnAbs.basePath + '/graphs', outputDir, args.gtimeout)
print('Executing {}'.format(createGraphCmd))
subprocess.run(createGraphCmd.split(' '), stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT)

print('Result Directory: {}'.format(outputDir))
