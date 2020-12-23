
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
from tensorflow.keras import datasets, layers, models
import numpy as np
import logging
import matplotlib.pyplot as plt
from enum import Enum

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
    zero_test = np.zeros((1,) + x_test.shape[1:])
    half_test = 0.5 * np.ones((1,) + x_test.shape[1:])
    ones_test = np.ones((1,) + x_test.shape[1:])
    single_test = np.array([x_test[0]])
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
    origMConv = None
    origMDense = None
    numTestSamples = 10
    Policy = Enum("Policy","Centered AllClassRank SingleClassRank MajorityClassVote")

    def output_model_path(m, suffix=""):
        if suffix:
            suffix = "_" + suffix
        return "./{}{}.onnx".format(m.name, suffix)

    def printDictToFile(dic, fName):
        with open(fName,"w") as f:
            for i,x in dic.items():
                f.write("{},{}\n".format(i,x))

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

def cloneAndMaskConvModel(origM, rplcLayerName, mask, cfg_freshModelAbs=True):
    if cfg_freshModelAbs:
        rplcIn = origM.get_layer(name=rplcLayerName).input_shape        
        rplcOut = origM.get_layer(name=rplcLayerName).output_shape
        origW = origM.get_layer(name=rplcLayerName).get_weights()[0]
        origB = origM.get_layer(name=rplcLayerName).get_weights()[1]    
        strides = origM.get_layer(name=rplcLayerName).strides
        clnW, clnB = maskAndDensifyNDimConv(origW, origB, mask, rplcIn, rplcOut, strides)
        clnLayers = [tf.keras.Input(shape=mnistProp.input_shape, name="input_clnM")]
        toSetWeights = {}
        lSuffix = "_clnM_{}".format(mnistProp.numClones)
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

        if np.all(np.isclose(clnM.predict(mnistProp.x_test), origM.predict(mnistProp.x_test))):
        #if np.all(np.equal(clnM.predict(mnistProp.x_test), origM.predict(mnistProp.x_test))):
            print("Prediction aligned")    
        else:
            print("Prediction not aligned")
    else:
        clnM = models.load_model(mnistProp.savedModelAbs)

    return clnM


def genCnnForAbsTest(cfg_limitCh=True, cfg_freshModelOrig=mnistProp.cfg_fresh, savedModelOrig="cnn_abs_orig.h5"):

    print("Starting model building")
    #https://keras.io/examples/vision/mnist_convnet/

    if cfg_freshModelOrig:
        num_ch = 1 if cfg_limitCh else 32
        origM = tf.keras.Sequential(
            [
                tf.keras.Input(shape=mnistProp.input_shape, name="input_origM"),
                layers.Conv2D(num_ch, kernel_size=(3,3), activation="relu", name="c1"),
                layers.MaxPooling2D(pool_size=(3,3), name="mp1"),
                layers.Conv2D(num_ch, kernel_size=(3,3), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(3,3), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dense(40, activation="relu", name="fc1"),
                #layers.Dropout(0.5, name="do1"),
                #layers.Dense(mnistProp.num_classes, activation="softmax", name="sm1")
                layers.Dense(mnistProp.num_classes, activation=None, name="sm1")
            ],
            name="origModel"
        )

        batch_size = 128
        epochs = 15

        origM.build(input_shape=mnistProp.featureShape)
        origM.summary()        
        origM.compile(optimizer=mnistProp.optimizer, loss=mnistProp.loss, metrics=mnistProp.metrics)
        origM.fit(mnistProp.x_train, mnistProp.y_train, epochs=epochs, batch_size=batch_size, validation_split=0.1)
        
        score = origM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print("(Original) Test loss:", score[0])
        print("(Original) Test accuracy:", score[1])
        origM.save(savedModelOrig)
    else:
        origM = load_model(savedModelOrig)
        origM.summary()

        #FIXME
        #w0 = np.ones(origM.get_layer(name="c2").get_weights()[0].shape)
        ####w1 = np.ones(origM.get_layer(name="c2").get_weights()[1].shape)
        #w1 = origM.get_layer(name="c2").get_weights()[1]
        #origM.get_layer(name="c2").set_weights([w0,w1])
        #FIXME
        
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
    return np.all(np.less_equal(l,p)) and np.all(np.greater_equal(u,p))
    
def setAdversarial(net, x, inDist, yCorrect, yBad):
    inAsNP = np.array(net.inputVars)
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
    net.addInequality([yCorrectVar, yBadVar], [1,-1], 0) # correct - bad <= 0
    return net
    
def cexToImage(net, valDict, xAdv, inDist, inputVarsMapping=None, outputVarsMapping=None, useMapping=True):
    if useMapping:
        lBounds = getBoundsInftyBall(xAdv, inDist)[0]
        cex           = np.array([valDict[i.item()] if i.item() != -1 else lBnd for i,lBnd in zip(np.nditer(np.array(inputVarsMapping)), np.nditer(lBounds))]).reshape(xAdv.shape)
        cexPrediction = np.array([valDict[o.item()] if o.item() != -1 else 0 for o in np.nditer(np.array(outputVarsMapping))]).reshape(outputVarsMapping.shape)      
    else:
        cex = np.array([valDict[i.item()] for i in np.nditer(np.array(net.inputVars))]).reshape(xAdv.shape)        
        cexPrediction = np.array([valDict[o.item()] for o in np.nditer(np.array(net.outputVars))])
    return cex, cexPrediction

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
                for w,v in eq.addendList[:-1]:
                    if w != 0:
                        reach.add(v)
        for maxArgs, maxOut in net.maxList:
            if maxOut in reachPrev:            
                [reach.add(arg) for arg in maxArgs]
        for vin,vout in net.reluList:
            if vout in reachPrev:
                reach.add(vin)
        for vin,vout in net.absList:
            if vout in reachPrev:
                reach.add(vin)
        for vin,vout in net.signList:
            if vout in reachPrev:
                reach.add(vin)
        if len(net.disjunctionList) > 0:
            raise Exception("Not implemented")
    unreach = set([v for v in range(net.numVars) if v not in reach])

    print("COI : reached={}, unreached={}, out_of={}".format(len(reach), len(unreach), net.numVars))        

    reachList = list(reach)
    reachList.sort()
    reachDict = {v:i for i,v in enumerate(reachList)}
    assert reach == set(reachDict.keys())
    tr = lambda v: reachDict[v] if v in reachDict else -1
    
    newEquList = list()
    for vin,vout in net.reluList:
        assert (vin not in reach) == (vout not in reach)
    for vin,vout in net.absList:
        assert (vin not in reach) == (vout not in reach)
    for vin,vout in net.signList:
        assert (vin not in reach) == (vout not in reach)
    
    for eq in net.equList:
        if (eq.EquationType == MarabouCore.Equation.EQ) and (eq.addendList[-1][0] == -1): #FIXME should suport other types?            
            newEq = MarabouUtils.Equation()
            newEq.scalar = eq.scalar
            newEq.EquationType = MarabouCore.Equation.EQ
            newEq.addendList = [(el[0],tr(el[1])) for el in eq.addendList if el[1] in reach]
            if (eq.addendList[-1][1] not in reach) or len(newEq.addendList) == 1:
                continue
            if newEq.addendList:
                newEquList.append(newEq)
        else:
            newEq = MarabouUtils.Equation()
            newEq.scalar = eq.scalar
            newEq.EquationType = MarabouCore.Equation.EQ
            newEq.addendList = [(el[0],tr(el[1])) for el in eq.addendList]
            newEquList.append(eq)
    net.equList  = newEquList
    net.maxList  = [({tr(arg) for arg in maxArgs if arg in reach}, tr(maxOut)) for maxArgs, maxOut in net.maxList if (maxOut in reach and any([arg in reach for arg in maxArgs]))]
    net.reluList = [(tr(vin),tr(vout)) for vin,vout in net.reluList if vout in reach]
    net.absList  = [(tr(vin),tr(vout)) for vin,vout in net.absList  if vout in reach]
    net.signList = [(tr(vin),tr(vout)) for vin,vout in net.signList if vout in reach]
    net.lowerBounds = {tr(v):l for v,l in net.lowerBounds.items() if v in reach}
    net.upperBounds = {tr(v):u for v,u in net.upperBounds.items() if v in reach}
    inputVarsMapping = np.array([tr(v) for v in net.inputVars[0].flatten().tolist()]).reshape(net.inputVars[0].shape)
    outputVarsMapping = np.array([tr(v) for v in net.outputVars.flatten().tolist()]).reshape(net.outputVars.shape)
    net.inputVars  = [np.array([tr(v) for v in net.inputVars[0].flatten().tolist()  if v in reach])]
    net.outputVars = np.array([tr(v) for v in net.outputVars.flatten().tolist() if v in reach])
    net.numVars = len(reachList)
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
    #exit()
    return inputVarsMapping, outputVarsMapping
    
def runMarabouOnKeras(model, logger, xAdv, inDist, yMax, ySecond, runName="runMarabouOnKeras", coi=True):
    #runName = runName + "_" + str(mnistProps.numInputQueries)
    #mnistProps.numInputQueries = mnistProps.numInputQueries + 1
    modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=(1 if logger.level==logging.DEBUG else 0))
    modelOnnxName = mnistProp.output_model_path(model)
    keras2onnx.save_model(modelOnnx, modelOnnxName)
    modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
    setAdversarial(modelOnnxMarabou, xAdv, inDist, yMax, ySecond)
    if coi:
        inputVarsMapping, outputVarsMapping = setCOIBoundes(modelOnnxMarabou, modelOnnxMarabou.outputVars.flatten().tolist())
        print("*** inputVarsMapping ={} ***".format(inputVarsMapping))
        print("*** outputVarsMapping={} ***".format(outputVarsMapping))
    else:
        inputVarsMapping, outputVarsMapping = None, None
    #modelOnnxMarabou.saveQuery(runName+"_beforeSolve")
    modelOnnxMarabou.saveQuery(runName)    
    vals, stats = modelOnnxMarabou.solve(verbose=False) #FIXME
    #return False, np.array([]), np.array([]), dict(), dict()  #FIXME
    sat = len(vals) > 0        
    if not sat:
        return False, np.array([]), np.array([]), dict(), dict()  
    inputDict = {i.item():vals[i.item()] for i in np.nditer(np.array(modelOnnxMarabou.inputVars))}
    outputDict = {o.item():vals[o.item()] for o in np.nditer(np.array(modelOnnxMarabou.outputVars))}
    #modelOnnxMarabou.saveQuery(runName+"_AfterSolve")    
    cex, cexPrediction = cexToImage(modelOnnxMarabou, vals, xAdv, inDist, inputVarsMapping, outputVarsMapping, useMapping=coi)
    fName = "Cex_{}.png".format(mnistProp.numCex)
    mnistProp.numCex += 1
    mbouPrediction = cexPrediction.argmax()
    kerasPrediction = model.predict(np.array([cex])).argmax()
    if mbouPrediction != kerasPrediction:
        origMConvPrediction = mnistProp.origMConv.predict(np.array([cex])).argmax()
        origMDensePrediction = mnistProp.origMDense.predict(np.array([cex])).argmax()
        print("Marabou and keras doesn't predict the same class. mbouPrediction ={}, kerasPrediction={}, origMConvPrediction={}, origMDensePrediction={}".format(mbouPrediction, kerasPrediction, origMConvPrediction, origMDensePrediction))
    plt.title('CEX, MarabouY={}, modelY={}'.format(mbouPrediction, kerasPrediction))
    plt.imshow(cex.reshape(xAdv.shape[:-1]), cmap='Greys')
    plt.savefig(fName)
    mnistProp.printDictToFile(inputDict, "DICT_runMarabouOnKeras_InputDict")
    mnistProp.printDictToFile(outputDict, "DICT_runMarabouOnKeras_OutputDict")        
    return True, cex, cexPrediction, inputDict, outputDict

def verifyMarabou(model, xAdv, xPrediction, inputDict, outputDict, runName="verifyMarabou", fromImage=False):
    mnistProp.printDictToFile(inputDict, "DICT_verifyMarabou_InputDict_in")
    mnistProp.printDictToFile(outputDict, "DICT_verifyMarabou_OutputDict_in")    
    modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=0)
    modelOnnxName = mnistProp.output_model_path(model)
    keras2onnx.save_model(modelOnnx, modelOnnxName)
    modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
    if fromImage:
        inAsNP = np.array(modelOnnxMarabou.inputVars)
        xAdv = xAdv.reshape(inAsNP.shape)
        for i,x in zip(np.nditer(inAsNP),np.nditer(xAdv)):    
            modelOnnxMarabou.setLowerBound(i.item(),x.item())
            modelOnnxMarabou.setUpperBound(i.item(),x.item())
    else:
        for i,x in inputDict.items():    
            modelOnnxMarabou.setLowerBound(i,x)
            modelOnnxMarabou.setUpperBound(i,x)
    vals, stats = modelOnnxMarabou.solve(verbose=False)
    modelOnnxMarabou.saveQuery(runName)
    if fromImage:
        predictionMbou = np.array([vals[o.item()] for o in np.nditer(np.array(modelOnnxMarabou.outputVars))])
        print("predictionMbou={}".format(predictionMbou))
        print("xPrediction={}".format(xPrediction))    
        return np.all(xPrediction == predictionMbou), xPrediction.argmax() == predictionMbou.argmax(), predictionMbou
    else:
        inputDictInner = {i.item():vals[i.item()] for i in np.nditer(np.array(modelOnnxMarabou.inputVars))}
        outputDictInner = {o.item():vals[o.item()] for o in np.nditer(np.array(modelOnnxMarabou.outputVars))}
        mnistProp.printDictToFile(inputDictInner, "DICT_verifyMarabou_InputDict_out")
        mnistProp.printDictToFile(outputDictInner, "DICT_verifyMarabou_OutputDict_out")
        equality = (set(outputDictInner.keys()) == set(outputDict.keys())) and all([outputDict[k] == outputDictInner[k] for k in outputDict.keys()])        
        return (outputDictInner == outputDict) and equality 

def isCEXSporious(model, x, inDist, yCorrect, yBad, cex, logger):
    if not inBoundsInftyBall(x, inDist, cex):        
        raise Exception("CEX out of bounds")
    if model.predict(np.array([cex])).argmax() == yCorrect:        
        return True
    return False


def genActivationMask(intermidModel, example, prediction, policy=mnistProp.Policy.AllClassRank):
    if policy == mnistProp.Policy.AllClassRank:
        return genActivationMaskAllClassRank(intermidModel)
    elif policy == mnistProp.Policy.SingleClassRank:
        return genActivationMaskSingleClassRank(intermidModel, example, prediction)
    elif policy == mnistProp.Policy.MajorityClassVote:
        return genActivationMaskMajorityClassVote(intermidModel)
    elif policy == mnistProp.Policy.Centered:
        return genActivationMaskCentered(intermidModel)
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

def genMaskByOrderedInd(sortedIndReverse, maskShape, stepSize=10):
    numberOfNeurons = 0
    mask = np.zeros(maskShape)
    numNeurons = len(sortedIndReverse)
    masks = list()
    while numberOfNeurons < numNeurons and len(sortedIndReverse) > 0:
        toAdd = min(stepSize, len(sortedIndReverse))
        for coor in sortedIndReverse[:toAdd]:
            mask[tuple(coor)] = 1
        numberOfNeurons += toAdd
        sortedIndReverse = sortedIndReverse[toAdd:]
        masks.append(mask.copy())
    return masks
    
def genMaskByActivation(intermidModel, features, stepSize=10):
    sortedIndReverse = sortReverseNeuronsByActivation(intermidModel, features)
    return genMaskByOrderedInd(sortedIndReverse, intermidModel.output_shape[1:], stepSize=stepSize)

#Policy - Unmask stepsize most activated neurons, calculating activation on the entire Mnist test.
def genActivationMaskAllClassRank(intermidModel, stepSize=10):
    return genMaskByActivation(intermidModel, mnistProp.x_test, stepSize=10)

#Policy - Unmask stepsize most activated neurons, calculating activation on the Mnist test examples labeled the same as prediction label.
def genActivationMaskSingleClassRank(intermidModel, example, prediction):
    features = [x for x,y in zip(mnistProp.x_test, mnistProp.y_test) if y == prediction]
    return genMaskByActivation(intermidModel, features, stepSize=10)

#Policy - calculate per class 
def genActivationMaskMajorityClassVote(intermidModel):
    features = [[x for x,y in zip(mnistProp.x_test, mnistProp.y_test) if y == label] for label in range(mnistProp.num_classes)]
    actMaps = [meanActivation(intermidModel.predict(feat)) for feat in features]
    dicriminate = lambda actM : np.square(actM) #FIXME explore discriminating schemes.
    actMaps = [discriminate(actMap) for actMap in actMaps]
    sortedIndReverseDiscriminated = sortActMapReverse(sum(actMaps))
    return genMaskByOrderedInd(sortedIndReverseDiscriminated, stepSize=10)

#Policy - most important neurons are the center of the image.
def genActivationMaskCentered(intermidModel):
    maskShape = intermidModel.output_shape[1:]
    for thresh in reversed(range(int(min(maskShape)/2))):        
        yield genSquareMask(maskShape, [thresh for dim in maskShape if dim > (2 * thresh)], [dim - thresh for dim in maskShape if dim > (2 * thresh)])
    
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
