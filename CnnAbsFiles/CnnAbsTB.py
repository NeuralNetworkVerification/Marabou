import os
import tensorflow as tf
#import numpy as np
from CnnAbs import CnnAbs, Policy
import time
import argparse
import datetime

###########################################################################
####  _____              __ _                   _   _                  ####
#### /  __ \            / _(_)                 | | (_)                 ####
#### | /  \/ ___  _ __ | |_ _  __ _ _   _  __ _| |_ _  ___  _ __  ___  ####
#### | |    / _ \| '_ \|  _| |/ _` | | | |/ _` | __| |/ _ \| '_ \/ __| ####
#### | \__/\ (_) | | | | | | | (_| | |_| | (_| | |_| | (_) | | | \__ \ ####
####  \____/\___/|_| |_|_| |_|\__, |\__,_|\__,_|\__|_|\___/|_| |_|___/ ####
####                           __/ |                                   ####
####                          |___/                                    ####
###########################################################################

defaultBatchId = "default_" + datetime.datetime.now().strftime("%d-%m-%y_%H-%M-%S")
parser = argparse.ArgumentParser(description='Solve adversarial robustness query using CNN-Abs absttraction refinement verification scheme.')
parser.add_argument("--sample",         type=int,                                   default=0,                      help="Index in database of sample image to run on.")
parser.add_argument("--prop_distance",  type=float,                                 default=0.03,                   help="Distance checked for adversarial robustness (L-inf metric)")
parser.add_argument("--timeout",        type=int,                                   default=800,                    help="Single solver timeout in seconds.")
parser.add_argument("--gtimeout",       type=int,                                   default=3600,                   help="Global timeout for all solving in seconds.")
parser.add_argument("--policy",         type=str, choices=Policy.allPolicies(),       default="Vanilla",        help="Which abstraction policy to use")
parser.add_argument("--net", type=str, default="", help="Verified neural network (.h5 file)", required=True)
parser.add_argument("--abstract_first", dest='abstract_first', action='store_true' , help="Abstract the first layer, instead of the last Max-pooling layer (used for policy comparison experiment)")
parser.add_argument("--propagate_from_file", dest='propagate_from_file', action='store_true' , help="Read propagated bounds from file, for no-Gurobi runs.")
parser.add_argument("--run_title",      type=str,                                   default="default",              help="Add unique identifier identifying this current run (used by scripts).")
parser.add_argument("--batch_id",       type=str,                                   default=defaultBatchId,         help="Add unique identifier identifying the whole batch (used by scripts).")

args = parser.parse_args()

resultsJson = dict()
cfg_distance          = args.prop_distance
cfg_runTitle          = args.run_title
assert args.batch_id
cfg_batchDir          = args.batch_id
cfg_abstractionPolicy = args.policy
cfg_sampleIndex       = args.sample
cfg_boundTightening   = 'lp'
cfg_symbolicTightening= 'sbt'
cfg_timeoutInSeconds  = args.timeout
cfg_gtimeout          = args.gtimeout
cfg_network           = args.net
cfg_abstractFirst     = args.abstract_first

cfg_propagateFromFile = args.propagate_from_file

#cfg_cwd = os.getcwd()

options = dict(verbosity=0, timeoutInSeconds=cfg_timeoutInSeconds, milpTightening=cfg_boundTightening, dumpBounds=True, tighteningStrategy=cfg_symbolicTightening, milpSolverTimeout=100)


cnnAbs = CnnAbs(ds='mnist', options=options, logDir="/".join(filter(None, ["logs_CnnAbs", cfg_batchDir, cfg_runTitle])), gtimeout=cfg_gtimeout, policy=cfg_abstractionPolicy, abstractFirst=cfg_abstractFirst, network=cfg_network, propagateFromFile=cfg_propagateFromFile)

startPrepare = time.time()

cnnAbs.resultsJson["cfg_distance"]          = cfg_distance
cnnAbs.resultsJson["cfg_runTitle"]          = cfg_runTitle
cnnAbs.resultsJson["cfg_batchDir"]          = cfg_batchDir
cnnAbs.resultsJson["cfg_abstractionPolicy"] = cfg_abstractionPolicy
cnnAbs.resultsJson["cfg_sampleIndex"]       = cfg_sampleIndex
cnnAbs.resultsJson["cfg_boundTightening"]   = cfg_boundTightening
cnnAbs.resultsJson["cfg_symbolicTightening"]= cfg_symbolicTightening
cnnAbs.resultsJson["cfg_timeoutInSeconds"]  = cfg_timeoutInSeconds
cnnAbs.resultsJson["cfg_gtimeout"]          = cfg_gtimeout
cnnAbs.resultsJson["cfg_network"]           = cfg_network
cnnAbs.resultsJson["cfg_abstractFirst"]     = cfg_abstractFirst
cnnAbs.resultsJson["SAT"] = None
cnnAbs.resultsJson["Result"] = "GTIMEOUT"

cnnAbs.resultsJson["cfg_propagateFromFile"] = cfg_propagateFromFile
cnnAbs.dumpResultsJson()

#############################################################################################
####  _   _           _  __ _           _   _               ______ _                     ####
#### | | | |         (_)/ _(_)         | | (_)              | ___ \ |                    ####
#### | | | | ___ _ __ _| |_ _  ___ __ _| |_ _  ___  _ __    | |_/ / |__   __ _ ___  ___  ####
#### | | | |/ _ \ '__| |  _| |/ __/ _` | __| |/ _ \| '_ \   |  __/| '_ \ / _` / __|/ _ \ ####
#### \ \_/ /  __/ |  | | | | | (_| (_| | |_| | (_) | | | |  | |   | | | | (_| \__ \  __/ ####
####  \___/ \___|_|  |_|_| |_|\___\__,_|\__|_|\___/|_| |_|  \_|   |_| |_|\__,_|___/\___| ####
####                                                                                     ####
#############################################################################################

CnnAbs.printLog("Started model building")
modelTF = cnnAbs.modelUtils.loadModel(cfg_network)
CnnAbs.printLog("Finished model building")

result, cex = cnnAbs.solveAdversarial(modelTF, cfg_abstractionPolicy, cfg_sampleIndex, cfg_distance)
cexFile = cnnAbs.logDir + 'finalCEX.npy'
print('Result={}')
if result.lower() == 'sat':
    print('cex at {}'.format(result, cexFile))    
    cnnAbs.dumpNpArray(cex, 'finalCEX')
CnnAbs.printLog("Log files at {}".format(cnnAbs.logDir))
