import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#pip install keras2onnx
import keras2onnx
import tensorflow as tf
from maraboupy import MarabouNetworkONNX as monnx
from maraboupy import MarabouCore
from maraboupy import MarabouUtils
from maraboupy import Marabou
from tensorflow.keras import datasets, layers, models
import numpy as np
import logging
import matplotlib.pyplot as plt
from enum import Enum
import json
import random

from tensorflow.keras.models import load_model
cfg_freshModelAbs = True
savedModelAbs = "cnn_abs_abs.h5"
class mnistProp:
    num_classes = 10
    input_shape = (28,28,1)
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    x_train, x_test = x_train / 255.0, x_test / 255.0
    x_train = np.expand_dims(x_train, -1)
    x_test = np.expand_dims(x_test, -1)
    featureShape=(1,28,28)
    loss='sparse_categorical_crossentropy'
    optimizer='adam'
    metrics=['accuracy']
    savedModelAbs = "cnn_abs_abs.h5"
    cfg_dis_w = False
    cfg_dis_b = False
    cfg_fresh = False
    numClones = 0
    numInputQueries = 0
    numCex = 0
    origMSize = None
    origMDense = None
    absPolicies = ["Centered", "AllClassRank", "SingleClassRank", "MajorityClassVote", "Random"]
    policies = absPolicies + ["FindMinProvable"]
    Policy = Enum("Policy"," ".join(policies))
    optionsObj = None
    runSuffix = ""
    logger = None
    basePath = None
    currPath = None
    stepSize = 10
    startWith = 5 * stepSize
    dumpDir = ""

    def output_model_path(m, suffix=""):
        if suffix:
            suffix = "_" + suffix
        return "./{}{}.onnx".format(m.name, suffix)

    def printDictToFile(dic, fName):
        with open(fName,"w") as f:
            for i,x in dic.items():
                f.write("{},{}\n".format(i,x))


def marabouNetworkStats(net):
    return {"numVars" : net.numVars,
            "numEquations" : len(net.equList),
            "numReluConstraints" : len(net.reluList),
            "numMaxConstraints" : len(net.maxList),
            "numAbsConstraints" : len(net.absList),
            "numSignConstraints" : len(net.signList),
            "numDisjunction" : len(net.disjunctionList),
            "numLowerBounds" : len(net.lowerBounds),
            "numUpperBounds" : len(net.upperBounds),
            "numInputVars" : sum([np.array(inputVars).size for inputVars in net.inputVars]),
            "numOutputVars" : net.outputVars.size}

#replaceW, replaceB = maskAndDensifyNDimConv(np.ones((2,2,1,1)), np.array([0.5]), np.ones((3,3,1)), (3,3,1), (3,3,1), (1,1))
def maskAndDensifyNDimConv(origW, origB, mask, convInShape, convOutShape, strides, cfg_dis_w=mnistProp.cfg_dis_w):
    #https://stackoverflow.com/questions/36966392/python-keras-how-to-transform-a-dense-layer-into-a-convolutional-layer
    if convOutShape[0] == None:
        convOutShape = convOutShape[1:]
    if convInShape[0] == None:
        convInShape = convInShape[1:]
    replaceW = np.zeros((np.prod(convInShape), np.prod(convOutShape)))
    replaceB = np.zeros(np.prod(convOutShape))
    fDim = origW.shape[:-2] # W/O in/out channels.

    sOff = [int(np.prod(convInShape[i+1:]))  for i in range(len(convInShape))]
    tOff = [int(np.prod(convOutShape[i+1:])) for i in range(len(convOutShape))]

    tCoors = product(*[range(d) for d in convOutShape[:-1]])
    inChNum = origW.shape[-2]
    outChNum = origW.shape[-1]
    for tCoor in tCoors:
        if mask[tCoor]:
            sCoors = product(*[[coor for coor in [j*s+t for j in range(f)] if coor < i] for f,s,t,i in zip(fDim, strides, tCoor, convInShape)])
            sCoors = list(sCoors)
            wMats  = [origW[coor] for coor in product(*[range(d) for d in fDim])]
            for sCoor, wMat in zip(sCoors, wMats):
                for in_ch, out_ch in product(range(inChNum), range(outChNum)):
                    sCoorFlat = sum([coor * off for coor, off in zip((*sCoor, in_ch) , sOff)])
                    tCoorFlat = sum([coor * off for coor, off in zip((*tCoor, out_ch), tOff)])
                    if wMat[in_ch, out_ch].shape != ():
                        raise Exception("wMat[x,y] values should be scalars. shape={}".format( wMat[in_ch, out_ch].shape))
                    replaceW[sCoorFlat, tCoorFlat] = np.ones(wMat[in_ch, out_ch].shape) if cfg_dis_w else wMat[in_ch, out_ch].item()
                    replaceB[tCoorFlat] = origB[out_ch]

    return replaceW, replaceB

def cloneAndMaskConvModel(origM, rplcLayerName, mask):
    rplcIn = origM.get_layer(name=rplcLayerName).input_shape
    rplcOut = origM.get_layer(name=rplcLayerName).output_shape
    origW = origM.get_layer(name=rplcLayerName).get_weights()[0]
    origB = origM.get_layer(name=rplcLayerName).get_weights()[1]
    strides = origM.get_layer(name=rplcLayerName).strides
    clnW, clnB = maskAndDensifyNDimConv(origW, origB, mask, rplcIn, rplcOut, strides)
    clnLayers = [tf.keras.Input(shape=mnistProp.input_shape, name="input_clnM")]
    toSetWeights = {}
    lSuffix = "_clnM_{}_{}".format(mnistProp.runSuffix, mnistProp.numClones)
    for l in origM.layers:
        if l.name == rplcLayerName:
            clnLayers.append(layers.Flatten(name=(rplcLayerName + "_f_rplc" + lSuffix)))
            clnLayers.append(layers.Dense(units=np.prod(rplcOut[1:]), activation=l.activation, name=(rplcLayerName + "_rplcConv" + lSuffix)))
            toSetWeights[(rplcLayerName + "_rplcConv" + lSuffix)] = [clnW, clnB]
            clnLayers.append(layers.Reshape(rplcOut[1:], name=(rplcLayerName + "_rshp_rplcOut" + lSuffix)))
        else:
            if isinstance(l, tf.keras.layers.Dense):
                newL = tf.keras.layers.Dense(l.units, activation=l.activation, name=(l.name + lSuffix))
            elif isinstance(l, tf.keras.layers.Conv2D):
                newL = tf.keras.layers.Conv2D(l.get_weights()[0].shape[3], kernel_size=l.get_weights()[0].shape[0:2], activation=l.activation, name=(l.name + lSuffix))
            elif isinstance(l, tf.keras.layers.MaxPooling2D):
                newL = tf.keras.layers.MaxPooling2D(pool_size=l.pool_size, name=(l.name + lSuffix))
            elif isinstance(l, tf.keras.layers.Flatten):
                newL = tf.keras.layers.Flatten(name=(l.name + lSuffix))
            elif isinstance(l, tf.keras.layers.Dropout):
                newL = tf.keras.layers.Dropout(l.rate, name=(l.name + lSuffix))
            else:
                raise Exception("Not implemented")
            toSetWeights[newL.name] = l.get_weights()
            clnLayers.append(newL)
    clnM = tf.keras.Sequential(
        clnLayers,
        name=("AbsModel_{}".format(mnistProp.numClones))
    )
    mnistProp.numClones += 1

    clnM.build(input_shape=mnistProp.featureShape)
    clnM.compile(loss=mnistProp.loss, optimizer=mnistProp.optimizer, metrics=mnistProp.metrics)
    clnM.summary()

    for l,w in toSetWeights.items():
        clnM.get_layer(name=l).set_weights(w)
    score = clnM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
    print("(Clone, neurons masked:{}%) Test loss:".format(100*(1 - np.average(mask))), score[0])
    print("(Clone, neurons masked:{}%) Test accuracy:".format(100*(1 - np.average(mask))), score[1])

    if np.all(np.equal(mask, np.ones_like(mask))):
        if np.all(np.isclose(clnM.predict(mnistProp.x_test), origM.predict(mnistProp.x_test))):
            #if np.all(np.equal(clnM.predict(mnistProp.x_test), origM.predict(mnistProp.x_test))):
            print("Prediction aligned")
        else:
            print("Prediction not aligned")

    return clnM

def myLoss(labels, logits):
    return tf.keras.losses.sparse_categorical_crossentropy(labels, logits, from_logits=True)

def printLog(s):
    mnistProp.logger.info(s)
    print(s)


def genCnnForAbsTest(cfg_limitCh=True, cfg_freshModelOrig=mnistProp.cfg_fresh, savedModelOrig="cnn_abs_orig.h5", cnnSizeChoice = "small"):

    print("Starting model building")
    #https://keras.io/examples/vision/mnist_convnet/

    savedModelOrig = savedModelOrig.replace(".h5", "_" + cnnSizeChoice + ".h5")

    if cfg_freshModelOrig:
        if cnnSizeChoice in ["big", "big_validation"]:
            num_ch = 32
        elif cnnSizeChoice in ["medium", "medium_validation"]:
            num_ch = 16
        elif cnnSizeChoice in ["small", "small_validation"]:
            num_ch = 1
        elif cnnSizeChoice == "toy":
            raise Exception("Toy network is not meant to be retrained")
        else:
            raise Exception("cnnSizeChoice {} not supported".format(cnnSizeChoice))
        origM = tf.keras.Sequential(
            [
                tf.keras.Input(shape=mnistProp.input_shape),
                layers.Conv2D(num_ch, kernel_size=(3, 3), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                layers.Conv2D(num_ch, kernel_size=(2, 2), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                layers.Dense(mnistProp.num_classes, activation=None, name="sm1"),
            ],
            name="origModel_" + cnnSizeChoice
        )

        batch_size = 128
        epochs = 15

        origM.build(input_shape=mnistProp.featureShape)
        origM.summary()

        origM.compile(optimizer=mnistProp.optimizer, loss=myLoss, metrics=[tf.keras.metrics.SparseCategoricalAccuracy()])
        #origM.compile(optimizer=mnistProp.optimizer, loss=mnistProp.loss, metrics=mnistProp.metrics)
        origM.fit(mnistProp.x_train, mnistProp.y_train, epochs=epochs, batch_size=batch_size, validation_split=0.1)

        score = origM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print("(Original) Test loss:", score[0])
        print("(Original) Test accuracy:", score[1])

        origM.save(mnistProp.basePath + "/" + savedModelOrig)

    else:
        origM = load_model(mnistProp.basePath + "/" + savedModelOrig, custom_objects={'myLoss': myLoss})
        #origM = load_model(mnistProp.basePath + "/" + savedModelOrig)
        origM.summary()
        score = origM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print("(Original) Test loss:", score[0])
        print("(Original) Test accuracy:", score[1])

    rplcLayerName = "c2"
    return origM, rplcLayerName

def getBoundsInftyBall(x, r, pos=True):
    if pos:
        return np.maximum(x - r,np.zeros(x.shape)), x + r
    return x - r, x + r

def inBoundsInftyBall(x, r, p, pos=True):
    l,u = getBoundsInftyBall(x,r,pos=pos)
    #lowerBoundCond = np.less_equal(l,p)
    #upperBoundCond = np.less_equal(p,u)
    #print("lowerBoundCond={}".format(lowerBoundCond))
    #print("upperBoundCond={}".format(upperBoundCond))
#    if not np.all(lowerBoundCond):
#        print("np.where(np.less(p,l)={}".format(np.where(np.less(p,l))))
#        xa, ya, za = np.where(np.less(p,l))
#        for ind in zip(np.nditer(xa), np.nditer(ya), np.nditer(za)):
#            print("l[ind]={}, p[ind]={}".format(l[ind], p[ind]))
#    if not np.all(upperBoundCond):
#        print("np.where(np.less(u,p)={}".format(np.where(np.less(u,p))))
#        xa, ya, za = np.where(np.less(u,p))
#        for ind in zip(np.nditer(xa), np.nditer(ya), np.nditer(za)):
#            print("p[ind]={}, u[ind]={}".format(p[ind], u[ind]))
    ###return np.all(np.less_equal(l,p)) and np.all(np.less_equal(p,u))
    geqLow = np.logical_or(np.less_equal(l,p), np.isclose(l,p))    
    leqUp = np.logical_or(np.less_equal(p,u), np.isclose(p,u))
    inBounds = np.logical_and(geqLow, leqUp)
    violations = np.logical_not(inBounds)
    return np.all(inBounds), violations #FIXME shouldn't allow isclose, floating point errors?

def setAdversarial(net, x, inDist, outSlack, yCorrect, yBad):
    inAsNP = np.array(net.inputVars[0])
    x = x.reshape(inAsNP.shape)
    xDown, xUp = getBoundsInftyBall(x, inDist)
    for i,d,u in zip(np.nditer(inAsNP),np.nditer(xDown),np.nditer(xUp)):
        net.lowerBounds.pop(i.item(), None)
        net.upperBounds.pop(i.item(), None)
        net.setLowerBound(i.item(),d.item())
        net.setUpperBound(i.item(),u.item())
    for j,o in enumerate(np.nditer(np.array(net.outputVars))):
        if j == yCorrect:
            yCorrectVar = o.item()
        if j == yBad:
            yBadVar = o.item()
    net.addInequality([yCorrectVar, yBadVar], [1,-1], outSlack) # correct - bad <= slack
    return net

def cexToImage(net, valDict, xAdv, inDist, inputVarsMapping=None, outputVarsMapping=None, useMapping=True):
    if useMapping:
        lBounds = getBoundsInftyBall(xAdv, inDist)[0]

        inputDict = {indOrig : valDict[indCOI.item()] if indCOI.item() != -1 else lBnd for (indOrig,indCOI),lBnd in zip(enumerate(np.nditer(np.array(inputVarsMapping), flags=["refs_ok"])), np.nditer(lBounds))}
        outputDict = {indOrig : valDict[indCOI.item()] if indCOI.item() != -1 else 0 for indOrig, indCOI in enumerate(np.nditer(np.array(outputVarsMapping), flags=["refs_ok"]))}

        cex           = np.array([valDict[i.item()] if i.item() != -1 else lBnd for i,lBnd in zip(np.nditer(np.array(inputVarsMapping), flags=["refs_ok"]), np.nditer(lBounds))]).reshape(xAdv.shape)
        cexPrediction = np.array([valDict[o.item()] if o.item() != -1 else 0 for o in np.nditer(np.array(outputVarsMapping), flags=["refs_ok"])]).reshape(outputVarsMapping.shape)
    else: #FIXME not compatible with case where net is an InputQuery
        inputDict = {i.item():valDict[i.item()] for i in np.nditer(np.array(net.inputVars[0]))}
        outputDict = {o.item():valDict[o.item()] for o in np.nditer(np.array(net.outputVars))}
        cex = np.array([valDict[i.item()] for i in np.nditer(np.array(net.inputVars[0]))]).reshape(xAdv.shape)
        cexPrediction = np.array([valDict[o.item()] for o in np.nditer(np.array(net.outputVars))])
    return cex, cexPrediction, inputDict, outputDict

def setUnconnectedAsInputs(net):
    varsWithIngoingEdgesOrInputs = set([v.item() for nparr in net.inputVars for v in np.nditer(nparr)])
    for eq in net.equList:
        if eq.EquationType == MarabouCore.Equation.EQ and eq.addendList[-1][0] == -1 and any([el[0] != 0 for el in eq.addendList[:-1]]):
            varsWithIngoingEdgesOrInputs.add(eq.addendList[-1][1])
    for maxCons in net.maxList:
        varsWithIngoingEdgesOrInputs.add(maxCons[1])
    for reluCons in net.reluList:
        varsWithIngoingEdgesOrInputs.add(reluCons[1])
    for signCons in net.signList:
        varsWithIngoingEdgesOrInputs.add(signCons[1])
    for absCons in net.absList:
        varsWithIngoingEdgesOrInputs.add(absCons[1])
    varsWithoutIngoingEdges = {v for v in range(net.numVars) if v not in varsWithIngoingEdgesOrInputs}
    print("varsWithoutIngoingEdges = {}".format(varsWithoutIngoingEdges))
    for v in varsWithoutIngoingEdges: #FIXME this isn't working
        if not net.lowerBoundExists(v):
            print("inaccessible w/o lower bounds: {}".format(v))
####            net.setLowerBound(v, -100000)
        if not net.upperBoundExists(v):
            print("inaccessible w/o upper bounds: {}".format(v))            
####            net.setUpperBound(v,  100000)
    '''# This is to make deeppoly work. Setting aux=b when b is in an input entreing a relu.
    #dontKeep = set()
    for reluCons in net.reluList:
        if reluCons[0] in varsWithoutIngoingEdges:
            varsWithoutIngoingEdges.remove(reluCons[0])
            auxVar = net.numVars
            auxEq = MarabouUtils.Equation()
            auxEq.scalar = 0
            auxEq.addendList = [(1, reluCons[0]), (-1, auxVar)]
            net.equList.append(auxEq)
            net.numVars += 1
            varsWithoutIngoingEdges.add(auxVar)'''
    net.inputVars.append(np.array([v for v in varsWithoutIngoingEdges]))
    #    if reluCons[0] in varsWithoutIngoingEdges:
    #        varsWithoutIngoingEdges.remove(reluCons[0])
    #        dontKeep.add(reluCons[0])
    #        varsWithoutIngoingEdges.add(reluCons[1])
    #net.inputVars.append(np.array([v for v in varsWithoutIngoingEdges]))
    #return removeRedundantVariables(net, dontKeep, keepSet=False)
    ##[net.inputVars.append(np.array([v])) for v in varsWithoutIngoingEdges]

def removeRedundantVariables(net, varSet, keepSet=True): # If keepSet then remove every variable not in keepSet. Else, remove variables in varSet.
    if not keepSet:        
        net.reluList = [(vin,vout) for vin,vout in net.reluList if (vin not in varSet) and (vout not in varSet)]
        net.absList  = [(vin,vout) for vin,vout in net.absList  if (vin not in varSet) and (vout not in varSet)]
        net.signList = [(vin,vout) for vin,vout in net.signList if (vin not in varSet) and (vout not in varSet)]
        varSet = {v for v in range(net.numVars) if v not in varSet}

    varSetList = list(varSet)
    varSetList.sort()
    varSetDict = {v:i for i,v in enumerate(varSetList)}
    assert varSet == set(varSetDict.keys())
    tr = lambda v: varSetDict[v] if v in varSetDict else -1
    varsMapping = {v : tr(v) for v in range(net.numVars)}
    
    if keepSet:
        for vin,vout in net.reluList:
            assert (vin not in varSet) == (vout not in varSet)
        for vin,vout in net.absList:
            assert (vin not in varSet) == (vout not in varSet)
        for vin,vout in net.signList:
            assert (vin not in varSet) == (vout not in varSet)

    newEquList = list()
    for eq in net.equList:
        if (eq.EquationType == MarabouCore.Equation.EQ) and (eq.addendList[-1][0] == -1):
            newEq = MarabouUtils.Equation()
            newEq.scalar = eq.scalar
            newEq.EquationType = MarabouCore.Equation.EQ
            newEq.addendList = [(el[0],tr(el[1])) for el in eq.addendList if el[1] in varSet]
            if (eq.addendList[-1][1] not in varSet) or len(newEq.addendList) == 1:
                continue
            if all([el[0] == 0 for el in eq.addendList[:-1]]):
                continue
            if newEq.addendList:
                newEquList.append(newEq)
        else:
            newEq = MarabouUtils.Equation()
            newEq.scalar = eq.scalar
            newEq.EquationType = eq.EquationType
            newEq.addendList = [(el[0],tr(el[1])) for el in eq.addendList]
            newEquList.append(newEq)
    net.equList  = newEquList
    net.maxList  = [({tr(arg) for arg in maxArgs if arg in varSet}, tr(maxOut)) for maxArgs, maxOut in net.maxList if (maxOut in varSet and any([arg in varSet for arg in maxArgs]))]
    net.reluList = [(tr(vin),tr(vout)) for vin,vout in net.reluList if vout in varSet]
    net.absList  = [(tr(vin),tr(vout)) for vin,vout in net.absList  if vout in varSet]
    net.signList = [(tr(vin),tr(vout)) for vin,vout in net.signList if vout in varSet]
    net.lowerBounds = {tr(v):l for v,l in net.lowerBounds.items() if v in varSet}
    net.upperBounds = {tr(v):u for v,u in net.upperBounds.items() if v in varSet}
    inputVarsMapping = np.array([tr(v) for v in net.inputVars[0].flatten().tolist()]).reshape(net.inputVars[0].shape)
    outputVarsMapping = np.array([tr(v) for v in net.outputVars.flatten().tolist()]).reshape(net.outputVars.shape)
    net.inputVars  = [np.array([tr(v) for v in net.inputVars[0].flatten().tolist()  if v in varSet])]
    net.outputVars = np.array([tr(v) for v in net.outputVars.flatten().tolist() if v in varSet])
    net.numVars = len(varSetList)
    return inputVarsMapping, outputVarsMapping, varsMapping
    
def setCOIBoundes(net, init):

    print("len(net.equList)={}".format(len(net.equList)))
    print("len(net.maxList)={}".format(len(net.maxList)))
    print("len(net.reluList)={}".format(len(net.reluList)))
    print("len(net.absList)={}".format(len(net.absList)))
    print("len(net.signList)={}".format(len(net.signList)))
    print("len(net.lowerBounds)={}".format(len(net.lowerBounds)))
    print("len(net.upperBounds)={}".format(len(net.upperBounds)))
    print("len(net.inputVars)={}".format(len(net.inputVars)))
    print("len(net.outputVars)={}".format(len(net.outputVars)))

    reach = set(init)
    lastLen = 0
    while len(reach) > lastLen:
        lastLen = len(reach)
        reachPrev = reach.copy()
        for eq in net.equList:
            if (eq.addendList[-1][1] in reachPrev) and (eq.addendList[-1][0] == -1) and (eq.EquationType == MarabouCore.Equation.EQ):
                [reach.add(v) for w,v in eq.addendList[:-1] if w != 0]
            elif (eq.EquationType != MarabouCore.Equation.EQ) or (eq.addendList[-1][0] != -1):
                [reach.add(v) for w,v in eq.addendList]
                print("eq.addendList={}, eq.scalar={}, eq.EquationType={}".format(eq.addendList, eq.scalar, eq.EquationType))
        for maxArgs, maxOut in net.maxList:
            if maxOut in reachPrev:
                [reach.add(arg) for arg in maxArgs]
        [reach.add(vin) for vin,vout in net.reluList if vout in reachPrev]
        [reach.add(vin) for vin,vout in net.absList  if vout in reachPrev]
        [reach.add(vin) for vin,vout in net.signList if vout in reachPrev]
        if len(net.disjunctionList) > 0:
            raise Exception("Not implemented")
    unreach = set([v for v in range(net.numVars) if v not in reach])

    print("COI : reached={}, unreached={}, out_of={}".format(len(reach), len(unreach), net.numVars))

    inputVarsMapping, outputVarsMapping, varsMapping = removeRedundantVariables(net, reach)
    for eq in net.equList:
        for w,v in eq.addendList:
            if v > net.numVars:
                print("eq.addendList={}, eq.scalar={}, eq.EquationType={}".format(eq.addendList, eq.scalar, eq.EquationType))
    print("len(net.equList)={}".format(len(net.equList)))
    print("len(net.maxList)={}".format(len(net.maxList)))
    print("len(net.reluList)={}".format(len(net.reluList)))
    print("len(net.absList)={}".format(len(net.absList)))
    print("len(net.signList)={}".format(len(net.signList)))
    print("len(net.lowerBounds)={}".format(len(net.lowerBounds)))
    print("len(net.upperBounds)={}".format(len(net.upperBounds)))
    print("len(net.inputVars)={}".format(len(net.inputVars)))
    print("len(net.outputVars)={}".format(len(net.outputVars)))
    print("COI : reached={}, unreached={}, out_of={}".format(len(reach), len(unreach), net.numVars))
    return inputVarsMapping, outputVarsMapping, varsMapping

def dumpBounds(model, xAdv, inDist, outSlack, yMax, ySecond):
    modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=(1 if mnistProp.logger.level==logging.DEBUG else 0))
    modelOnnxName = mnistProp.output_model_path(model)
    keras2onnx.save_model(modelOnnx, modelOnnxName)
    modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
    setAdversarial(modelOnnxMarabou, xAdv, inDist, outSlack, yMax, ySecond)
    return processInputQuery(modelOnnxMarabou)

def processInputQuery(net):
    return MarabouCore.preprocess(net.getMarabouQuery(), mnistProp.optionsObj)

def setBounds(model, boundDict):
    if boundDict:
        for i, (lb, ub) in boundDict.items():
            if i < model.numVars: #FIXME This might mean that there is disalignment between the queries' definition of variables                
                if (not i in model.lowerBounds) or (model.lowerBounds[i] < lb):
                    model.setLowerBound(i,lb)
                if (not i in model.upperBounds) or (ub < model.upperBounds[i]):
                    model.setUpperBound(i,ub)

def runMarabouOnKeras(model, xAdv, inDist, outSlack, yMax, ySecond, boundDict, runName="runMarabouOnKeras", coi=True, mask=True, onlyDump=False, fromDumpedQuery=False):
    if not fromDumpedQuery:
        modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=(1 if mnistProp.logger.level==logging.DEBUG else 0))
        modelOnnxName = mnistProp.output_model_path(model)
        keras2onnx.save_model(modelOnnx, modelOnnxName)
        modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
        setAdversarial(modelOnnxMarabou, xAdv, inDist, outSlack, yMax, ySecond)
        setBounds(modelOnnxMarabou, boundDict)
        originalQueryStats = marabouNetworkStats(modelOnnxMarabou)
        with open(mnistProp.dumpDir + "originalQueryStats_" + runName + ".json", "w") as f:
            json.dump(originalQueryStats, f, indent = 4)
        if coi:
            inputVarsMapping, outputVarsMapping, varsMapping = setCOIBoundes(modelOnnxMarabou, modelOnnxMarabou.outputVars.flatten().tolist())
            plt.title('COI_{}'.format(runName))
            plt.imshow(np.array([0 if i == -1 else 1 for i in np.nditer(inputVarsMapping.flatten())]).reshape(inputVarsMapping.shape[1:-1]), cmap='Greys')
            plt.savefig('COI_{}'.format(runName))
        else:
            inputVarsMapping, outputVarsMapping, varsMapping = None, None, None
        with open(mnistProp.dumpDir + "inputVarsMapping_" + runName + ".npy", "wb") as f:
            np.save(f, inputVarsMapping)
        with open(mnistProp.dumpDir + "outputVarsMapping_" + runName + ".npy", "wb") as f:
            np.save(f, outputVarsMapping)
        with open(mnistProp.dumpDir + "varsMapping_" + runName + ".json", "w") as f:
            json.dump(varsMapping, f, indent=4)
        setUnconnectedAsInputs(modelOnnxMarabou)
        #inputVarsMapping2, outputVarsMapping2 =setUnconnectedAsInputs(modelOnnxMarabou)
        modelOnnxMarabou.saveQuery(mnistProp.dumpDir + "IPQ_" + runName)
        finalQueryStats = marabouNetworkStats(modelOnnxMarabou)
        with open(mnistProp.dumpDir + "finalQueryStats_" + runName + ".json", "w") as f:
            json.dump(finalQueryStats, f, indent = 4)
        if onlyDump:
            return "IPQ_" + runName            
        vals, stats = modelOnnxMarabou.solve(verbose=False, options=mnistProp.optionsObj)
    else:
        ipq = Marabou.load_query(mnistProp.dumpDir + "IPQ_" + runName)
        vals, stats = Marabou.solve_query(ipq, verbose=False, options=mnistProp.optionsObj)
    sat = len(vals) > 0
    timedOut = stats.hasTimedOut()
    if fromDumpedQuery:
        with open(mnistProp.dumpDir + "originalQueryStats_" + runName + ".json", "r") as f:
            originalQueryStats = json.load(f)
        with open(mnistProp.dumpDir + "finalQueryStats_" + runName + ".json", "r") as f:
            finalQueryStats = json.load(f)
        with open(mnistProp.dumpDir + "inputVarsMapping_" + runName + ".npy", "rb") as f:            
            inputVarsMapping = np.load(f, allow_pickle=True)
        with open(mnistProp.dumpDir + "outputVarsMapping_" + runName + ".npy", "rb") as f:
            outputVarsMapping = np.load(f, allow_pickle=True)
        with open(mnistProp.dumpDir + "varsMapping_" + runName + ".json", "r") as f:
            varsMapping = json.load(f)            
    if not sat:            
        return False, timedOut, np.array([]), np.array([]), dict(), dict(), originalQueryStats, finalQueryStats
    if not fromDumpedQuery:
        cex, cexPrediction, inputDict, outputDict = cexToImage(modelOnnxMarabou, vals, xAdv, inDist, inputVarsMapping, outputVarsMapping, useMapping=coi)
    else:
        assert coi #FIXME sat and Full isn't working now.
        cex, cexPrediction, inputDict, outputDict = cexToImage(ipq, vals, xAdv, inDist, inputVarsMapping, outputVarsMapping, useMapping=coi)
    fName = "Cex_{}.png".format(runName)
    mnistProp.numCex += 1
    mbouPrediction = cexPrediction.argmax()
    if not fromDumpedQuery: #FIXME - I need everything to work in the general and dumped mode.
        kerasPrediction = model.predict(np.array([cex])).argmax()
        if mbouPrediction != kerasPrediction:
            origM, replaceLayer = genCnnForAbsTest(cfg_freshModelOrig=False, cnnSizeChoice=mnistProp.origMSize)
            origMConvPrediction = origM.predict(np.array([cex])).argmax()
            origMDensePrediction = mnistProp.origMDense.predict(np.array([cex])).argmax()
            print("Marabou and keras doesn't predict the same class. mbouPrediction ={}, kerasPrediction={}, origMConvPrediction={}, origMDensePrediction={}".format(mbouPrediction, kerasPrediction, origMConvPrediction, origMDensePrediction))
    else: #FIXME should load the modelAbs and check the prediction.
        kerasPrediction = None
    plt.title('CEX, yMax={}, ySecond={}, MarabouPredictsCEX={}, modelPredictsCEX={}'.format(yMax, ySecond, mbouPrediction, kerasPrediction))
    plt.imshow(cex.reshape(xAdv.shape[:-1]), cmap='Greys')
    plt.savefig(fName)
    mnistProp.printDictToFile(inputDict, "DICT_runMarabouOnKeras_InputDict")
    mnistProp.printDictToFile(outputDict, "DICT_runMarabouOnKeras_OutputDict")
    return True, timedOut, cex, cexPrediction, inputDict, outputDict, originalQueryStats, finalQueryStats

def verifyMarabou(model, xAdv, xPrediction, inputDict, outputDict, runName="verifyMarabou", fromImage=False):
    mnistProp.printDictToFile(inputDict, "DICT_verifyMarabou_InputDict_in")
    mnistProp.printDictToFile(outputDict, "DICT_verifyMarabou_OutputDict_in")
    modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=0)
    modelOnnxName = mnistProp.output_model_path(model)
    keras2onnx.save_model(modelOnnx, modelOnnxName)
    modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
    if fromImage:
        inAsNP = np.array(modelOnnxMarabou.inputVars[0])
        xAdv = xAdv.reshape(inAsNP.shape)
        for i,x in zip(np.nditer(inAsNP),np.nditer(xAdv)):
            modelOnnxMarabou.setLowerBound(i.item(),x.item())
            modelOnnxMarabou.setUpperBound(i.item(),x.item())
    else:
        for i,x in inputDict.items():
            modelOnnxMarabou.setLowerBound(i,x)
            modelOnnxMarabou.setUpperBound(i,x)
    vals, stats = modelOnnxMarabou.solve(verbose=True, options=mnistProp.optionsObj)
    modelOnnxMarabou.saveQuery(runName)
    if fromImage:
        predictionMbou = np.array([vals[o.item()] for o in np.nditer(np.array(modelOnnxMarabou.outputVars))])
        print("predictionMbou={}".format(predictionMbou))
        print("xPrediction={}".format(xPrediction))
        return xPrediction.argmax() == predictionMbou.argmax(), np.all(xPrediction == predictionMbou), predictionMbou
    else:
        inputDictInner = {i.item():vals[i.item()] for i in np.nditer(np.array(modelOnnxMarabou.inputVars[0]))}
        outputDictInner = {o.item():vals[o.item()] for o in np.nditer(np.array(modelOnnxMarabou.outputVars))}
        mnistProp.printDictToFile(inputDictInner, "DICT_verifyMarabou_InputDict_out")
        mnistProp.printDictToFile(outputDictInner, "DICT_verifyMarabou_OutputDict_out")
        equality = (set(outputDictInner.keys()) == set(outputDict.keys())) and all([outputDict[k] == outputDictInner[k] for k in outputDict.keys()])
        return (outputDictInner == outputDict) and equality,

#Return bool, bool: Left is wether yCorrect is the maximal one, Right is wether yBad > yCorrect.
def isCEXSporious(model, x, inDist, outSlack, yCorrect, yBad, cex, sporiousStrict=False):
    inBounds, violations =  inBoundsInftyBall(x, inDist, cex)
    if not inBounds:
        raise Exception("CEX out of bounds, violations={}".format(violations.nonzero()))
    prediction = model.predict(np.array([cex]))
    if not sporiousStrict:
        return prediction.argmax() == yCorrect
    return prediction[0,yBad] + outSlack < prediction[0,yCorrect]

def genActivationMask(intermidModel, example, prediction, policy=mnistProp.Policy.SingleClassRank):
    if policy == mnistProp.Policy.AllClassRank or policy == mnistProp.Policy.AllClassRank.name:
        return genActivationMaskAllClassRank(intermidModel)
    elif policy == mnistProp.Policy.SingleClassRank or policy == mnistProp.Policy.SingleClassRank.name:
        return genActivationMaskSingleClassRank(intermidModel, prediction)
    elif policy == mnistProp.Policy.MajorityClassVote or policy == mnistProp.Policy.MajorityClassVote.name:
        return genActivationMaskMajorityClassVote(intermidModel)
    elif policy == mnistProp.Policy.Centered or policy == mnistProp.Policy.Centered.name:
        return genActivationMaskCentered(intermidModel)
    elif policy == mnistProp.Policy.Random or policy == mnistProp.Policy.Random.name:
        return genActivationMaskRandom(intermidModel)
    elif policy == mnistProp.Policy.FindMinProvable or policy == mnistProp.Policy.FindMinProvable.name:
        return genActivationMaskFindMinProvable(intermidModel)
    raise Exception("genActivationMask - policy not implemented:{}".format(policy))

def sortActMapReverse(actMap):
    sorted = list(np.array(list(product(*[range(d) for d in actMap.shape])))[actMap.flatten().argsort()])
    sorted.reverse()
    return sorted

def sortReverseNeuronsByActivation(intermidModel, samples):
    actMap = meanActivation(intermidModel.predict(samples))
    sortedIndReverse = sortActMapReverse(actMap)
    assert len(sortedIndReverse) == actMap.size
    return sortedIndReverse

def genMaskByOrderedInd(sortedIndDecsending, maskShape, stepSize=mnistProp.stepSize):
    mask = np.zeros(maskShape)
    masks = list()
    stepSize = max(stepSize,1)
    startWith = max(mnistProp.startWith,0)
    first = True
    while len(sortedIndDecsending) > 0:        
        toAdd = min(stepSize + (startWith if first else 0), len(sortedIndDecsending))
        for coor in sortedIndDecsending[:toAdd]:
            mask[tuple(coor)] = 1
        sortedIndDecsending = sortedIndDecsending[toAdd:]
        if np.array_equal(mask, np.ones_like(mask)):
            break
        masks.append(mask.copy())
        first = False
    return masks

def genMaskByActivation(intermidModel, features, stepSize=mnistProp.stepSize):
    sortedIndReverse = sortReverseNeuronsByActivation(intermidModel, features)
    return genMaskByOrderedInd(sortedIndReverse, intermidModel.output_shape[1:-1], stepSize=stepSize)

#Policy - Unmask stepsize most activated neurons, calculating activation on the entire Mnist test.
def genActivationMaskAllClassRank(intermidModel, stepSize=mnistProp.stepSize):
    return genMaskByActivation(intermidModel, mnistProp.x_test, stepSize=stepSize)

#Policy - Unmask stepsize most activated neurons, calculating activation on the Mnist test examples labeled the same as prediction label.
def genActivationMaskSingleClassRank(intermidModel, prediction, stepSize=mnistProp.stepSize):
    features = [x for x,y in zip(mnistProp.x_test, mnistProp.y_test) if y == prediction]
    return genMaskByActivation(intermidModel, np.array(features), stepSize=stepSize)

#Policy - calculate per class
def genActivationMaskMajorityClassVote(intermidModel, stepSize=mnistProp.stepSize):
    features = [[x for x,y in zip(mnistProp.x_test, mnistProp.y_test) if y == label] for label in range(mnistProp.num_classes)]
    actMaps = [meanActivation(intermidModel.predict(np.array(feat))) for feat in features]
    discriminate = lambda actM : np.square(actM) #FIXME explore discriminating schemes.
    actMaps = [discriminate(actMap) for actMap in actMaps]
    sortedIndReverseDiscriminated = sortActMapReverse(sum(actMaps))
    return genMaskByOrderedInd(sortedIndReverseDiscriminated, intermidModel.output_shape[1:-1], stepSize=stepSize)

#Policy - Most important neurons are the center of the image.
def genActivationMaskCentered(intermidModel, stepSize=mnistProp.stepSize):
    maskShape = intermidModel.output_shape[1:-1]
    indicesList = list(product(*[range(d) for d in maskShape]))
    center = np.array([float(d) / 2 for d in maskShape])
    indicesSortedDecsending = sorted(indicesList, key=lambda x: np.linalg.norm(np.array(x)-center))
    return genMaskByOrderedInd(indicesSortedDecsending, maskShape, stepSize=stepSize)

#Policy - Add neurons randomly.
def genActivationMaskRandom(intermidModel, stepSize=mnistProp.stepSize):
    maskShape = intermidModel.output_shape[1:-1]
    indices = np.random.permutation(np.array(list(product(*[range(d) for d in maskShape]))))
    return genMaskByOrderedInd(indices, maskShape, stepSize=stepSize)

def genActivationMaskFindMinProvable(intermidModel):
    maskShape = intermidModel.output_shape[1:-1]
    # [(0,5)] -> UNSAT
    # [(0,5),(0,4)] -> UNSAT
    # [(0,5),(0,4),(1,5)] -> UNSAT
    # [(0,5),(0,0)] -> SAT
    # [(0,5),(0,4),(0,6)] -> SAT
    indices = list(product(*[range(d) for d in maskShape]))
    zeroList = [(0,5),(0,4),(1,5),(1,4),(10,9)]
    FailedWithZeroList = [] #Meaning that was SAT or TimedOut during run with [(0,5),(0,4)]
    indices = list(set(indices) - set(zeroList))
    masks = list()
    mask = np.ones(maskShape)
    chooseFrom = set(indices) - set(FailedWithZeroList)
    for z in zeroList:
        mask[z] = 0
    for i in range(10):
        if not chooseFrom:
            break
        randomElement = random.choice(list(chooseFrom))
        chooseFrom.remove(randomElement)
        mask[randomElement] = 0
        masks.append(mask.copy())
        mask[randomElement] = 1
    return masks
    #return [mask]

def genSquareMask(shape, lBound, uBound):
    onesInd = list(product(*[range(l,min(u+1,dim)) for dim, l, u in zip(shape, lBound, uBound)]))
    mask = np.zeros(shape)
    for ind in onesInd:
        mask[ind] = 1
    return mask

def intermidModel(model, layerName):
    if layerName not in [l.name for l in model.layers]:
        layerName = list([l.name for l in reversed(model.layers) if l.name.startswith(layerName)])[0]
    return tf.keras.Model(inputs=model.input, outputs=model.get_layer(name=layerName).output)

def meanActivation(features, seperateCh=False, model=None):
    if len(features.shape) == 2:
        features = features.reshape(features.shape + (1,1))
    elif len(features.shape) == 3:
        features = features.reshape(features.shape + (1,))
    act = np.mean(features, axis=0) #Assume I recieve 4 dim feature array, then act is 3 dim.
    #if act.shape[2] > 1:
    #    act = np.max(act, axis=2).reshape(act.shape[:-1] + (1,))
    if seperateCh:
        act = [act[:,:,i] for i in range(act.shape[2])]
    else:
        act = np.max(act, axis=2)
    return act
    #return act.reshape(act.shape[:-1]) if act.shape[-1]==1 else act
    #return np.mean(intermidModel(model, layerName).predict(features), axis=0)

def printImg(image, title):
    if not isinstance(image,list):
        imlist = [image]
    else:
        imlist = image
    for i, im in enumerate(imlist):
        if len(im.shape) == 1:
            im = im.reshape(im.shape + (1,))
        if len(imlist) > 1:
            suff = "_" + str(i)
        else:
            suff = ""
        plt.title(title + suff)
        plt.imshow(im, cmap='Greys')
        plt.savefig(title + suff + ".png")

def outputLayerName(model):
    return model.layers[-1].name

def printAvgDomain(model, from_label=False): #from_label or from_prediction
    if from_label:
        x_test_by_class = {label : np.asarray([x for x,y in zip(mnistProp.x_test, mnistProp.y_test) if y == label]) for label in range(mnistProp.num_classes)}
    else:
        predictions =  model.predict(mnistProp.x_test)
        x_test_by_class = {label : np.asarray([x for x,y in zip(mnistProp.x_test, predictions) if y.argmax() == label]) for label in range(mnistProp.num_classes)}
    meanAct = [meanActivation(model, outputLayerName(model), x_test_by_class[y]) for y in range(mnistProp.num_classes)]
    for y in range(mnistProp.num_classes):
        printImg(meanAct[y], "{}_meanAct_label_{}.png".format(model.name, y))

#https://keras.io/getting_started/faq/#how-can-i-obtain-the-output-of-an-intermediate-layer-feature-extraction
def compareModels(origM, absM):
    print("compareModels - Starting evaluation of differances between models.")
    layersOrig = [l.name for l in origM.layers]
    layersAbs  = [l.name for l in absM.layers if ("rplc" not in l.name or "rplcOut" in l.name)]
    log = []
    #print("orig")
    #[print(l,l.input) for l in origM.layers]
    #print(origM.input)
    #print("abs")
    #[print(l,l.input) for l in absM.layers]
    #print(absM.input)
    equal_full   = np.all(np.equal  (origM.predict(mnistProp.x_test), absM.predict(mnistProp.x_test)))
    isclose_full = np.all(np.isclose(origM.predict(mnistProp.x_test), absM.predict(mnistProp.x_test)))
    print("equal_full={:>2}, equal_isclose={:>2}".format(equal_full, isclose_full))
    for lo, la in zip(layersOrig, layersAbs):
        mid_origM = intermidModel(origM, lo)
        mid_absM  = intermidModel(absM , la)
        print("compare {} to {}".format(lo, la))
        w_equal = (len(origM.get_layer(name=lo).get_weights()) == len(absM.get_layer(name=la).get_weights())) and all([np.all(np.equal(wo,wa)) for wo,wa in zip(origM.get_layer(name=lo).get_weights(), absM.get_layer(name=la).get_weights())])
        equal   = np.all(np.equal  (mid_origM.predict(mnistProp.x_test), mid_absM.predict(mnistProp.x_test)))
        isclose = np.all(np.isclose(mid_origM.predict(mnistProp.x_test), mid_absM.predict(mnistProp.x_test)))
        log.append(equal)
        print("layers: orig={:>2} ; abs={:>20}, equal={:>2}, isclose={:>2}, w_equal={:>2}".format(lo, la, equal, isclose, w_equal))
    return log
