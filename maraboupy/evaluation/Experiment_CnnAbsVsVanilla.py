
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
    outputDir = args.results
else:
    outputDir = CnnAbs.maraboupyPath + '/Results'

networks = ['A', 'B', 'C']
distances = [0.01, 0.02, 0.03]

for net, dist in itertools.product(networks, distances):

    diststr = str(dist)    
    suffix = '_'.join([net, diststr.replace('.','-')])
    graphDir = '_'.join([graphDirBase, suffix])
    
    print('network{}, dist={}'.format(net,dist))
    print('Launching Command JSON creation')
    subprocess.run('python3 {0}/evaluation/launcher.py --pyFile {0}/CnnAbsTB.py --net {0}/evaluation/network{3}.h5 --prop_dist {4} --propagate_from_file --runs_per_type {1} --batchDir {2}'.format(CnnAbs.maraboupyPath, args.instances, graphDir, net, diststr).split(' ') + (['--gtimeout', str(args.gtimeout)] if args.gtimeout != -1 else []))
    print('Launching Command JSON')
    executeFromJson.executeFile('{}/logs_CnnAbs/{}/launcherCmdList.json'.format(CnnAbs.maraboupyPath, graphDir))
    print('Creating Graphs - First Parser')
    cwd = os.getcwd()
    logDir = '{}/logs_CnnAbs/{}'.format(CnnAbs.maraboupyPath, graphDir)
    print('chdir {}'.format(logDir))
    os.chdir(logDir)
    resultsParserCmd ='python3 {}/evaluation/resultsParser.py --graph_dir_name {} --force'.format(CnnAbs.maraboupyPath, suffix)
    print('Executing {}'.format(resultsParserCmd))
    subprocess.run(resultsParserCmd.split(' '))
    os.chdir(cwd)
    
createGraphCmd = 'python3 {}/evaluation/CreateGraphs.py --dataDir {} --outputDir {} --onlyCnnAbsVsVanilla'.format(CnnAbs.maraboupyPath, CnnAbs.maraboupyPath + '/graphs', outputDir)
print('Executing {}'.format(createGraphCmd))
subprocess.run(createGraphCmd.split(' '))

print('Result Directory: {}'.format(outputDir))
