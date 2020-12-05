

import sys
import os
import tensorflow as tf
import numpy as np
import keras2onnx
import onnx
import onnxruntime
from cnn_abs import *
import logging

#sys.path.append("/cs/labs/guykatz/matanos/Marabou")
#sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import matplotlib.pyplot as plt

logging.basicConfig(
    level = logging.DEBUG,
    format = "%(asctime)s %(levelname)s %(message)s",
    filename = "cnnAbsTB.log",
    filemode = "w"
)
logger = logging.getLogger('cnnAbsTB')

def printLog(s):
    logger.info(s)
    print(s)

#logger.setLevel(logging.DEBUG)
logger.setLevel(logging.INFO)
fh = logging.FileHandler('cnnAbsTB.log')
fh.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.ERROR)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
logger.addHandler(fh)
logger.addHandler(ch)

printLog("Started model building")
modelOrig, replaceLayerName = genCnnForAbsTest()
maskShape = modelOrig.get_layer(name=replaceLayerName).output_shape[:-1]
if maskShape[0] == None:
    maskShape = maskShape[1:]
#FIXME - created modelOrigDense to compensate on possible translation error when densifing. This way the abstractions are assured to be abstraction of this model.
modelOrigDense = cloneAndMaskConvModel(modelOrig, replaceLayerName, np.ones(maskShape))
#compareModels(modelOrig, modelOrigDense)

mnistProp.origMConv = modelOrig
mnistProp.origMDense = modelOrigDense
printLog("Finished model building")

printLog("Choosing adversarial example")
inDist = 0.1
xAdvInds = range(mnistProp.numTestSamples)
xAdvInd = xAdvInds[0]
xAdv = mnistProp.x_test[xAdvInd]
yAdv = mnistProp.y_test[xAdvInd]
yPredict = modelOrigDense.predict(np.array([xAdv]))
yMax = yPredict.argmax()
yPredictNoMax = np.copy(yPredict)
yPredictNoMax[0][yMax] = 0
ySecond = yPredictNoMax.argmax()
if ySecond == yMax:
    ySecond = 0 if yMax > 0 else 1

yPredictUnproc = modelOrig.predict(np.array([xAdv]))
yMaxUnproc = yPredictUnproc.argmax()
yPredictNoMaxUnproc = np.copy(yPredictUnproc)
yPredictNoMaxUnproc[0][yMaxUnproc] = 0
ySecondUnproc = yPredictNoMaxUnproc.argmax()
if ySecondUnproc == yMaxUnproc:
    ySecondUnproc = 0 if yMaxUnproc > 0 else 1
    
fName = "xAdv.png"
printLog("Printing original input: {}".format(fName))
plt.title('Example %d. Label: %d' % (xAdvInd, yAdv))
plt.imshow(xAdv.reshape(xAdv.shape[:-1]), cmap='Greys')
plt.savefig(fName)

printLog("Starting Abstractions")

maskList = list(genActivationMask(intermidModel(modelOrigDense, "c2"), xAdv, yMax))
maskList = [mask for mask in maskList if np.any(np.not_equal(mask, np.ones(mask.shape)))]
printLog("Created {} masks".format(len(maskList)))

currentMbouRun = 0
isSporious = False
reachedFull = False
successful = None
reachedFinal = False
for i, mask in enumerate(maskList):
    modelAbs = cloneAndMaskConvModel(modelOrig, replaceLayerName, mask)
    sat, cex, cexPrediction, inputDict, outputDict = runMarabouOnKeras(modelAbs, logger, xAdv, inDist, yMax, ySecond, "IPQ_runMarabouOnKeras_{}".format(currentMbouRun))
    currentMbouRun += 1
    isSporious = None
    if sat:
        printLog("Found CEX in mask number {} out of {}, checking if sporious.".format(i, len(maskList)))
        isSporious = isCEXSporious(modelOrigDense, xAdv, inDist, yMax, ySecond, cex, logger)
        printLog("CEX in mask number {} out of {} is {}sporious.".format(i, len(maskList), "" if isSporious else "not "))        
        if not isSporious:
            printLog("Found real CEX in mask number {} out of {}".format(i, len(maskList)))
            printLog("successful={}/{}".format(i, len(maskList)))
            successful = i
            break;
    else:
        printLog("Found UNSAT in mask number {} out of {}".format(i, len(maskList)))
        printLog("successful={}/{}".format(i, len(maskList)))
        successful = i
        break;
else:
    reachedFinal = True
    sat, cex, cexPrediction, inputDict, outputDict = runMarabouOnKeras(modelOrigDense, logger, xAdv, inDist, yMax, ySecond, "IPQ_runMarabouOnKeras_{}".format(currentMbouRun))
    currentMbouRun += 1    

if sat:
    printLog("SAT, reachedFinal={}".format(reachedFinal))
    if isCEXSporious(modelOrigDense, xAdv, inDist, yMax, ySecond, cex, logger):
        assert reachedFinal
        printLog("Sporious CEX after end")
        fromImage = False        
        if fromImage:
            probDistEq, predictionEq, marabouDist = verifyMarabou(modelOrigDense, cex, cexPrediction, inputDict, outputDict, "IPQ_verifyMarabou_{}".format(currentMbouRun-1), fromImage=True)
            print("verifyMarabou={}".format((probDistEq, predictionEq, marabouDist)))
            if predictionEq:
                raise Exception("Sporious CEX after end with verified Marabou result")
            else:            
                raise Exception("inconsistant Marabou result.")
        else:
            outDictEq = verifyMarabou(modelOrigDense, cex, cexPrediction, inputDict, outputDict, "IPQ_verifyMarabou_{}".format(currentMbouRun-1), fromImage=False)
            print("verifyMarabou={}".format(outDictEq))
            if outDictEq:
                raise Exception("Sporious CEX after end with verified Marabou result")
            else:            
                raise Exception("inconsistant Marabou result.")
    if modelOrigDense.predict(np.array([cex])).argmax() != ySecond:
        printLog("Unxepcted prediction result, cex result is {}".format(modelOrigDense.predict(np.array([cex])).argmax()))
    printLog("Found CEX in origin")
    printLog("successful=original")
    printLog("SAT")
else:
    printLog("UNSAT")
    #printLog("verifying UNSAT on unprocessed network")
    #FIXME this is not exactly the same query as the proccessed one.
    #sat, cex, cexPrediction = runMarabouOnKeras(modelOrig, logger, xAdv, inDist, yMaxUnproc, ySecondUnproc)
    #if not sat:
    #    printLog("Proved UNSAT on unprocessed network")
    #else:
    #    printLog("Found CEX on unprocessed network")


