import tensorflow as tf
#import numpy as np
from CnnAbs_WIP import CnnAbs, Policy
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
parser.add_argument("--fresh",          action="store_true",                        default=False,                  help="Retrain CNN")
parser.add_argument("--validation",     type=str,                                   default="",                     help="Use validation DNN")
parser.add_argument("--cnn_size",       type=str, choices=["big","medium","small","toy"], default="small",          help="Which CNN size to use")
parser.add_argument("--run_title",      type=str,                                   default="default",              help="Add unique identifier identifying this current run")
parser.add_argument("--batch_id",       type=str,                                   default=defaultBatchId,         help="Add unique identifier identifying the whole batch")
parser.add_argument("--prop_distance",  type=float,                                 default=0.03,                   help="Distance checked for adversarial robustness (L1 metric)")
parser.add_argument("--prop_slack",     type=float,                                 default=0,                      help="Slack given at the output property, ysecond >= ymax - slack. Positive slack makes the property easier to satisfy, negative makes it harder.")
parser.add_argument("--timeout",        type=int,                                   default=600,                    help="Single solver timeout in seconds.")
parser.add_argument("--gtimeout",       type=int,                                   default=7200,                   help="Global timeout for all solving in seconds.")
parser.add_argument("--sample",         type=int,                                   default=0,                      help="Index, in MNIST database, of sample image to run on.")
parser.add_argument("--policy",         type=str, choices=Policy.solvingPolicies(),       default="Vanilla",        help="Which abstraction policy to use")
parser.add_argument("--bound_tightening",         type=str, choices=["lp", "lp-inc", "milp", "milp-inc", "iter-prop", "none"], default="lp", help="Which bound tightening technique to use.")
parser.add_argument("--symbolic",       type=str, choices=["deeppoly", "sbt", "none"], default="sbt",               help="Which bound tightening technique to use.")
rerun_parser = parser.add_mutually_exclusive_group(required=False)
rerun_parser.add_argument('--rerun_spurious'   , dest='rerun_spurious', action='store_true',  help="When recieved spurious SAT, run again CEX to find a satisfying assignment.")
rerun_parser.add_argument('--norerun_spurious', dest='rerun_spurious', action='store_false', help="Disable: When recieved spurious SAT, run again CEX to find a satisfying assignment.")
parser.set_defaults(rerun_spurious=False)

args = parser.parse_args()

resultsJson = dict()
cfg_freshModelOrig    = args.fresh
cfg_propDist          = args.prop_distance
cfg_propSlack         = args.prop_slack
cfg_runTitle          = args.run_title
cfg_batchDir          = args.batch_id if "batch_" + args.batch_id else ""
cfg_abstractionPolicy = args.policy
cfg_sampleIndex       = args.sample
cfg_boundTightening   = args.bound_tightening
cfg_symbolicTightening= args.symbolic
cfg_timeoutInSeconds  = args.timeout
cfg_validation        = args.validation
cfg_cnnSizeChoice     = args.cnn_size
cfg_rerunSpurious     = args.rerun_spurious
cfg_gtimeout          = args.gtimeout

options = dict(verbosity=0, timeoutInSeconds=cfg_timeoutInSeconds, milpTightening=cfg_boundTightening, dumpBounds=True, tighteningStrategy=cfg_symbolicTightening, milpSolverTimeout=100)

cnnAbs = CnnAbs(ds='mnist', options=options, logDir="/".join(filter(None, ["logs", cfg_batchDir, cfg_runTitle])), gtimeout=cfg_gtimeout, maskIndex='', policy=cfg_abstractionPolicy)

startPrepare = time.time()

cnnAbs.resultsJson["cfg_freshModelOrig"]    = cfg_freshModelOrig
cnnAbs.resultsJson["cfg_cnnSizeChoice"]     = cfg_cnnSizeChoice
cnnAbs.resultsJson["cfg_propDist"]          = cfg_propDist
cnnAbs.resultsJson["cfg_propSlack"]         = cfg_propSlack
cnnAbs.resultsJson["cfg_runTitle"]          = cfg_runTitle
cnnAbs.resultsJson["cfg_batchDir"]          = cfg_batchDir
cnnAbs.resultsJson["cfg_abstractionPolicy"] = cfg_abstractionPolicy
cnnAbs.resultsJson["cfg_sampleIndex"]       = cfg_sampleIndex
cnnAbs.resultsJson["cfg_boundTightening"]   = cfg_boundTightening
cnnAbs.resultsJson["cfg_symbolicTightening"]= cfg_symbolicTightening
cnnAbs.resultsJson["cfg_timeoutInSeconds"]  = cfg_timeoutInSeconds
cnnAbs.resultsJson["cfg_validation"]        = cfg_validation
cnnAbs.resultsJson["cfg_rerunSpurious"]     = cfg_rerunSpurious
cnnAbs.resultsJson["cfg_gtimeout"]          = cfg_gtimeout
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
modelOrig = cnnAbs.modelUtils.genCnnForAbsTest(cfg_freshModelOrig=cfg_freshModelOrig, cnnSizeChoice=cfg_cnnSizeChoice, validation=cfg_validation)
CnnAbs.printLog("Finished model building")

cnnAbs.solve(modelOrig, cfg_abstractionPolicy, cfg_sampleIndex, cfg_propDist, propSlack=cfg_propSlack)
