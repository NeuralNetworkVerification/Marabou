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
from enum import Enum, auto
import json
import random
from tensorflow.keras.models import load_model

#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################

class PolicyBase:

    def __init__(self, ds):
        self.policy = None
        self.stepSize = 10
        self.startWith = 50
        self.ds = DataSet(ds)
        self.coi = True
    
    def meanActivation(features, seperateCh=False, model=None):
        if len(features.shape) == 2:
            features = features.reshape(features.shape + (1,1))
        elif len(features.shape) == 3:
            features = features.reshape(features.shape + (1,))
        act = np.mean(features, axis=0) #Assume I recieve 4 dim feature array, then act is 3 dim.
        if seperateCh:
            act = [act[:,:,i] for i in range(act.shape[2])]
        else:
            act = np.max(act, axis=2)
        return act

    def genMaskByActivation(self, intermidModel, features, includeFull=True):
        sortedIndReverse = PolicyBase.sortReverseNeuronsByActivation(intermidModel, features)
        return self.genMaskByOrderedInd(sortedIndReverse, intermidModel.output_shape[1:-1], includeFull=includeFull)

    def sortActMapReverse(actMap):
        sorted = list(np.array(list(product(*[range(d) for d in actMap.shape])))[actMap.flatten().argsort()])
        sorted.reverse()
        return sorted
    
    def sortReverseNeuronsByActivation(intermidModel, samples):
        actMap = PolicyBase.meanActivation(intermidModel.predict(samples))
        sortedIndReverse = PolicyBase.sortActMapReverse(actMap)
        assert len(sortedIndReverse) == actMap.size
        return sortedIndReverse
    
    def genMaskByOrderedInd(self, sortedIndDecsending, maskShape, includeFull=True):
        mask = np.zeros(maskShape)
        masks = list()
        stepSize = max(self.stepSize,1)
        startWith = max(self.startWith,0)
        first = True
        while len(sortedIndDecsending) > 0:
            toAdd = min(stepSize + (startWith if first else 0), len(sortedIndDecsending))
            for coor in sortedIndDecsending[:toAdd]:
                mask[tuple(coor)] = 1
            sortedIndDecsending = sortedIndDecsending[toAdd:]
            if np.array_equal(mask, np.ones_like(mask)) and not includeFull:
                break
            masks.append(mask.copy())
            first = False
        return masks

    def genSquareMask(shape, lBound, uBound):
        onesInd = list(product(*[range(l,min(u+1,dim)) for dim, l, u in zip(shape, lBound, uBound)]))
        mask = np.zeros(shape)
        for ind in onesInd:
            mask[ind] = 1
        return mask
    
    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        raise NotImplementedError

#Policy - Most important neurons are the center of the image.
class PolicyCentered(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.Centered

    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        maskShape = intermidModel.output_shape[1:-1]
        indicesList = list(product(*[range(d) for d in maskShape]))
        center = np.array([float(d) / 2 for d in maskShape])
        indicesSortedDecsending = sorted(indicesList, key=lambda x: np.linalg.norm(np.array(x)-center))
        return self.genMaskByOrderedInd(indicesSortedDecsending, maskShape, includeFull=includeFull)

#Policy - Unmask stepsize most activated neurons, calculating activation on the entire Mnist test.    
class PolicyAllClassRank(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.AllClassRank

    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        return self.genMaskByActivation(intermidModel, self.ds.x_test, includeFull=includeFull)

#Policy - Unmask stepsize most activated neurons, calculating activation on the Mnist test examples labeled the same as prediction label.    
class PolicySingleClassRank(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.SingleClassRank
            
    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        features = [x for x,y in zip(self.ds.x_test, self.ds.y_test) if y == prediction]
        return self.genMaskByActivation(intermidModel, np.array(features), includeFull=includeFull)
    
#Policy - calculate per class
class PolicyMajorityClassVote(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)  
        self.policy = Policy.MajorityClassVote  
            
    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        features = [[x for x,y in zip(self.ds.x_test, self.ds.y_test) if y == label] for label in range(self.ds.num_classes)]
        actMaps = [PolicyBase.meanActivation(intermidModel.predict(np.array(feat))) for feat in features]
        discriminate = lambda actM : np.square(actM) #TODO explore discriminating schemes.
        actMaps = [discriminate(actMap) for actMap in actMaps]
        sortedIndReverseDiscriminated = PolicyBase.sortActMapReverse(sum(actMaps))
        return self.genMaskByOrderedInd(sortedIndReverseDiscriminated, intermidModel.output_shape[1:-1], includeFull=includeFull)

#Policy - Add neurons randomly. 
class PolicyRandom(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)  
        self.policy = Policy.Random  

    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        maskShape = intermidModel.output_shape[1:-1]
        indices = np.random.permutation(np.array(list(product(*[range(d) for d in maskShape]))))
        return self.genMaskByOrderedInd(indices, maskShape, includeFull=includeFull)
    
class PolicyVanilla(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.Vanilla
        self.coi = False
        
    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        maskShape = intermidModel.output_shape[1:-1]
        if includeFull:
            return [np.ones(maskShape)]
        return []
    
class PolicyFindMinProvable(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds) 
        self.policy = Policy.FindMinProvable
        
    def genActivationMask(self, intermidModel, prediction=None, includeFull=True):
        maskShape = intermidModel.output_shape[1:-1]
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

class Policy(Enum):
    Centered          = 0
    AllClassRank      = 1
    SingleClassRank   = 2
    MajorityClassVote = 3
    Random            = 4
    Vanilla           = 5
    FindMinProvable   = 6

    @staticmethod
    def absPolicies():
        return [Policy.Centered.name, Policy.AllClassRank.name, Policy.SingleClassRank.name, Policy.MajorityClassVote.name, Policy.Random.name]

    @staticmethod
    def solvingPolicies():
        return Policy.absPolicies() + [Policy.Vanilla.name]

#    @staticmethod
#    def asString():
#        return [policy.name for policy in Policy.solvingPolicies()]

    @staticmethod
    def fromString(s, ds):
        s = s.lower()
        if   s == Policy.Centered.name.lower():
            return PolicyCentered(ds)
        elif s == Policy.AllClassRank.name.lower():
            return PolicyAllClassRank(ds)
        elif s == Policy.SingleClassRank.name.lower():
            return PolicySingleClassRank(ds)
        elif s == Policy.MajorityClassVote.name.lower():
            return PolicyMajorityClassVote(ds)
        elif s == Policy.Random.name.lower():
            return PolicyRandom(ds)
        elif s == Policy.Vanilla.name.lower():
            return PolicyVanilla(ds)
        elif s == Policy.FindMinProvable.name.lower():
            return PolicyFindMinProvable(ds)
        else:
            raise NotImplementedError

class AdversarialProperty:

    def __init__(self, xAdv, yMax, ySecond, inDist, outSlack):
        self.xAdv = xAdv
        self.yMax = yMax
        self.ySecond = ySecond
        self.inDist = inDist
        self.outSlack = outSlack

class Result(Enum):
    
    TIMEOUT = 0
    SAT = 1
    UNSAT = 2

    @classmethod
    def str2Result(cls, s):
        s = s.lower()
        if s == "timeout":
            return cls.TIMEOUT
        elif s == "sat":
            return cls.SAT
        elif s == "unsat":
            return cls.UNSAT
        else:
            raise NotImplementedError            
        
class ResultObj:
    
    def __init__(self, result):
        self.originalQueryStats = None
        self.finalQueryStats = None
        self.cex = np.array([])
        self.cexPrediction = np.array([])
        self.inputDict = dict()
        self.outputDict = dict()
        self.result = Result.str2Result(result)

    def timedOut(self):
        return self.result is Result.TIMEOUT

    def sat(self):
        return self.result is Result.SAT
    
    def unsat(self):
        return self.result is Result.UNSAT
    
    def setStats(self, orig, final):
        self.originalQueryStats = orig
        self.finalQueryStats = final

    def setCex(self, cex, cexPrediction, inputDict, outputDict):
        self.cex = cex
        self.cexPrediction = cexPrediction
        self.inputDict = inputDict
        self.outputDict = outputDict        

class DataSet:

    def __init__(self, ds='mnist'):
        if ds.lower() == 'mnist':
            self.setMnist()
        else:
            raise NotImplementedError
        
    def setMnist(self):
        self.num_classes = 10
        self.input_shape = (28,28,1)
        (self.x_train, self.y_train), (self.x_test, self.y_test) = tf.keras.datasets.mnist.load_data()
        self.x_train, self.x_test = self.x_train / 255.0, self.x_test / 255.0
        self.x_train = np.expand_dims(self.x_train, -1)
        self.x_test = np.expand_dims(self.x_test, -1)
        self.featureShape=(1,28,28)
        self.loss='sparse_categorical_crossentropy'
        self.optimizer='adam'
        self.metrics=['accuracy']        
    
class CnnAbs:

    logger = None
    
    def __init__(self, ds='mnist', dumpDir='', optionsObj=None):
        if CnnAbs.logger == None:
            CnnAbs.setLogger()
        self.ds = DataSet(ds)
        self.optionsObj = optionsObj
        self.modelUtils = ModelUtils(self.ds, self.optionsObj)
        self.dumpDir = dumpDir        
        if dumpDir and not os.path.exists(dumpDir):
            os.mkdir(dumpDir)

    def genAdvMbouNet(self, model, prop, boundDict, runName, coi):
        modelOnnxMarabou = ModelUtils.tf2MbouOnnx(model)
        InputQueryUtils.setAdversarial(modelOnnxMarabou, prop.xAdv, prop.inDist, prop.outSlack, prop.yMax, prop.ySecond)
        InputQueryUtils.setBounds(modelOnnxMarabou, boundDict)
        originalQueryStats = self.dumpQueryStats(modelOnnxMarabou, "originalQueryStats_" + runName)
        if coi:
            inputVarsMapping, outputVarsMapping, varsMapping = InputQueryUtils.setCOIBoundes(modelOnnxMarabou, modelOnnxMarabou.outputVars.flatten().tolist())
            self.dumpCoi(inputVarsMapping, runName)
        else:
            inputVarsMapping = modelOnnxMarabou.inputVars[0]
            outputVarsMapping = modelOnnxMarabou.outputVars
            varsMapping = {v : v for v in range(modelOnnxMarabou.numVars)}
        self.dumpNpArray(inputVarsMapping, "inputVarsMapping_" + runName)
        self.dumpNpArray(outputVarsMapping, "outputVarsMapping_" + runName)
        self.dumpJson(varsMapping, "varsMapping_" + runName)
        InputQueryUtils.setUnconnectedAsInputs(modelOnnxMarabou)        
        modelOnnxMarabou.saveQuery(self.dumpDir + "IPQ_" + runName)
        finalQueryStats = self.dumpQueryStats(modelOnnxMarabou, "finalQueryStats_" + runName)
        return modelOnnxMarabou, originalQueryStats, finalQueryStats, inputVarsMapping, outputVarsMapping, varsMapping
    
    def runMarabouOnKeras(self, model, prop, boundDict, runName="runMarabouOnKeras", coi=True, onlyDump=False, fromDumpedQuery=False):
        CnnAbs.printLog("\n\n\n ----- Start Solving {} ----- \n\n\n".format(runName))
        if not fromDumpedQuery:
            mbouNet, originalQueryStats, finalQueryStats, inputVarsMapping, outputVarsMapping, varsMapping = self.genAdvMbouNet(model, prop, boundDict, runName, coi)
            ipq = mbouNet.getMarabouQuery()
            if onlyDump:
                return "IPQ_" + runName            
        else:
            ipq = Marabou.load_query(self.dumpDir + "IPQ_" + runName)
        vals, stats = Marabou.solve_query(ipq, verbose=False, options=self.optionsObj)
        CnnAbs.printLog("\n\n\n ----- Finished Solving {} ----- \n\n\n".format(runName))
        sat = len(vals) > 0
        timedOut = stats.hasTimedOut()
        if fromDumpedQuery:
            originalQueryStats = self.loadJson("originalQueryStats_" + runName)
            finalQueryStats = self.loadJson("finalQueryStats_" + runName)
            inputVarsMapping = self.loadNpArray("inputVarsMapping_" + runName)
            outputVarsMapping = self.loadNpArray("outputVarsMapping_" + runName)
            varsMapping = self.loadJson("varsMapping_" + runName)            
        if not sat:
            if timedOut:
                result = ResultObj("timeout")
                CnnAbs.printLog("\n\n\n ----- Timed out in {} ----- \n\n\n".format(runName))
            else:
                result = ResultObj("unsat")
                CnnAbs.printLog("\n\n\n ----- UNSAT in {} ----- \n\n\n".format(runName))
            result.setStats(originalQueryStats, finalQueryStats)                
            return result
        cex, cexPrediction, inputDict, outputDict = ModelUtils.cexToImage(vals, prop, inputVarsMapping, outputVarsMapping, useMapping=coi)
        self.dumpCex(cex, cexPrediction, prop, runName)
        self.dumpJson(inputDict, "DICT_runMarabouOnKeras_InputDict")
        self.dumpJson(outputDict, "DICT_runMarabouOnKeras_OutputDict")
        result = ResultObj("sat")
        result.setCex(cex, cexPrediction, inputDict, outputDict)
        result.setStats(originalQueryStats, finalQueryStats)
        CnnAbs.printLog("\n\n\n ----- SAT in {} ----- \n\n\n".format(runName))
        return result

    def setLogger():
        logging.basicConfig(level = logging.DEBUG, format = "%(asctime)s %(levelname)s %(message)s", filename = "cnnAbsTB.log", filemode = "w")
        CnnAbs.logger = logging.getLogger('cnnAbsTB')
        CnnAbs.logger.setLevel(logging.INFO)
        fh = logging.FileHandler('cnnAbsTB.log')
        fh.setLevel(logging.DEBUG)
        fh.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
        CnnAbs.logger.addHandler(fh)
        ch = logging.StreamHandler()
        ch.setLevel(logging.ERROR)
        ch.setFormatter(logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s'))
        CnnAbs.logger.addHandler(ch)

        logging.getLogger('matplotlib.font_manager').disabled = True

    @staticmethod
    def printLog(s):
        if CnnAbs.logger:
            CnnAbs.logger.info(s)
        print(s)

    def dumpNpArray(self, npArray, name):
        with open(self.dumpDir + name + ".npy", "wb") as f:
            np.save(f, npArray)      

    def dumpQueryStats(self, mbouNet, name):
        queryStats = ModelUtils.marabouNetworkStats(mbouNet)
        self.dumpJson(queryStats, name)
        return queryStats

    def dumpJson(self, data, name):
        with open(self.dumpDir + name + ".json", "w") as f:
            json.dump(data, f, indent = 4)

    def dumpCex(self, cex, cexPrediction, prop, runName):
        mbouPrediction = cexPrediction.argmax()
        plt.title('CEX, yMax={}, ySecond={}, MarabouPredictsCEX={}'.format(prop.yMax, prop.ySecond, mbouPrediction))
        plt.imshow(cex.reshape(prop.xAdv.shape[:-1]), cmap='Greys')
        plt.savefig("Cex_{}".format(runName) + ".png")
        self.dumpNpArray(cex, "Cex_{}".format(runName))

    @staticmethod        
    def dumpCoi(inputVarsMapping, runName):
        plt.title('COI_{}'.format(runName))
        plt.imshow(np.array([0 if i == -1 else 1 for i in np.nditer(inputVarsMapping.flatten())]).reshape(inputVarsMapping.shape[1:-1]), cmap='Greys')
        plt.savefig('COI_{}'.format(runName))

    def loadJson(self, name):
        with open(self.dumpDir + name + ".json", "r") as f:
            return json.load(f)

    def loadNpArray(self, name):
        with open(self.dumpDir + name + ".npy", "rb") as f:            
            return np.load(f, allow_pickle=True)        

#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################

def myLoss(labels, logits):
    return tf.keras.losses.sparse_categorical_crossentropy(labels, logits, from_logits=True)

class ModelUtils:
    
    numClones = 0

    def __init__(self, ds, optionsObj):
        self.ds = ds
        self.optionsObj = optionsObj

    @classmethod
    def incNumClones(cls):
        cls.numClones += 1

    @staticmethod
    def outputModelPath(m, suffix=""):
        if suffix:
            suffix = "_" + suffix
        return "./{}{}.onnx".format(m.name, suffix)
        

    @staticmethod
    def tf2MbouOnnx(model):
        modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=0)
        modelOnnxName = ModelUtils.outputModelPath(model)
        keras2onnx.save_model(modelOnnx, modelOnnxName)
        return monnx.MarabouNetworkONNX(modelOnnxName)
                       
    def intermidModel(model, layerName):
        if layerName not in [l.name for l in model.layers]:
            layerName = list([l.name for l in reversed(model.layers) if l.name.startswith(layerName)])[0]
        return tf.keras.Model(inputs=model.input, outputs=model.get_layer(name=layerName).output)
        
    #https://keras.io/getting_started/faq/#how-can-i-obtain-the-output-of-an-intermediate-layer-feature-extraction
    def compareModels(origM, absM):
        CnnAbs.printLog("compareModels - Starting evaluation of differances between models.")
        layersOrig = [l.name for l in origM.layers]
        layersAbs  = [l.name for l in absM.layers if ("rplc" not in l.name or "rplcOut" in l.name)]
        log = []
        equal_full   = np.all(np.equal  (origM.predict(self.ds.x_test), absM.predict(self.ds.x_test)))
        isclose_full = np.all(np.isclose(origM.predict(self.ds.x_test), absM.predict(self.ds.x_test)))
        CnnAbs.printLog("equal_full={:>2}, equal_isclose={:>2}".format(equal_full, isclose_full))
        for lo, la in zip(layersOrig, layersAbs):
            mid_origM = intermidModel(origM, lo)
            mid_absM  = intermidModel(absM , la)
            CnnAbs.printLog("compare {} to {}".format(lo, la))
            w_equal = (len(origM.get_layer(name=lo).get_weights()) == len(absM.get_layer(name=la).get_weights())) and all([np.all(np.equal(wo,wa)) for wo,wa in zip(origM.get_layer(name=lo).get_weights(), absM.get_layer(name=la).get_weights())])
            equal   = np.all(np.equal  (mid_origM.predict(self.ds.x_test), mid_absM.predict(self.ds.x_test)))
            isclose = np.all(np.isclose(mid_origM.predict(self.ds.x_test), mid_absM.predict(self.ds.x_test)))
            log.append(equal)
            CnnAbs.printLog("layers: orig={:>2} ; abs={:>20}, equal={:>2}, isclose={:>2}, w_equal={:>2}".format(lo, la, equal, isclose, w_equal))
        return log
    
    def dumpBounds(self, model, xAdv, inDist, outSlack, yMax, ySecond):
        modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=0)
        modelOnnxName = ModelUtils.outputModelPath(model)
        keras2onnx.save_model(modelOnnx, modelOnnxName)
        modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
        InputQueryUtils.setAdversarial(modelOnnxMarabou, xAdv, inDist, outSlack, yMax, ySecond)
        return self.processInputQuery(modelOnnxMarabou)
    
    def processInputQuery(self, net):
        net.saveQuery("processInputQuery")
        return MarabouCore.preprocess(net.getMarabouQuery(), self.optionsObj)
    
    #FIXME should work on MaxPool layers too
    #replaceW, replaceB = maskAndDensifyNDimConv(np.ones((2,2,1,1)), np.array([0.5]), np.ones((3,3,1)), (3,3,1), (3,3,1), (1,1))
    def maskAndDensifyNDimConv(origW, origB, mask, convInShape, convOutShape, strides, cfg_dis_w=False):
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
    
    def cloneAndMaskConvModel(self, origM, rplcLayerName, mask, evaluate=True):
        inputShape = self.ds.input_shape
        rplcIn = origM.get_layer(name=rplcLayerName).input_shape
        rplcOut = origM.get_layer(name=rplcLayerName).output_shape
        origW = origM.get_layer(name=rplcLayerName).get_weights()[0]
        origB = origM.get_layer(name=rplcLayerName).get_weights()[1]
        strides = origM.get_layer(name=rplcLayerName).strides
        clnW, clnB = ModelUtils.maskAndDensifyNDimConv(origW, origB, mask, rplcIn, rplcOut, strides)
        clnLayers = [tf.keras.Input(shape=inputShape, name="input_clnM")]
        toSetWeights = {}
        lSuffix = "_clnM_{}".format(ModelUtils.numClones)
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
            name=("AbsModel_{}".format(ModelUtils.numClones))
        )
        ModelUtils.incNumClones()
    
        clnM.build(input_shape=self.ds.featureShape)
        clnM.compile(loss=self.ds.loss, optimizer=self.ds.optimizer, metrics=self.ds.metrics)
        clnM.summary()
    
        for l,w in toSetWeights.items():
            clnM.get_layer(name=l).set_weights(w)
    
        if evaluate:
            score = clnM.evaluate(self.ds.x_test, self.ds.y_test, verbose=0)
            CnnAbs.printLog("(Clone, neurons masked:{}%) Test loss:{}".format(100*(1 - np.average(mask)), score[0]))
            CnnAbs.printLog("(Clone, neurons masked:{}%) Test accuracy:{}".format(100*(1 - np.average(mask)), score[1]))
    
            if np.all(np.equal(mask, np.ones_like(mask))):
                if np.all(np.isclose(clnM.predict(self.ds.x_test), origM.predict(self.ds.x_test))):
                    #if np.all(np.equal(clnM.predict(self.ds.x_test), origM.predict(self.ds.x_test))):
                    CnnAbs.printLog("Prediction aligned")
                else:
                    CnnAbs.printLog("Prediction not aligned")
    
        return clnM
    
    def genValidationModel(self, validation):
        if "base" in validation: 
            if validation == "mnist_base_4":
                num_ch = 4
            elif validation == "mnist_base_2":
                num_ch = 2
            elif validation == "mnist_base_1":
                num_ch = 1
            else:
                raise Exception("Validation net {} not supported".format(validation))
            return  tf.keras.Sequential(
                [
                    tf.keras.Input(shape=self.ds.input_shape),
                    layers.Conv2D(num_ch, kernel_size=(3, 3), activation="relu", name="c1"),
                    layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                    layers.Conv2D(num_ch, kernel_size=(2, 2), activation="relu", name="c2"),
                    layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                    layers.Flatten(name="f1"),
                    layers.Dense(40, activation="relu", name="fc1"),
                    layers.Dense(self.ds.num_classes, activation=None, name="sm1"),
                ],
                name="origModel_" + validation
            )
        if "long" in validation:
            if validation == "mnist_long_4":
                num_ch = 4
            elif validation == "mnist_long_2":
                num_ch = 2
            elif validation == "mnist_long_1":
                num_ch = 1
            else:
                raise Exception("Validation net {} not supported".format(validation))
            return  tf.keras.Sequential(
                [
                    tf.keras.Input(shape=self.ds.input_shape),
                    layers.Conv2D(num_ch, kernel_size=(3, 3), activation="relu", name="c0"),
                    layers.MaxPooling2D(pool_size=(2, 2), name="mp0"),
                    layers.Conv2D(num_ch, kernel_size=(2, 2), activation="relu", name="c1"),
                    layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                    layers.Conv2D(num_ch, kernel_size=(3, 3), activation="relu", name="c2"),
                    layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                    layers.Flatten(name="f1"),
                    layers.Dense(40, activation="relu", name="fc1"),
                    layers.Dense(self.ds.num_classes, activation=None, name="sm1"),
                ],
                name="origModel_" + validation
            )
    
    def genCnnForAbsTest(self, cfg_limitCh=True, cfg_freshModelOrig=False, savedModelOrig="cnn_abs_orig.h5", cnnSizeChoice = "small", validation=None):
    
        CnnAbs.printLog("Starting model building")
        #https://keras.io/examples/vision/mnist_convnet/
    
        if not validation:
            savedModelOrig = savedModelOrig.replace(".h5", "_" + cnnSizeChoice + ".h5")
        else:
            savedModelOrig = savedModelOrig.replace(".h5", "_" + "validation_" + validation + ".h5")

        basePath = "/cs/labs/guykatz/matanos/Marabou/maraboupy" #FIXME ugly
        noModel = not os.path.exists(basePath + "/" + savedModelOrig)
    
        if cfg_freshModelOrig or noModel:
            if not validation:
                if cnnSizeChoice in ["big"]:
                    num_ch = 32
                elif cnnSizeChoice in ["medium"]:
                    num_ch = 16
                elif cnnSizeChoice in ["small"]:
                    num_ch = 1
                elif cnnSizeChoice == "toy":
                    raise Exception("Toy network is not meant to be retrained")
                else:
                    raise Exception("cnnSizeChoice {} not supported".format(cnnSizeChoice))
                origM = tf.keras.Sequential(
                    [
                        tf.keras.Input(shape=self.ds.input_shape),
                        layers.Conv2D(num_ch, kernel_size=(3, 3), activation="relu", name="c1"),
                        layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
                        layers.Conv2D(num_ch, kernel_size=(2, 2), activation="relu", name="c2"),
                        layers.MaxPooling2D(pool_size=(2, 2), name="mp2"),
                        layers.Flatten(name="f1"),
                        layers.Dense(40, activation="relu", name="fc1"),
                        layers.Dense(self.ds.num_classes, activation=None, name="sm1"),
                    ],
                    name="origModel_" + cnnSizeChoice
                )
            else:
                origM = genValidationModel(validation)
    
            batch_size = 128
            epochs = 15
    
            origM.build(input_shape=self.ds.featureShape)
            origM.summary()
    
            origM.compile(optimizer=self.ds.optimizer, loss=myLoss, metrics=[tf.keras.metrics.SparseCategoricalAccuracy()])
            #origM.compile(optimizer=self.ds.optimizer, loss=self.ds.loss, metrics=self.ds.metrics)
            origM.fit(self.ds.x_train, self.ds.y_train, epochs=epochs, batch_size=batch_size, validation_split=0.1)
    
            score = origM.evaluate(self.ds.x_test, self.ds.y_test, verbose=0)
            CnnAbs.printLog("(Original) Test loss:{}".format(score[0]))
            CnnAbs.printLog("(Original) Test accuracy:{}".format(score[1]))
    
            origM.save(basePath + "/" + savedModelOrig)
    
        else:
            origM = load_model(basePath + "/" + savedModelOrig, custom_objects={'myLoss': myLoss})
            #origM = load_model(basePath + "/" + savedModelOrig)
            origM.summary()
            score = origM.evaluate(self.ds.x_test, self.ds.y_test, verbose=0)
            CnnAbs.printLog("(Original) Test loss:".format(score[0]))
            CnnAbs.printLog("(Original) Test accuracy:".format(score[1]))
    
        return origM

    @staticmethod
    def cexToImage(valDict, prop, inputVarsMapping=None, outputVarsMapping=None, useMapping=True):
        if useMapping:
            lBounds = InputQueryUtils.getBoundsInftyBall(prop.xAdv, prop.inDist)[0]
            fail = False
            for (indOrig,indCOI) in enumerate(np.nditer(np.array(inputVarsMapping), flags=["refs_ok"])):
                if indCOI.item() is None:
                    CnnAbs.printLog("Failure. indOrig={}, indCOI={}".format(indOrig, indCOI))
                    fail = True
            if fail:
                CnnAbs.printLog("inputVarsMapping={}".format(inputVarsMapping))
            inputDict  = {indOrig : valDict[indCOI.item()] if indCOI.item() != -1 else lBnd.item() for (indOrig, indCOI),lBnd in zip(enumerate(np.nditer(np.array(inputVarsMapping) , flags=["refs_ok"])), np.nditer(lBounds))}
            outputDict = {indOrig : valDict[indCOI.item()] if indCOI.item() != -1 else 0    for (indOrig, indCOI)      in     enumerate(np.nditer(np.array(outputVarsMapping), flags=["refs_ok"]))}
    
            cex           = np.array([valDict[i.item()] if i.item() != -1 else lBnd for i,lBnd in zip(np.nditer(np.array(inputVarsMapping), flags=["refs_ok"]), np.nditer(lBounds))]).reshape(prop.xAdv.shape)
            cexPrediction = np.array([valDict[o.item()] if o.item() != -1 else 0 for o in np.nditer(np.array(outputVarsMapping), flags=["refs_ok"])]).reshape(outputVarsMapping.shape)
        else:
            inputDict = {i.item():valDict[i.item()] for i in np.nditer(inputVarsMapping)}
            outputDict = {o.item():valDict[o.item()] for o in np.nditer(outputVarsMapping)}
            cex = np.array([valDict[i.item()] for i in np.nditer(inputVarsMapping)]).reshape(prop.xAdv.shape)
            cexPrediction = np.array([valDict[o.item()] for o in np.nditer(outputVarsMapping)])
        return cex, cexPrediction, inputDict, outputDict        

    @staticmethod
    #Return bool, bool: Left is wether yCorrect is the maximal one, Right is wether yBad > yCorrect.
    def isCEXSporious(model, x, inDist, outSlack, yCorrect, yBad, cex, sporiousStrict=True):
        inBounds, violations =  InputQueryUtils.inBoundsInftyBall(x, inDist, cex)
        if not inBounds:
            raise Exception("CEX out of bounds, violations={}, values={}".format(np.transpose(violations.nonzero()), np.absolute(cex-x)[violations.nonzero()]))
        prediction = model.predict(np.array([cex]))
        if not sporiousStrict:
            return prediction.argmax() == yCorrect
        return prediction[0,yBad] + outSlack < prediction[0,yCorrect]

    @staticmethod
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
    

#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################


class InputQueryUtils:

    @staticmethod
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
        CnnAbs.printLog("varsWithoutIngoingEdges = {}".format(varsWithoutIngoingEdges))
        for v in varsWithoutIngoingEdges:
            if not net.lowerBoundExists(v):
                raise Exception("inaccessible w/o lower bounds: {}".format(v))
            if not net.upperBoundExists(v):
                raise Exception("inaccessible w/o upper bounds: {}".format(v))
        net.inputVars.append(np.array([v for v in varsWithoutIngoingEdges]))

    @staticmethod        
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
        if inputVarsMapping is None or outputVarsMapping is None: #I made this change now to test inputVarsMapping==None
            raise Exception("None input/output varsMapping")
        return inputVarsMapping, outputVarsMapping, varsMapping

    @staticmethod
    def setCOIBoundes(net, init):
    
        CnnAbs.printLog("net.numVars={}".format(net.numVars))    
        CnnAbs.printLog("len(net.equList)={}".format(len(net.equList)))
        CnnAbs.printLog("len(net.maxList)={}".format(len(net.maxList)))
        CnnAbs.printLog("len(net.reluList)={}".format(len(net.reluList)))
        CnnAbs.printLog("len(net.absList)={}".format(len(net.absList)))
        CnnAbs.printLog("len(net.signList)={}".format(len(net.signList)))
        CnnAbs.printLog("len(net.lowerBounds)={}".format(len(net.lowerBounds)))
        CnnAbs.printLog("len(net.upperBounds)={}".format(len(net.upperBounds)))
        CnnAbs.printLog("sum([i.size for i in net.inputVars])={}".format(sum([i.size for i in net.inputVars])))
        CnnAbs.printLog("net.outputVars.size={}".format(net.outputVars.size))
    
        reach = set(init)
        lastLen = 0
        while len(reach) > lastLen:
            lastLen = len(reach)
            reachPrev = reach.copy()
            for eqi, eq in enumerate(net.equList):
                if (eq.addendList[-1][1] in reachPrev) and (eq.addendList[-1][0] == -1) and (eq.EquationType == MarabouCore.Equation.EQ):
                    [reach.add(v) for w,v in eq.addendList[:-1] if w != 0]
                elif (eq.EquationType != MarabouCore.Equation.EQ) or (eq.addendList[-1][0] != -1):
                    [reach.add(v) for w,v in eq.addendList]
                    CnnAbs.printLog("eqi={}, eq.addendList={}, eq.scalar={}, eq.EquationType={}".format(eqi, eq.addendList, eq.scalar, eq.EquationType))
            for maxArgs, maxOut in net.maxList:
                if maxOut in reachPrev:
                    [reach.add(arg) for arg in maxArgs]
            [reach.add(vin) for vin,vout in net.reluList if vout in reachPrev]
            [reach.add(vin) for vin,vout in net.absList  if vout in reachPrev]
            [reach.add(vin) for vin,vout in net.signList if vout in reachPrev]
            if len(net.disjunctionList) > 0:
                raise Exception("Not implemented")
        unreach = set([v for v in range(net.numVars) if v not in reach])
    
        CnnAbs.printLog("COI : reached={}, unreached={}, out_of={}".format(len(reach), len(unreach), net.numVars))
    
        inputVarsMapping, outputVarsMapping, varsMapping = InputQueryUtils.removeRedundantVariables(net, reach)
        for eq in net.equList:
            for w,v in eq.addendList:
                if v > net.numVars:
                    CnnAbs.printLog("eq.addendList={}, eq.scalar={}, eq.EquationType={}".format(eq.addendList, eq.scalar, eq.EquationType))
        CnnAbs.printLog("net.numVars={}".format(net.numVars))
        CnnAbs.printLog("len(net.equList)={}".format(len(net.equList)))
        CnnAbs.printLog("len(net.maxList)={}".format(len(net.maxList)))
        CnnAbs.printLog("len(net.reluList)={}".format(len(net.reluList)))
        CnnAbs.printLog("len(net.absList)={}".format(len(net.absList)))
        CnnAbs.printLog("len(net.signList)={}".format(len(net.signList)))
        CnnAbs.printLog("len(net.lowerBounds)={}".format(len(net.lowerBounds)))
        CnnAbs.printLog("len(net.upperBounds)={}".format(len(net.upperBounds)))
        CnnAbs.printLog("sum([i.size for i in net.inputVars])={}".format(sum([i.size for i in net.inputVars])))
        CnnAbs.printLog("net.outputVars.size={}".format(net.outputVars.size))
        CnnAbs.printLog("COI : reached={}, unreached={}, out_of={}".format(len(reach), len(unreach), net.numVars))
        return inputVarsMapping, outputVarsMapping, varsMapping

    @staticmethod
    def setBounds(model, boundDict):
        if boundDict:
            for i, (lb, ub) in boundDict.items():
                if i < model.numVars: #This might mean that there is disalignment between the queries' definition of variables
                    if (i not in model.lowerBounds) or (model.lowerBounds[i] < lb):
                        model.setLowerBound(i,lb)
                    if (i not in model.upperBounds) or (ub < model.upperBounds[i]):
                        model.setUpperBound(i,ub)

    @staticmethod                        
    def getBoundsInftyBall(x, r, pos=True, floatingPointErrorGap=0):
        if r > floatingPointErrorGap and floatingPointErrorGap > 0:
            r -= floatingPointErrorGap
        if pos:
            return np.maximum(x - r,np.zeros(x.shape)), x + r
        return x - r, x + r

    @staticmethod    
    def inBoundsInftyBall(x, r, p, pos=True, allowClose=True):
        assert p.shape == x.shape
        l,u = InputQueryUtils.getBoundsInftyBall(x,r,pos=pos)
        if allowClose:
            geqLow = np.logical_or(np.less_equal(l,p), np.isclose(l,p))    
            leqUp  = np.logical_or(np.less_equal(p,u), np.isclose(p,u))
        else:
            geqLow = np.less_equal(l,p)
            leqUp  = np.less_equal(p,u)
        inBounds = np.logical_and(geqLow, leqUp)
        violations = np.logical_not(inBounds)
        assert violations.shape == x.shape
        return np.all(inBounds), violations #FIXME shouldn't allow isclose, floating point errors?

    @staticmethod
    def setAdversarial(net, x, inDist, outSlack, yCorrect, yBad):
        inAsNP = np.array(net.inputVars[0])
        x = x.reshape(inAsNP.shape)
        xDown, xUp = InputQueryUtils.getBoundsInftyBall(x, inDist, floatingPointErrorGap=0.0025)
        floatingPointErrorGapOutput = 0.03
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
        net.addInequality([yCorrectVar, yBadVar], [1,-1], outSlack - floatingPointErrorGapOutput) # correct + floatingPointErrorGap <= bad + slack
        return net
                            
    

#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
                
#def printImg(image, title):
#    if not isinstance(image,list):
#        imlist = [image]
#    else:
#        imlist = image
#    for i, im in enumerate(imlist):
#        if len(im.shape) == 1:
#            im = im.reshape(im.shape + (1,))
#        if len(imlist) > 1:
#            suff = "_" + str(i)
#        else:
#            suff = ""
#        plt.title(title + suff)
#        plt.imshow(im, cmap='Greys')
#        plt.savefig(title + suff + ".png")
#
#def printAvgDomain(model, from_label=False): #from_label or from_prediction
#    if from_label:
#        x_test_by_class = {label : np.asarray([x for x,y in zip(self.ds.x_test, self.ds.y_test) if y == label]) for label in range(self.ds.num_classes)}
#    else:
#        predictions =  model.predict(self.ds.x_test)
#        x_test_by_class = {label : np.asarray([x for x,y in zip(self.ds.x_test, predictions) if y.argmax() == label]) for label in range(self.ds.num_classes)}
#    meanAct = [meanActivation(model, model.layers[-1].name, x_test_by_class[y]) for y in range(self.ds.num_classes)]
#    for y in range(self.ds.num_classes):
#        printImg(meanAct[y], "{}_meanAct_label_{}.png".format(model.name, y))
#
