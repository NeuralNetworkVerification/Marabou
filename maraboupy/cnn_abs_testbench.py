import sys
import os
import tensorflow as tf
import numpy as np
import keras2onnx
import onnx
import onnxruntime
from cnn_abs import *
import logging
import time
import argparse
import datetime
import json

#from itertools import product, chain
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
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

def dumpJson(jsonDict):
    with open("Results.json", "w") as f:
        json.dump(jsonDict, f, indent = 4)

def subResultAppend(resultsJson, runType=None, index=None, numMasks=None, runtime=None, runtimeTotal=None, originalQueryStats=None, finalQueryStats=None, sat=None):
    resultsJson["subResults"].append({"type": runType,
                                      "index" : index,
                                      "outOf" : numMasks,
                                      "runtime" : runtime,
                                      "runtimeTotal":runtimeTotal,
                                      "originalQueryStats" : originalQueryStats,
                                      "finalQueryStats" : finalQueryStats,
                                      "SAT" : sat})
    dumpJson(resultsJson)

def subResultUpdate(resultsJson, runType=None, index=None, numMasks=None, runtime=None, runtimeTotal=None, originalQueryStats=None, finalQueryStats=None, sat=None):
    resultsJson["subResults"][-1] = {"type": runType,
                                     "index" : index,
                                     "outOf" : numMasks,
                                     "runtime" : runtime,
                                     "runtimeTotal":runtimeTotal,
                                     "originalQueryStats" : originalQueryStats,
                                     "finalQueryStats" : finalQueryStats,
                                     "SAT" : sat}
    dumpJson(resultsJson)

    

defaultBatchId = "default_" + datetime.datetime.now().strftime("%d-%m-%y_%H-%M-%S")
parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--no_coi",         action="store_true",                        default=False,                  help="Don't use COI pruning")
parser.add_argument("--no_mask",        action="store_true",                        default=False,                  help="Don't use mask abstraction")
parser.add_argument("--no_verify",      action="store_true",                        default=False,                  help="Don't run verification process")
parser.add_argument("--fresh",          action="store_true",                        default=False,                  help="Retrain CNN")
parser.add_argument("--cnn_size",       type=str, choices=["big","medium","small","toy"], default="small",          help="Which CNN size to use")
parser.add_argument("--run_on",         type=str, choices=["local", "cluster"],     default="local",                help="Is the program running on cluster or local run?")
parser.add_argument("--run_title",      type=str,                                   default="default",              help="Add unique identifier identifying this current run")
parser.add_argument("--batch_id",       type=str,                                   default=defaultBatchId,         help="Add unique identifier identifying the whole batch")
parser.add_argument("--prop_distance",  type=float,                                 default=0.1,                    help="Distance checked for adversarial robustness (L1 metric)")
parser.add_argument("--num_cpu",        type=int,                                   default=8,                      help="Number of CPU workers in a cluster run.")
parser.add_argument("--sample",         type=int,                                   default=0,                      help="Index, in MNIST database, of sample image to run on.")
parser.add_argument("--policy",         type=str, choices=mnistProp.policies,       default="AllClassRank",         help="Which abstraction policy to use")
parser.add_argument("--sporious_strict",action="store_true",                        default=False,                  help="Criteria for sporious is that the original label is not achieved (no flag) or the second label is actually voted more tha the original (flag)")
parser.add_argument("--double_check"   ,action="store_true",                        default=False,                  help="Run Marabou again using recieved CEX as an input assumption.")
args = parser.parse_args()

resultsJson = dict()
    
cfg_freshModelOrig    = args.fresh
cfg_noVerify          = args.no_verify
cfg_cnnSizeChoice     = args.cnn_size
cfg_pruneCOI          = not args.no_coi
cfg_maskAbstract      = not args.no_mask
cfg_propDist          = args.prop_distance
cfg_runOn             = args.run_on
cfg_runTitle          = args.run_title
cfg_batchDir          = args.batch_id if "batch_" + args.batch_id else ""
cfg_numClusterCPUs    = args.num_cpu
cfg_abstractionPolicy = args.policy
cfg_sporiousStrict    = args.sporious_strict
cfg_sampleIndex       = args.sample
cfg_doubleCheck       = args.double_check

resultsJson["cfg_freshModelOrig"]    = cfg_freshModelOrig
resultsJson["cfg_noVerify"]          = cfg_noVerify
resultsJson["cfg_cnnSizeChoice"]     = cfg_cnnSizeChoice
resultsJson["cfg_pruneCOI"]          = cfg_pruneCOI
resultsJson["cfg_maskAbstract"]      = cfg_maskAbstract
resultsJson["cfg_propDist"]          = cfg_propDist
resultsJson["cfg_runOn"]             = cfg_runOn
resultsJson["cfg_runTitle"]          = cfg_runTitle
resultsJson["cfg_batchDir"]          = cfg_batchDir
resultsJson["cfg_numClusterCPUs"]    = cfg_numClusterCPUs
resultsJson["cfg_abstractionPolicy"] = cfg_abstractionPolicy
resultsJson["cfg_sporiousStrict"]    = cfg_sporiousStrict
resultsJson["cfg_sampleIndex"]       = cfg_sampleIndex
resultsJson["cfg_doubleCheck"]       = cfg_doubleCheck

resultsJson["SAT"] = None
resultsJson["Result"] = "TIMEOUT"
resultsJson["subResults"] = []

dumpJson(resultsJson)

cexFromImage = False

#mnistProp.runTitle = cfg_runTitle

optionsLocal = Marabou.createOptions(snc=False, verbosity=2)
optionsCluster = Marabou.createOptions(snc=True, verbosity=0, numWorkers=cfg_numClusterCPUs)
if cfg_runOn == "local":
    mnistProp.optionsObj = optionsLocal
else :
    mnistProp.optionsObj = optionsCluster

mnistProp.basePath = "/cs/labs/guykatz/matanos/Marabou/maraboupy"
currPath = mnistProp.basePath + "/logs"
if not os.path.exists(currPath):
    os.mkdir(currPath)
if cfg_batchDir:
    currPath += "/" + cfg_batchDir
    if not os.path.exists(currPath):
        os.mkdir(currPath)
if cfg_runTitle:
    currPath += "/" + cfg_runTitle
    if not os.path.exists(currPath):
        os.mkdir(currPath)        
os.chdir(currPath)
mnistProp.currPath = currPath
    
logging.basicConfig(level = logging.DEBUG, format = "%(asctime)s %(levelname)s %(message)s", filename = "cnnAbsTB.log", filemode = "w")
mnistProp.logger = logging.getLogger('cnnAbsTB_{}'.format(cfg_runTitle))
#logger.setLevel(logging.DEBUG)
mnistProp.logger.setLevel(logging.INFO)
fh = logging.FileHandler('cnnAbsTB_{}.log'.format(cfg_runTitle))
fh.setLevel(logging.DEBUG)
fh.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
mnistProp.logger.addHandler(fh)
ch = logging.StreamHandler()
ch.setLevel(logging.ERROR)
ch.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
mnistProp.logger.addHandler(ch)

logging.getLogger('matplotlib.font_manager').disabled = True

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

## Build initial model.

printLog("Started model building")
modelOrig, replaceLayerName = genCnnForAbsTest(cfg_freshModelOrig=cfg_freshModelOrig, cnnSizeChoice=cfg_cnnSizeChoice)
maskShape = modelOrig.get_layer(name=replaceLayerName).output_shape[:-1]
if maskShape[0] == None:
    maskShape = maskShape[1:]

modelOrigDense = cloneAndMaskConvModel(modelOrig, replaceLayerName, np.ones(maskShape))
#FIXME - created modelOrigDense to compensate on possible translation error when densifing. This way the abstractions are assured to be abstraction of this model.
#compareModels(modelOrig, modelOrigDense)

mnistProp.origMConv = modelOrig
mnistProp.origMDense = modelOrigDense
printLog("Finished model building")

if cfg_noVerify:
    printLog("Skipping verification phase")
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

printLog("Choosing adversarial example")

xAdv = mnistProp.x_test[cfg_sampleIndex]
yAdv = mnistProp.y_test[cfg_sampleIndex]
yPredict = modelOrigDense.predict(np.array([xAdv]))
yMax = yPredict.argmax()
yPredictNoMax = np.copy(yPredict)
yPredictNoMax[0][yMax] = np.min(yPredict)
ySecond = yPredictNoMax.argmax()
if ySecond == yMax:
    ySecond = 0 if yMax > 0 else 1
    
fName = "xAdv.png"
printLog("Printing original input to file {}, this is sample {} with label {}".format(fName, cfg_sampleIndex, yAdv))
plt.title('Example %d. Label: %d' % (cfg_sampleIndex, yAdv))
plt.savefig(fName)

resultsJson["yDataset"] = int(yAdv.item())
resultsJson["yMaxPrediction"] = int(yMax)
resultsJson["ySecondPrediction"] = int(ySecond)
dumpJson(resultsJson)    

dumpBounds(modelOrigDense, xAdv, cfg_propDist, yMax, ySecond)
if os.path.isfile(os.getcwd() + "/dumpBounds.json"):
    with open('dumpBounds.json', 'r') as boundFile:
        boundList = json.load(boundFile)
        boundDict = {bound["index"] : (bound["lower"], bound["upper"]) for bound in boundList}
else:
    boundDict = None

maskList = list(genActivationMask(intermidModel(modelOrigDense, "c2"), xAdv, yMax, policy=cfg_abstractionPolicy))
if not cfg_maskAbstract:
    maskList = []
printLog("Created {} masks".format(len(maskList)))

printLog("Strating verification phase")

reachedFull = False
successful = None
reachedFinal = False
startTotal = time.time()

for i, mask in enumerate(maskList):
    modelAbs = cloneAndMaskConvModel(modelOrig, replaceLayerName, mask)
    printLog("\n\n\n ----- Start Solving mask number {} ----- \n\n\n {} \n\n\n".format(i+1, mask))
    startLocal = time.time()
    subResultAppend(resultsJson, runType="mask", index=i+1, numMasks=len(maskList))    
    sat, cex, cexPrediction, inputDict, outputDict, originalQueryStats, finalQueryStats = runMarabouOnKeras(modelAbs, xAdv, cfg_propDist, yMax, ySecond, boundDict, "runMarabouOnKeras_mask_{}".format(i+1), coi=cfg_pruneCOI)
    subResultUpdate(resultsJson, runType="mask", index=i+1, numMasks=len(maskList), runtime=time.time() - startLocal, runtimeTotal=time.time() - startTotal, originalQueryStats=originalQueryStats, finalQueryStats=finalQueryStats, sat=sat)
    printLog("\n\n\n ----- Finished Solving mask number {} ----- \n\n\n".format(i+1))
    if sat:
        isSporious = isCEXSporious(modelOrigDense, xAdv, cfg_propDist, yMax, ySecond, cex, sporiousStrict=cfg_sporiousStrict)
        printLog("Found {} CEX in mask {}/{}.".format(i+1, len(maskList), "sporious" if isSporious else "real"))        

        if not isSporious:
            successful = i

            #FIXME check SingleClass vs AllClass
            #maskListSingleClass = list(genActivationMask(intermidModel(modelOrigDense, "c2"), xAdv, yMax, policy="SingleClassRank"))
            #modelAbsSingle = cloneAndMaskConvModel(modelOrig, replaceLayerName, maskListSingleClass[0])
            #_sat1, _cex, _cexPrediction, _inputDict, _outputDict, _originalQueryStats, _finalQueryStats = runMarabouOnKeras(modelAbsSingle, xAdv, cfg_propDist, yMax, ySecond, "runMarabouOnKeras_SingleClass_{}".format(1), coi=cfg_pruneCOI)
            #_sat2, _cex, _cexPrediction, _inputDict, _outputDict, _originalQueryStats, _finalQueryStats = runMarabouOnKeras(modelAbsSingle, xAdv, cfg_propDist, yMax, ySecond, "runMarabouOnKeras_SingleClass_{}".format(2), coi=False)
            #printLog("yMax={}, ySecond={}, AllClassPredictCEX={}, SingleClassAbsPredictCEX={}".format(yMax, ySecond, modelAbs.predict(np.array([cex])).argmax(), modelAbsSingle.predict(np.array([cex])).argmax()))
            #printLog("yPredictAll={}".format(modelAbs.predict(np.array([cex]))))
            #printLog("yPredictSingle={}".format(modelAbsSingle.predict(np.array([cex]))))
            #printLog("_sat1 (COI) = {}".format(_sat1))            
            #printLog("_sat2 (no COI) = {}".format(_sat2))            
            break;
    else:
        printLog("Found UNSAT in mask number {} out of {}".format(i+1, len(maskList)))
        successful = i
        break;
else:
    reachedFinal = True
    printLog("\n\n\n ----- Start Solving Full ----- \n\n\n")
    startLocal = time.time()
    subResultAppend(resultsJson, runType="full")
    sat, cex, cexPrediction, inputDict, outputDict, originalQueryStats, finalQueryStats = runMarabouOnKeras(modelOrigDense, xAdv, cfg_propDist, yMax, ySecond, boundDict, "runMarabouOnKeras_Full", coi=cfg_pruneCOI)
    subResultUpdate(resultsJson, runType="full", runtime=time.time() - startLocal, runtimeTotal=time.time() - startTotal, originalQueryStats=originalQueryStats, finalQueryStats=finalQueryStats, sat=sat)
    printLog("\n\n\n ----- Finished Solving Full ----- \n\n\n")
    successful = len(maskList)
    if sat:
        isSporious = isCEXSporious(modelOrigDense, xAdv, cfg_propDist, yMax, ySecond, cex, sporiousStrict=cfg_sporiousStrict)
        printLog("Found {} CEX in full network.".format("sporious" if isSporious else "real"))
        if isSporious:
            raise Exception("Sporious CEX at full network.")
    else:
        printLog("Found UNSAT in full network")

printLog("successful={}/{}".format(successful+1, len(maskList))) if successful < len(maskList) else printLog("successful=Full")
printLog("ReachedFinal={}".format(reachedFinal))

if sat:
    printLog("SAT")    
    if cfg_doubleCheck:
        verificationResult = verifyMarabou(modelOrigDense, cex, cexPrediction, inputDict, outputDict, "verifyMarabou", fromImage=cexFromImage)
        print("verifyMarabou={}".format(verificationResult))
        if not verificationResult[0]:
            raise Exception("Inconsistant Marabou result, marabou double check failed")
else:
    printLog("UNSAT")

resultsJson["SAT"] = sat
resultsJson["Result"] = "SAT" if sat else "UNSAT"    
resultsJson["totalRuntime"] = time.time() - startTotal
dumpJson(resultsJson)    

printLog("Log files at {}".format(currPath))