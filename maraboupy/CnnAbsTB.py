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

tf.compat.v1.enable_v2_behavior()

defaultBatchId = "default_" + datetime.datetime.now().strftime("%d-%m-%y_%H-%M-%S")
parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--run_title",      type=str,                                   default="default",              help="Add unique identifier identifying this current run")
parser.add_argument("--batch_id",       type=str,                                   default=defaultBatchId,         help="Add unique identifier identifying the whole batch")
parser.add_argument("--prop_distance",  type=float,                                 default=0.03,                   help="Distance checked for adversarial robustness (L1 metric)")
parser.add_argument("--prop_slack",     type=float,                                 default=0,                      help="Slack given at the output property, ysecond >= ymax - slack. Positive slack makes the property easier to satisfy, negative makes it harder.")
parser.add_argument("--timeout",        type=int,                                   default=800,                    help="Single solver timeout in seconds.")
parser.add_argument("--gtimeout",       type=int,                                   default=3600,                   help="Global timeout for all solving in seconds.")
parser.add_argument("--sample",         type=int,                                   default=0,                      help="Index, in MNIST database, of sample image to run on.")
parser.add_argument("--policy",         type=str, choices=Policy.allPolicies(),       default="Vanilla",        help="Which abstraction policy to use")

parser.add_argument("--net", type=str, default="", help="verified neural network", required=True)
parser.add_argument("--abstract_first", dest='abstract_first', action='store_true' , help="Abstract the first layer (used for specific experiment)")

args = parser.parse_args()

resultsJson = dict()
cfg_propDist          = args.prop_distance
cfg_propSlack         = args.prop_slack
cfg_runTitle          = args.run_title
cfg_batchDir          = args.batch_id if "batch_" + args.batch_id else ""
cfg_abstractionPolicy = args.policy
cfg_sampleIndex       = args.sample
cfg_boundTightening   = 'lp'
cfg_symbolicTightening= 'sbt'
cfg_timeoutInSeconds  = args.timeout
cfg_gtimeout          = args.gtimeout
cfg_network           = args.net
cfg_abstractFirst     = args.abstract_first

cfg_cwd = os.getcwd()

options = dict(verbosity=0, timeoutInSeconds=cfg_timeoutInSeconds, milpTightening=cfg_boundTightening, dumpBounds=True, tighteningStrategy=cfg_symbolicTightening, milpSolverTimeout=100)

cnnAbs = CnnAbs(ds='mnist', options=options, logDir="/".join(filter(None, ["logs_CnnAbs", cfg_batchDir, cfg_runTitle])), gtimeout=cfg_gtimeout, policy=cfg_abstractionPolicy, abstractFirst=cfg_abstractFirst)

startPrepare = time.time()

cnnAbs.resultsJson["cfg_propDist"]          = cfg_propDist
cnnAbs.resultsJson["cfg_propSlack"]         = cfg_propSlack
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
modelTF = cnnAbs.modelUtils.loadModel(cfg_cwd + '/' + cfg_network)
CnnAbs.printLog("Finished model building")

cnnAbs.solveAdversarial(modelTF, cfg_abstractionPolicy, cfg_sampleIndex, cfg_propDist, propSlack=cfg_propSlack)
CnnAbs.printLog("Log files at {}".format(cnnAbs.logDir))
