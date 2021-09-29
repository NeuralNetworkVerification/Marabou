import sys
import os
import tensorflow as tf
import numpy as np
import keras2onnx
import onnx
import onnxruntime
from CnnAbs_WIP import *
#from cnn_abs import *
import logging
import time
import argparse
import datetime
import json

#from itertools import product, chain
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
from tensorflow.keras.models import load_model
import matplotlib.pyplot as plt

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
parser.add_argument("--no_coi",         action="store_true",                        default=False,                  help="Don't use COI pruning")
parser.add_argument("--no_mask",        action="store_true",                        default=False,                  help="Don't use mask abstraction")
parser.add_argument("--no_full",        action="store_true",                        default=False,                  help="Don't run on full network")
parser.add_argument("--no_verify",      action="store_true",                        default=False,                  help="Don't run verification process")
parser.add_argument("--dump_queries",   action="store_true",                        default=False,                  help="Don't solve queries, just create and dump them")
parser.add_argument("--use_dumped_queries", action="store_true",                    default=False,                  help="Use dumped queries")
parser.add_argument("--dump_dir",       type=str,                                   default="",                     help="Location of dumped queries")
parser.add_argument("--fresh",          action="store_true",                        default=False,                  help="Retrain CNN")
parser.add_argument("--validation",     type=str,                                   default="",                     help="Use validation DNN")
parser.add_argument("--cnn_size",       type=str, choices=["big","medium","small","toy"], default="small",          help="Which CNN size to use")
parser.add_argument("--run_on",         type=str, choices=["local", "cluster"],     default="local",                help="Is the program running on cluster or local run?")
parser.add_argument("--run_title",      type=str,                                   default="default",              help="Add unique identifier identifying this current run")
parser.add_argument("--batch_id",       type=str,                                   default=defaultBatchId,         help="Add unique identifier identifying the whole batch")
parser.add_argument("--prop_distance",  type=float,                                 default=0.03,                   help="Distance checked for adversarial robustness (L1 metric)")
parser.add_argument("--prop_slack",     type=float,                                 default=0,                      help="Slack given at the output property, ysecond >= ymax - slack. Positive slack makes the property easier to satisfy, negative makes it harder.")
parser.add_argument("--num_cpu",        type=int,                                   default=8,                      help="Number of CPU workers in a cluster run.")
parser.add_argument("--timeout",        type=int,                                   default=600,                    help="Single solver timeout in seconds.")
parser.add_argument("--gtimeout",       type=int,                                   default=7200,                   help="Global timeout for all solving in seconds.")
#parser.add_argument("--timeout_factor", type=float,                                 default=1.5,                   help="timeoutFactor in DNC mode.")
parser.add_argument("--sample",         type=int,                                   default=0,                      help="Index, in MNIST database, of sample image to run on.")
parser.add_argument("--policy",         type=str, choices=Policy.solvingPolicies(),       default="Vanilla",        help="Which abstraction policy to use")
parser.add_argument("--spurious_strict",action="store_true",                        default=True,                   help="Criteria for spurious is that the original label is not achieved (no flag) or the second label is actually voted more tha the original (flag)")
parser.add_argument("--double_check"   ,action="store_true",                        default=False,                  help="Run Marabou again using recieved CEX as an input assumption.")
parser.add_argument("--bound_tightening",         type=str, choices=["lp", "lp-inc", "milp", "milp-inc", "iter-prop", "none"], default="lp", help="Which bound tightening technique to use.")
parser.add_argument("--symbolic",       type=str, choices=["deeppoly", "sbt", "none"], default="sbt",               help="Which bound tightening technique to use.")
parser.add_argument("--solve_with_milp",action="store_true",                        default=False,                  help="Use MILP solver instead of regular engine.")
parser.add_argument("--abs_layer",      type=str, default="c2",              help="Which layer should be abstracted.")
parser.add_argument("--arg",  type=str, default="", help="Push custom string argument.")
parser.add_argument("--no_dumpBounds",action="store_true",                          default=False,                  help="Disable initial bound tightening.")
parser.add_argument("--mask_index",     type=int,                                  default=-1,                      help="Choose specific mask to run.")
parser.add_argument("--slurm_seq"      ,action="store_true",                        default=False,                  help="Run next mask if this one fails.")
rerun_parser = parser.add_mutually_exclusive_group(required=False)
rerun_parser.add_argument('--rerun_spurious'   , dest='rerun_spurious', action='store_true',  help="When recieved spurious SAT, run again CEX to find a satisfying assignment.")
rerun_parser.add_argument('--norerun_spurious', dest='rerun_spurious', action='store_false', help="Disable: When recieved spurious SAT, run again CEX to find a satisfying assignment.")
parser.set_defaults(rerun_spurious=True)

args = parser.parse_args()

resultsJson = dict()
cfg_freshModelOrig    = args.fresh
cfg_noVerify          = args.no_verify
cfg_pruneCOI          = not args.no_coi
cfg_maskAbstract      = not args.no_mask
cfg_runFull           = not args.no_full
cfg_propDist          = args.prop_distance
cfg_propSlack         = args.prop_slack
cfg_runOn             = args.run_on
cfg_runTitle          = args.run_title
cfg_batchDir          = args.batch_id if "batch_" + args.batch_id else ""
cfg_numClusterCPUs    = args.num_cpu
cfg_abstractionPolicy = args.policy
cfg_spuriousStrict    = args.spurious_strict
cfg_sampleIndex       = args.sample
cfg_doubleCheck       = args.double_check
cfg_boundTightening   = args.bound_tightening
cfg_solveWithMILP     = args.solve_with_milp
cfg_symbolicTightening= args.symbolic
cfg_timeoutInSeconds  = args.timeout
cfg_dumpQueries       = args.dump_queries
cfg_useDumpedQueries  = args.use_dumped_queries
cfg_dumpDir           = args.dump_dir
cfg_validation        = args.validation
cfg_cnnSizeChoice     = args.cnn_size
#cfg_dumpBounds        = cfg_maskAbstract or (cfg_boundTightening != "none")
cfg_dumpBounds        = not args.no_dumpBounds
cfg_absLayer          = args.abs_layer
cfg_extraArg          = args.arg
cfg_maskIndex         = args.mask_index
cfg_slurmSeq          = args.slurm_seq
cfg_rerunSpurious     = args.rerun_spurious
cfg_gtimeout          = args.gtimeout

optionsLocal   = Marabou.createOptions(snc=False, verbosity=0,                                solveWithMILP=cfg_solveWithMILP, timeoutInSeconds=cfg_timeoutInSeconds, milpTightening=cfg_boundTightening, dumpBounds=cfg_dumpBounds, tighteningStrategy=cfg_symbolicTightening, milpSolverTimeout=100) #FIXME does actually tightening bounds with timeout>0 improve results?
optionsCluster = Marabou.createOptions(snc=True,  verbosity=0, numWorkers=cfg_numClusterCPUs, solveWithMILP=cfg_solveWithMILP, timeoutInSeconds=cfg_timeoutInSeconds, milpTightening=cfg_boundTightening, dumpBounds=cfg_dumpBounds, tighteningStrategy=cfg_symbolicTightening)
if cfg_runOn == "local":
    optionsObj = optionsLocal
else :
    optionsObj = optionsCluster

policy = Policy.fromString(cfg_abstractionPolicy, 'mnist')

maskIndexStr = str(cfg_maskIndex) if cfg_maskIndex > 0 else ''
cnnAbs = CnnAbs(ds='mnist', dumpDir=cfg_dumpDir, optionsObj=optionsObj, logDir="/".join(filter(None, [CnnAbs.basePath, "logs", cfg_batchDir, cfg_runTitle])), dumpQueries=cfg_dumpQueries, useDumpedQueries=cfg_useDumpedQueries, gtimeout=cfg_gtimeout, maskIndex=maskIndexStr, policy=policy)

startPrepare = time.time()

mi = lambda s: s if not cfg_slurmSeq else (s + "-" + str(cfg_maskIndex))

cnnAbs.resultsJson[mi("cfg_freshModelOrig")]    = cfg_freshModelOrig
cnnAbs.resultsJson[mi("cfg_noVerify")]          = cfg_noVerify
cnnAbs.resultsJson[mi("cfg_cnnSizeChoice")]     = cfg_cnnSizeChoice
cnnAbs.resultsJson[mi("cfg_pruneCOI")]          = cfg_pruneCOI
cnnAbs.resultsJson[mi("cfg_maskAbstract")]      = cfg_maskAbstract
cnnAbs.resultsJson[mi("cfg_propDist")]          = cfg_propDist
cnnAbs.resultsJson[mi("cfg_propSlack")]         = cfg_propSlack
cnnAbs.resultsJson[mi("cfg_runOn")]             = cfg_runOn
cnnAbs.resultsJson[mi("cfg_runTitle")]          = cfg_runTitle
cnnAbs.resultsJson[mi("cfg_batchDir")]          = cfg_batchDir
cnnAbs.resultsJson[mi("cfg_numClusterCPUs")]    = cfg_numClusterCPUs
cnnAbs.resultsJson[mi("cfg_abstractionPolicy")] = cfg_abstractionPolicy
cnnAbs.resultsJson[mi("cfg_spuriousStrict")]    = cfg_spuriousStrict
cnnAbs.resultsJson[mi("cfg_sampleIndex")]       = cfg_sampleIndex
cnnAbs.resultsJson[mi("cfg_doubleCheck")]       = cfg_doubleCheck
cnnAbs.resultsJson[mi("cfg_boundTightening")]   = cfg_boundTightening
cnnAbs.resultsJson[mi("cfg_solveWithMILP")]     = cfg_solveWithMILP
cnnAbs.resultsJson[mi("cfg_symbolicTightening")]= cfg_symbolicTightening
cnnAbs.resultsJson[mi("cfg_timeoutInSeconds")]  = cfg_timeoutInSeconds
cnnAbs.resultsJson[mi("cfg_dumpQueries")]       = cfg_dumpQueries
cnnAbs.resultsJson[mi("cfg_useDumpedQueries")]  = cfg_useDumpedQueries
cnnAbs.resultsJson[mi("cfg_dumpDir")]           = cfg_dumpDir
cnnAbs.resultsJson[mi("cfg_validation")]        = cfg_validation
cnnAbs.resultsJson[mi("cfg_dumpBounds")]        = cfg_dumpBounds
cnnAbs.resultsJson[mi("cfg_extraArg")]          = cfg_extraArg
cnnAbs.resultsJson[mi("cfg_maskIndex")]         = cfg_maskIndex
cnnAbs.resultsJson[mi("cfg_slurmSeq")]          = cfg_slurmSeq
cnnAbs.resultsJson[mi("cfg_rerunSpurious")]     = cfg_rerunSpurious
cnnAbs.resultsJson[mi("cfg_gtimeout")]          = cfg_gtimeout
cnnAbs.resultsJson["SAT"] = None
cnnAbs.resultsJson["Result"] = "TIMEOUT"
#cnnAbs.resultsJson["subResults"] = []
cnnAbs.dumpResultsJson()

###############################################################################
#### ______                                ___  ___          _      _      ####
#### | ___ \                               |  \/  |         | |    | |     ####
#### | |_/ / __ ___ _ __   __ _ _ __ ___   | .  . | ___   __| | ___| |___  ####
#### |  __/ '__/ _ \ '_ \ / _` | '__/ _ \  | |\/| |/ _ \ / _` |/ _ \ / __| ####
#### | |  | | |  __/ |_) | (_| | | |  __/  | |  | | (_) | (_| |  __/ \__ \ ####
#### \_|  |_|  \___| .__/ \__,_|_|  \___|  \_|  |_/\___/ \__,_|\___|_|___/ ####
####               | |                                                     ####
####               |_|                                                     ####
###############################################################################

replaceLayerName = cfg_absLayer

## Build initial model.

CnnAbs.printLog("Started model building")
modelOrig = cnnAbs.modelUtils.genCnnForAbsTest(cfg_freshModelOrig=cfg_freshModelOrig, cnnSizeChoice=cfg_cnnSizeChoice, validation=cfg_validation)
maskShape = modelOrig.get_layer(name=replaceLayerName).output_shape[:-1]
if maskShape[0] == None:
    maskShape = maskShape[1:]

CnnAbs.printLog("Finished model building")

#############################################################################################
####  _   _           _  __ _           _   _               ______ _                     ####
#### | | | |         (_)/ _(_)         | | (_)              | ___ \ |                    ####
#### | | | | ___ _ __ _| |_ _  ___ __ _| |_ _  ___  _ __    | |_/ / |__   __ _ ___  ___  ####
#### | | | |/ _ \ '__| |  _| |/ __/ _` | __| |/ _ \| '_ \   |  __/| '_ \ / _` / __|/ _ \ ####
#### \ \_/ /  __/ |  | | | | | (_| (_| | |_| | (_) | | | |  | |   | | | | (_| \__ \  __/ ####
####  \___/ \___|_|  |_|_| |_|\___\__,_|\__|_|\___/|_| |_|  \_|   |_| |_|\__,_|___/\___| ####
####                                                                                     ####
#############################################################################################

cnnAbs.solve(modelOrig, cfg_abstractionPolicy, cfg_sampleIndex, cfg_propDist, 'mnist')
