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

modelOrigDense = cnnAbs.modelUtils.cloneAndMaskConvModel(modelOrig, replaceLayerName, np.ones(maskShape))
#Created modelOrigDense to compensate on possible translation error when densifing. This way the abstractions are assured to be abstraction of this model.
#compareModels(modelOrig, modelOrigDense)

CnnAbs.printLog("Finished model building")

if cfg_noVerify:
    CnnAbs.printLog("Skipping verification phase")
    exit(0)

#############################################################################################
####  _   _           _  __ _           _   _               ______ _                     ####
#### | | | |         (_)/ _(_)         | | (_)              | ___ \ |                    ####
#### | | | | ___ _ __ _| |_ _  ___ __ _| |_ _  ___  _ __    | |_/ / |__   __ _ ___  ___  ####
#### | | | |/ _ \ '__| |  _| |/ __/ _` | __| |/ _ \| '_ \   |  __/| '_ \ / _` / __|/ _ \ ####
#### \ \_/ /  __/ |  | | | | | (_| (_| | |_| | (_) | | | |  | |   | | | | (_| \__ \  __/ ####
####  \___/ \___|_|  |_|_| |_|\___\__,_|\__|_|\___/|_| |_|  \_|   |_| |_|\__,_|___/\___| ####
####                                                                                     ####
#############################################################################################

## Choose adversarial example
ds = DataSet("mnist")

CnnAbs.printLog("Choosing adversarial example")

xAdv = ds.x_test[cfg_sampleIndex]
yAdv = ds.y_test[cfg_sampleIndex]
yPredict = modelOrigDense.predict(np.array([xAdv]))
yMax = yPredict.argmax()
yPredictNoMax = np.copy(yPredict)
yPredictNoMax[0][yMax] = np.min(yPredict)
ySecond = yPredictNoMax.argmax()
if ySecond == yMax:
    ySecond = 0 if yMax > 0 else 1

fName = "xAdv.png"
CnnAbs.printLog("Printing original input to file {}, this is sample {} with label {}".format(fName, cfg_sampleIndex, yAdv))
plt.figure()
plt.imshow(np.squeeze(xAdv))
plt.title('Example {}. Real Label: {}, modelOrigDense Predicts {}'.format(cfg_sampleIndex, yAdv, yMax))
plt.savefig(fName)
with open(fName.replace("png","npy"), "wb") as f:
    np.save(f, xAdv)


cnnAbs.resultsJson["yDataset"] = int(yAdv.item())
cnnAbs.resultsJson["yMaxPrediction"] = int(yMax)
cnnAbs.resultsJson["ySecondPrediction"] = int(ySecond)
cnnAbs.dumpResultsJson()

cnnAbs.tickGtimeout()

if cfg_dumpBounds and not (cfg_slurmSeq and cfg_maskIndex > 0):
    startBoundTightening = time.time()
    cnnAbs.tickGtimeout()
    if cnnAbs.isGlobalTimedOut():
        cnnAbs.resultsJson["Result"] = "GTIMEOUT"
        cnnAbs.resultsJson[mi("Result")] = "GTIMEOUT"
        accumRuntime = cnnAbs.resultsJson["accumRuntime"] if (("accumRuntime" in cnnAbs.resultsJson) and cfg_slurmSeq) else 0
        cnnAbs.resultsJson["totalRuntime"] = time.time() - cnnAbs.startTotal + accumRuntime
        cnnAbs.dumpResultsJson()
        exit()
    CnnAbs.printLog("Started dumping bounds - used for abstraction")        
    ipq = cnnAbs.modelUtils.dumpBounds(modelOrigDense, xAdv, cfg_propDist, cfg_propSlack, yMax, ySecond)
    MarabouCore.saveQuery(ipq, cnnAbs.logDir + "IPQ_dumpBounds")
    CnnAbs.printLog("Finished dumping bounds - used for abstraction")
    print(ipq.getNumberOfVariables())
    endBoundTightening = time.time()
    cnnAbs.tickGtimeout()
    cnnAbs.resultsJson["boundTighteningRuntime"] = endBoundTightening - startBoundTightening    
    if ipq.getNumberOfVariables() == 0:
        cnnAbs.resultsJson["SAT"] = False
        cnnAbs.resultsJson["Result"] = "UNSAT"
        cnnAbs.resultsJson[mi("Result")] = "UNSAT"
        cnnAbs.resultsJson["successfulRuntime"] = endBoundTightening - startBoundTightening
        cnnAbs.resultsJson["successfulRun"] = 0
        accumRuntime = cnnAbs.resultsJson["accumRuntime"] if (("accumRuntime" in cnnAbs.resultsJson) and cfg_slurmSeq) else 0
        cnnAbs.resultsJson["totalRuntime"] = time.time() - cnnAbs.startTotal + accumRuntime
        cnnAbs.dumpResultsJson()
        CnnAbs.printLog("UNSAT on first LP bound tightening")
        exit()
    else:
        os.rename(cnnAbs.logDir + "dumpBounds.json", cnnAbs.logDir + "dumpBoundsInitial.json") #This is to prevent accidental override of this file.
if cfg_dumpBounds and os.path.isfile(cnnAbs.logDir + "dumpBoundsInitial.json"):
        boundList = cnnAbs.loadJson("dumpBoundsInitial", loadDir=cnnAbs.logDir)
        boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}
else:
    boundDict = None

cnnAbs.optionsObj._dumpBounds = False

if policy.policy is Policy.SingleClassRank:    
    maskList = policy.genActivationMask(ModelUtils.intermidModel(modelOrigDense, replaceLayerName), prediction=yMax, includeFull=cfg_runFull)
else:
    maskList = policy.genActivationMask(ModelUtils.intermidModel(modelOrigDense, replaceLayerName), includeFull=cfg_runFull)
if not cfg_maskAbstract:
    if cfg_runFull:
        maskList = [np.ones(ModelUtils.intermidModel(modelOrigDense, replaceLayerName).output_shape[1:-1])]
    else:
        maskList = []
CnnAbs.printLog("Created {} masks".format(len(maskList)))
cnnAbs.resultsJson["numMasks"] = len(maskList)
cnnAbs.resultsJson["masks"] = ["non_zero={},{}=\n{}".format(i,np.count_nonzero(mask), mask) for i,mask in enumerate(maskList)]
if cfg_maskIndex != -1:
    CnnAbs.printLog("Using only mask {}".format(cfg_maskIndex))    
cnnAbs.dumpResultsJson()
#for i,mask in enumerate(maskList):
#    print("mask, non_zero={},{}=\n{}".format(i,np.count_nonzero(mask), mask))
#exit()

CnnAbs.printLog("Starting verification phase")

successful = None

modelOrigDenseSavedName = "modelOrigDense.h5" # Add logDir to this and ONNX files.
modelOrigDense.save(modelOrigDenseSavedName)
prop = AdversarialProperty(xAdv, yMax, ySecond, cfg_propDist, cfg_propSlack)

#runName = 'test'
#mbouNet, _ , _ , inputVarsMapping, outputVarsMapping, varsMapping, inputs = cnnAbs.genAdvMbouNet(modelOrigDense, prop, boundDict, runName + "_rerunSpurious", False)
#layersDiv, layerType = InputQueryUtils.divideToLayers(mbouNet)
#layerI = 8 if (cfg_validation and ("long" in cfg_validation)) else 5
#print("layerType={}".format(layerType))
#exit()

cnnAbs.numMasks = len(maskList)

cnnAbs.tickGtimeout()

for i, mask in enumerate(maskList):

    globalTimeout = cnnAbs.isGlobalTimedOut()
    if globalTimeout:
        break
    if cfg_maskIndex != -1 and i != cfg_maskIndex:
        continue
    cnnAbs.maskIndex = i
    if not cfg_useDumpedQueries:
        tf.keras.backend.clear_session()
        modelOrig = cnnAbs.modelUtils.genCnnForAbsTest(cfg_freshModelOrig=cfg_freshModelOrig, cnnSizeChoice=cfg_cnnSizeChoice, validation=cfg_validation)
#        pred = modelOrig.predict(np.array([xAdv]))
#        yMax2 = pred.argmax()
#        pred2 = np.copy(pred)
#        pred2[0][yMax] = np.min(pred)
#        ySecond2 = pred2.argmax()
#        assert yMax2 == yMax and ySecond == ySecond2
        modelAbs = cnnAbs.modelUtils.cloneAndMaskConvModel(modelOrig, replaceLayerName, mask)
    else:
        modelAbs = None    

    if i+1 == len(maskList):
        cnnAbs.optionsObj._timeoutInSeconds = 0
    runName = "sample_{},policy_{},propDist_{},mask_{}_outOf_{}".format(cfg_sampleIndex, cfg_abstractionPolicy, str(cfg_propDist).replace('.','-'), i, len(maskList)-1)
    cnnAbs.tickGtimeout()
    resultObj = cnnAbs.runMarabouOnKeras(modelAbs, prop, boundDict, runName, coi=(policy.coi and cfg_pruneCOI), onlyDump=cfg_dumpQueries, fromDumpedQuery=cfg_useDumpedQueries)
    if cfg_dumpQueries:
        continue
    if resultObj.timedOut():        
        continue
    if resultObj.sat():
        if not cfg_useDumpedQueries:
            modelOrigDense = load_model(modelOrigDenseSavedName)
        try:
            isSpurious = ModelUtils.isCEXSpurious(modelOrigDense, prop, resultObj.cex, spuriousStrict=cfg_spuriousStrict, valueRange=cnnAbs.ds.valueRange)
        except Exception as err:
            CnnAbs.printLog(err)
            isSpurious = True
        CnnAbs.printLog("Found {} CEX in mask {}/{}.".format("spurious" if isSpurious else "real", i, len(maskList)-1))
        if not isSpurious:
            successful = i
            break
        elif i+1 == len(maskList):
            resultObj = ResultObj("error")
            break 
            #raise Exception("Spurious CEX at full network.")
        elif cfg_rerunSpurious:
            mbouNet, _ , _ , inputVarsMapping, outputVarsMapping, varsMapping, inputs = cnnAbs.genAdvMbouNet(modelOrigDense, prop, boundDict, runName + "_rerunSpurious", False)
            layersDiv, layerType = InputQueryUtils.divideToLayers(mbouNet)
            layerI = 8 if (cfg_validation and ("long" in cfg_validation)) else 5
            resultObj.preAbsVars = {var for i in range(layerI) for var in layersDiv[i]}
            resultObjRerunSpurious = cnnAbs.runMarabouOnKeras(modelOrigDense, prop, boundDict, runName + "_rerunSpurious", coi=False, rerun=True, rerunObj=resultObj)
            if resultObjRerunSpurious.sat():
                try:
                    isSpurious = ModelUtils.isCEXSpuriouso(modelOrigDense, prop, resultObjRerunSpurious.cex, spuriousStrict=cfg_spuriousStrict, valueRange=cnnAbs.ds.valueRange)
                    assert not isSpurious
                except Exception as err:
                    CnnAbs.printLog(err)
                resultObj = resultObjRerunSpurious
                successful = i
                CnnAbs.printLog("Found real CEX in mask {}/{} after rerun.".format(i, len(maskList)-1))
                break
            else:
                resultObj = ResultObj("spurious")
                CnnAbs.printLog("Didn't found CEX in mask {}/{} after rerun.".format(i, len(maskList)-1))

    elif resultObj.unsat():
        CnnAbs.printLog("Found UNSAT in mask {}/{}.".format(i, len(maskList)-1))
        successful = i
        break
    else:
        raise NotImplementedError

cnnAbs.tickGtimeout()    
globalTimeout = cnnAbs.isGlobalTimedOut()    
if not cfg_dumpQueries:
    if globalTimeout:
        resultObj = ResultObj("gtimeout")
    success = not resultObj.timedOut() and (successful is not None)
    if resultObj.sat() and not success:
        resultObj = ResultObj("spurious")
else:
    success = False

forceContinuation = False #FIXME remove, this is a temp test.
if cfg_slurmSeq and (cfg_dumpQueries or (not success and not globalTimeout) or forceContinuation):
    cnnAbs.resultsJson["accumRuntime"] = time.time() - cnnAbs.startTotal + (cnnAbs.resultsJson["accumRuntime"] if "accumRuntime" in cnnAbs.resultsJson else 0)
    cnnAbs.dumpResultsJson()
    CnnAbs.printLog("Launching next mask")
    cnnAbs.launchNext(batchId=cfg_batchDir, cnnSize=cfg_cnnSizeChoice, validation=cfg_validation, runTitle=cfg_runTitle, sample=cfg_sampleIndex, policy=cfg_abstractionPolicy, rerun=cfg_rerunSpurious, propDist=cfg_propDist)
    
if not cfg_dumpQueries:    
    if success:
        CnnAbs.printLog("successful={}/{}".format(successful, len(maskList)-1)) if (successful+1) < len(maskList) else CnnAbs.printLog("successful=Full")
        cnnAbs.resultsJson["successfulRuntime"] = cnnAbs.resultsJson["subResults"][-1]["runtime"]
        cnnAbs.resultsJson["successfulRun"] = successful + 1
    accumRuntime = cnnAbs.resultsJson["accumRuntime"] if (("accumRuntime" in cnnAbs.resultsJson) and cfg_slurmSeq) else 0
    cnnAbs.resultsJson["totalRuntime"] = time.time() - cnnAbs.startTotal + accumRuntime #FIXME total runtime in graphs is simply 2hr regardless of actuall acummelated runtime.
    cnnAbs.resultsJson["SAT"] = resultObj.sat()
    cnnAbs.resultsJson["Result"] = resultObj.result.name
    cnnAbs.resultsJson[mi("Result")] = resultObj.result.name
    cnnAbs.dumpResultsJson()

    CnnAbs.printLog(resultObj.result.name)
#        if cfg_doubleCheck:
#            verificationResult = verifyMarabou(modelOrigDense, resultObj.cex, resultObj.cexPrediction, resultObj.inputDict, resultObj.outputDict, "verifyMarabou", fromImage=False)
#            print("verifyMarabou={}".format(verificationResult))
#            if not verificationResult[0]:
#                raise Exception("Inconsistant Marabou result, marabou double check failed")

CnnAbs.printLog("Log files at {}".format(cnnAbs.logDir))
