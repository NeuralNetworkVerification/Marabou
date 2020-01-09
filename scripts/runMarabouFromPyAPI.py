import warnings
warnings.filterwarnings('ignore')
import argparse
import configparser
import numpy as np
import importlib
import os
import sys
import time

def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('-c', '--config-file', required=True, type=str, default=None, help = '')
    p.add_argument('-n', '--network-file', required=True, type=str, default=None, help = '')
    p.add_argument('-p', '--property-file', required=True, type=str, default=None, help = '')
    p.add_argument('-s', '--summary-file', required=True, type=str, default=None, help='')
    p.add_argument('--dnc', action='store_true', default=False)
    p.add_argument('--verbosity', required=False, type=int, default=0)
    p.add_argument('--initial-divides', required=False, type=int, default=2)
    p.add_argument('--initial-timeout', required=False, type=int, default=-1)
    p.add_argument('--num-workers', required=False, type=int, default=4)
    p.add_argument('--num-online-divides', required=False, type=int, default=2)
    p.add_argument('--timeout-factor', required=False, type=float, default=1.5)
    p.add_argument('--restore-tree-states', action='store_true', default=False)
    p.add_argument('--look-ahead-preprocessing', action='store_true', default=False)
    p.add_argument('--preprocess-only', action='store_true', default=False)
    p.add_argument('--divide-strategy', required=False, type=str, default='auto')
    p.add_argument('--bias-strategy', required=False, type=str, default='centroid')
    p.add_argument('--focus-layer', required=False, type=int, default=0)
    p.add_argument('--fixed-relu', required=False, type=str, default="")
    p.add_argument("--max-depth", required=False, type=int, default=5, help="max depth")

    opts = p.parse_args()
    return opts

opts = parse_args()

config_file = opts.config_file
network_file = opts.network_file
property_file = opts.property_file
summary_file = opts.summary_file
fixed_relu_file = opts.fixed_relu


cfg = configparser.ConfigParser()
cfg.read(config_file)

marabou_path = cfg.get('global', 'marabou_path')
print("Marabou path: {}".format(marabou_path))
sys.path.append(marabou_path)
from maraboupy import Marabou

network = Marabou.read_tf(network_file)
print("Number of variables: %d"%network.numVars)
print("Number of input variables: %d"%len(np.array(network.inputVars).flatten()))
print("Number of outpu variables: %d"%len(np.array(network.outputVars).flatten()))
print("Number of ReLUs: %d"%len(network.reluList))

dirname = os.path.abspath(os.path.dirname(property_file))
sys.path.append(dirname)
property_module_name = os.path.basename(property_file)[:-3]
property_module = importlib.import_module(os.path.basename(property_module_name))
network = property_module.encode_property(network)

dnc = opts.dnc
verbosity=opts.verbosity
initial_divides= opts.initial_divides
initial_timeout = opts.initial_timeout
num_workers = opts.num_workers
online_divides = opts.num_online_divides
timeout_factor = opts.timeout_factor
restore_tree_states = opts.restore_tree_states
look_ahead_preprocessing = opts.look_ahead_preprocessing
preprocess_only = opts.preprocess_only
bias_strategy = opts.bias_strategy
divide_strategy = opts.divide_strategy
focus_layer = opts.focus_layer
max_depth = opts.max_depth

options = Marabou.createOptions(dnc=dnc, verbosity=verbosity, initialDivides=initial_divides,
                                initialTimeout=initial_timeout, numWorkers=num_workers,
                                onlineDivides=online_divides, timeoutFactor=timeout_factor,
                                lookAheadPreprocessing=look_ahead_preprocessing,
                                preprocessOnly=preprocess_only, focusLayer=focus_layer,
                                divideStrategy=divide_strategy, biasStrategy=bias_strategy,
                                maxDepth=max_depth)

start = time.time()
vals, stats = network.solve(summaryFilePath=summary_file, fixedReluFilePath=fixed_relu_file,
                            options=options)
end = time.time()
t = end-start

with open (summary_file, 'w') as out_file:
    if len(vals)>0:
        out_file.write("SAT")
    else:
        out_file.write("UNSAT")
    out_file.write(" {}\n".format(t))
