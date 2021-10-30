import sys
import os
import time

import keras2onnx
import tensorflow as tf
from maraboupy import MarabouNetworkONNX
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

import copy

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
        
    def rankByScore(model, score):
        score = CnnAbs.flattenTF(score)
        if score.shape != model.outputVars.shape:
            print(score.shape)
            print(model.outputVars.shape)
        assert score.shape == model.outputVars.shape
        scoreSort = np.argsort(score)
        return model.outputVars.flatten()[scoreSort]

    def rankAbsLayer(self, model, prop, absLayerPredictions):
        raise NotImplementedError    

    @staticmethod
    def linearStep(n, stepSize=10, startWith=50):
        stepSize = max(stepSize,1)
        startWith = max(startWith,0)
        size = n
        yield startWith
        size -= startWith
        while size > 0:
            toAdd = min(size, stepSize)
            yield toAdd
            size -= toAdd


    @staticmethod
    def geometricStep(n, initStepSize=10, startWith=0, factor=2):
        stepSize = max(initStepSize,1)
        startWith = max(startWith,0)
        size = n
        yield startWith
        size -= startWith
        i = 0
        while size > 0:
            toAdd = min(size, initStepSize * (factor ** i))
            yield toAdd
            size -= toAdd
            i += 1

    @classmethod
    def steps(cls, n):
        #return cls.linearStep(n)
        return cls.geometricStep(n)
    
#Policy - Most important neurons are the center of the image.
class PolicyCentered(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.Centered

    def rankAbsLayer(self, model, prop, absLayerPredictions):
        assert len(absLayerPredictions.shape) == 4
        absLayerShape = absLayerPredictions.shape[1:]
        corMeshList = np.meshgrid(*[np.arange(d) for d in absLayerShape], indexing='ij')
        center = [(float(d)-1) / 2 for d in absLayerShape]
        distance = np.linalg.norm(np.array([cor - cent for cor, cent in zip(corMeshList, center)]), axis=0)
        score = -distance
        return PolicyBase.rankByScore(model, score)


#Policy - Refine stepsize most activated neurons, calculating activation on the entire Mnist test.    
class PolicyAllSamplesRank(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.AllSamplesRank

    def rankAbsLayer(self, model, prop, absLayerPredictions):
        score = np.mean(absLayerPredictions, axis=0)
        return PolicyBase.rankByScore(model, score)

#Policy - Refine stepsize most activated neurons, calculating activation on the Mnist test examples labeled the same as prediction label.    
class PolicySingleClassRank(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.SingleClassRank

    def rankAbsLayer(self, model, prop, absLayerPredictions):        
        score = np.mean(np.array([alp for alp, label in zip(absLayerPredictions, self.ds.y_test) if label == prop.yMax]), axis=0)
        return PolicyBase.rankByScore(model, score)
    
#Policy - calculate per class
class PolicyMajorityClassVote(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)  
        self.policy = Policy.MajorityClassVote  
            
    def rankAbsLayer(self, model, prop, absLayerPredictions):
        actByClasses = np.array([np.mean(np.array([alp for alp,y in zip(absLayerPredictions, self.ds.y_test) if y == label]), axis=0) for label in range(self.ds.num_classes)])
        score = np.linalg.norm(actByClasses, axis=0)
        return PolicyBase.rankByScore(model, score)
    
#Policy - Add neurons randomly. 
class PolicyRandom(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)  
        self.policy = Policy.Random  

    def rankAbsLayer(self, model, prop, absLayerPredictions):
        return np.random.permutation(model.outputVars)

#Policy - No abstraction.
class PolicyVanilla(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.Vanilla
        self.coi = False
        
    def rankAbsLayer(self, model, prop, absLayerPredictions):
        raise NotImplementedError

#Policy - Rank According to activation values of single sample
class PolicySampleRank(PolicyBase):
    
    def __init__(self, ds):
        super().__init__(ds)
        self.policy = Policy.SampleRank

    def rankAbsLayer(self, model, prop, absLayerPredictions):
        score = np.abs(absLayerPredictions[prop.xAdvIndex])
        return PolicyBase.rankByScore(model, score)
    
    
class Policy(Enum):
    Centered          = 0
    AllSamplesRank    = 1
    SingleClassRank   = 2
    MajorityClassVote = 3
    Random            = 4
    Vanilla           = 5    
    SampleRank        = 6

    @staticmethod
    def absPolicies():
        return [Policy.Centered.name, Policy.AllSamplesRank.name, Policy.SingleClassRank.name, Policy.MajorityClassVote.name, Policy.Random.name, Policy.SampleRank.name]

    @staticmethod
    def solvingPolicies():
        return Policy.absPolicies() + [Policy.Vanilla.name]

    @staticmethod
    def fromString(s, ds):
        s = s.lower()
        if   s == Policy.Centered.name.lower():
            return PolicyCentered(ds)
        elif s == Policy.AllSamplesRank.name.lower():
            return PolicyAllSamplesRank(ds)
        elif s == Policy.SingleClassRank.name.lower():
            return PolicySingleClassRank(ds)
        elif s == Policy.MajorityClassVote.name.lower():
            return PolicyMajorityClassVote(ds)
        elif s == Policy.Random.name.lower():
            return PolicyRandom(ds)
        elif s == Policy.Vanilla.name.lower():
            return PolicyVanilla(ds)
        elif s == Policy.SampleRank.name.lower():
            return PolicySampleRank(ds)
        else:
            raise NotImplementedError

class AdversarialProperty:
    def __init__(self, xAdv, yMax, ySecond, inDist, outSlack, sample):
        self.xAdv = xAdv
        self.yMax = yMax
        self.ySecond = ySecond
        self.inDist = inDist
        self.outSlack = outSlack
        self.xAdvIndex = sample

class Result(Enum):
    
    TIMEOUT = 0
    SAT = 1
    UNSAT = 2
    GTIMEOUT = 3
    SPURIOUS = 4
    ERROR = 5

    @classmethod
    def str2Result(cls, s):
        s = s.lower()
        if s == "timeout":
            return cls.TIMEOUT
        elif s == "sat":
            return cls.SAT
        elif s == "unsat":
            return cls.UNSAT
        elif s == "gtimeout":
            return cls.GTIMEOUT
        elif s == "spurious":
            return cls.SPURIOUS
        elif s == "error":
            return cls.ERROR
        else:
            raise NotImplementedError            
        
class ResultObj:
    
    def __init__(self, result):
        self.originalQueryStats = None
        self.finalQueryStats = None
        self.cex = None
        self.cexPrediction = np.array([])
        self.inputDict = dict()
        self.outputDict = dict()
        self.result = Result.str2Result(result)

    def timedOut(self):
        return (self.result is Result.TIMEOUT) or (self.result is Result.GTIMEOUT)

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

    def returnResult(self):
        CnnAbs.printLog("\n*******************************************************************************\n")
        CnnAbs.printLog("\n\nResult is:\n\n    {}\n\n".format(self.result.name))        
        return self.result.name, self.cex

class DataSet:

    def __init__(self, ds='mnist'):
        if ds.lower() == 'mnist':
            self.setMnist()
        else:
            raise NotImplementedError

    @staticmethod
    def readFromFile(dataset):
        
        with open('/'.join([CnnAbs.maraboupyPath, dataset, 'x_train.npy']), 'rb') as f:
            x_train = np.load(f, allow_pickle=True)
        with open('/'.join([CnnAbs.maraboupyPath, dataset, 'y_train.npy']), 'rb') as f:
            y_train = np.load(f, allow_pickle=True)
        with open('/'.join([CnnAbs.maraboupyPath, dataset, 'x_test.npy']), 'rb') as f:
            x_test = np.load(f, allow_pickle=True)
        with open('/'.join([CnnAbs.maraboupyPath, dataset, 'y_test.npy']), 'rb') as f:
            y_test = np.load(f, allow_pickle=True)            
        return (x_train, y_train), (x_test, y_test)
    
    def setMnist(self):
        self.num_classes = 10
        self.input_shape = (28,28,1)
        #(self.x_train, self.y_train), (self.x_test, self.y_test) = tf.keras.datasets.mnist.load_data()
        (self.x_train, self.y_train), (self.x_test, self.y_test) = DataSet.readFromFile('mnist')
        self.x_train, self.x_test = self.x_train / 255.0, self.x_test / 255.0
        self.x_train = np.expand_dims(self.x_train, -1)
        self.x_test = np.expand_dims(self.x_test, -1)
        self.featureShape=(1,28,28)
        self.loss='sparse_categorical_crossentropy'
        self.optimizer='adam'
        self.metrics=['accuracy']
        self.valueRange = (0,1) #Input pixels value range
        self.name = 'mnist'
    
class CnnAbs:

    logger = None
    basePath = os.getcwd()
    maraboupyPath = basePath.split('maraboupy', maxsplit=1)[0] + 'maraboupy'
    marabouPath = basePath.split('Marabou', maxsplit=1)[0] + 'Marabou'    
    resultsFile = 'Results'
    resultsFileBrief = 'ResultsBrief'
    
    def __init__(self, ds='mnist', dumpDir='', options=None, logDir='', gtimeout=7200, policy=None, abstractFirst=False):
        optionsObj = Marabou.createOptions(**options)
        logDir = "/".join(filter(None, [CnnAbs.basePath, logDir]))
        self.ds = DataSet(ds)
        self.optionsObj = optionsObj
        self.logDir = logDir
        if not self.logDir.endswith("/"):
            self.logDir += "/"
        os.makedirs(self.logDir, exist_ok=True)
        if CnnAbs.logger == None:
            CnnAbs.setLogger(logDir=self.logDir)
        if dumpDir:            
            self.dumpDir = dumpDir
            if not self.dumpDir.endswith("/"):
                self.dumpDir += "/"            
            if self.dumpDir and not os.path.exists(self.dumpDir):
                os.mkdir(self.dumpDir)
        else:
            self.dumpDir = self.logDir    
        if os.path.exists(self.logDir + CnnAbs.resultsFile + ".json"):
            self.resultsJson = self.loadJson(CnnAbs.resultsFile, loadDir=self.logDir)
        else:
            self.resultsJson = dict(subResults=[])
        self.numSteps = None
        self.startTotal = time.time()
        self.gtimeout = gtimeout
        self.prevTimeStamp = time.time()
        self.policy = Policy.fromString(policy, self.ds.name)
        self.modelUtils = ModelUtils(self.ds, self.optionsObj, self.logDir)
        self.abstractFirst = abstractFirst

    def solveAdversarial(self, modelTF, policyName, sample, propDist, propSlack=0):
        if not self.tickGtimeout():
            return self.returnGtimeout()        
        policy = Policy.fromString(policyName, self.ds.name)
        xAdv = self.ds.x_test[sample]
        yAdv = self.ds.y_test[sample]
        yPredict = modelTF.predict(np.array([xAdv]))
        yMax = yPredict.argmax()
        yPredictNoMax = np.copy(yPredict)
        yPredictNoMax[0][yMax] = np.min(yPredict)
        ySecond = yPredictNoMax.argmax()
        if ySecond == yMax:
            ySecond = 0 if yMax > 0 else 1
            
        prop = AdversarialProperty(xAdv, yMax, ySecond, propDist, propSlack, sample)
        mbouModel = self.modelUtils.tf2Model(modelTF)
        InputQueryUtils.setAdversarial(mbouModel, xAdv, propDist, propSlack, yMax, ySecond, valueRange=self.ds.valueRange)        

        fName = "xAdv.png"
        CnnAbs.printLog("Printing original input to file {}, this is sample {} with label {}".format(fName, sample, yAdv))
        plt.figure()        
        plt.imshow(np.squeeze(xAdv), cmap='Greys')
        plt.colorbar()
        plt.title('Example {}. DataSet Label: {}, model Predicts {}'.format(sample, yAdv, yMax))
        plt.savefig(self.logDir + fName)
        with open(self.logDir + fName.replace("png","npy"), "wb") as f:
            np.save(f, xAdv)            
        self.resultsJson["yDataset"] = int(yAdv.item())
        self.resultsJson["yMaxPrediction"] = int(yMax)
        self.resultsJson["ySecondPrediction"] = int(ySecond)
        self.dumpResultsJson()
        if not self.tickGtimeout():
            return self.returnGtimeout()        
        return self.solve(mbouModel, modelTF, policy, prop, generalRunName="sample_{},policy_{},propDist_{}".format(sample, policyName, str(propDist).replace('.','-')))



    def solve(self, mbouModel, modelTF, policy, prop, generalRunName=""):
        startBoundTightening = time.time()
        if not self.tickGtimeout():
            return self.returnGtimeout()
        CnnAbs.printLog("Started dumping bounds - used for abstraction")
        ipq = self.propagateBounds(mbouModel)
        if not self.tickGtimeout():
            return self.returnGtimeout()        
        MarabouCore.saveQuery(ipq, self.logDir + "IPQ_dumpBounds")
        CnnAbs.printLog("Finished dumping bounds - used for abstraction")
        endBoundTightening = time.time()
        self.resultsJson["boundTighteningRuntime"] = endBoundTightening - startBoundTightening    
        if ipq.getNumberOfVariables() == 0:
            self.resultsJson["SAT"] = False
            self.resultsJson["Result"] = "UNSAT"
            self.resultsJson["successfulRuntime"] = endBoundTightening - startBoundTightening
            self.resultsJson["successfulRun"] = 0
            self.resultsJson["totalRuntime"] = time.time() - self.startTotal
            self.dumpResultsJson()
            CnnAbs.printLog("UNSAT on first LP bound tightening")
            return ResultObj("unsat").returnResult()
        else:
            os.rename(self.logDir + "dumpBounds.json", self.logDir + "dumpBoundsInitial.json") #This is to prevent accidental override of this file.
        if os.path.isfile(self.logDir + "dumpBoundsInitial.json"):
            boundList = self.loadJson("dumpBoundsInitial", loadDir=self.logDir)
            boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}
        else:
            boundDict = None

        self.optionsObj._dumpBounds = False
        self.modelUtils.optionsObj._dumpBounds = False        
        originalQueryStats = self.dumpQueryStats(mbouModel, "originalQueryStats_" + generalRunName)        
        successful = None
        absRefineBatches = self.abstractionRefinementBatches(mbouModel, modelTF, policy, prop)
        self.numSteps = len(absRefineBatches)
        self.resultsJson["absRefineBatches"] = self.numSteps
        self.dumpResultsJson()

        CnnAbs.printLog("Abstraction-refinement steps up to full network = {}".format(len(absRefineBatches)))
        for i, abstractNeurons in enumerate(absRefineBatches):
            if self.isGlobalTimedOut():
                break
            mbouModelAbstract, inputVarsMapping, outputVarsMapping, varsMapping = self.abstractAndPrune(mbouModel, abstractNeurons, boundDict)
            if i+1 == len(absRefineBatches):
                self.optionsObj._timeoutInSeconds = 0
            runName = generalRunName + ",step_{}_outOf_{}".format(i, len(absRefineBatches)-1)
            if not self.tickGtimeout():
                return self.returnGtimeout()
            resultObj = self.runMarabou(mbouModelAbstract, prop, runName, inputVarsMapping=inputVarsMapping, outputVarsMapping=outputVarsMapping, varsMapping=varsMapping, modelTF=modelTF, originalQueryStats=originalQueryStats)
            if not self.tickGtimeout():
                return self.returnGtimeout()            
            if resultObj.timedOut():
                continue
            if resultObj.sat():
                try:
                    isSpurious = ModelUtils.isCEXSpurious(modelTF, prop, resultObj.cex, valueRange=self.ds.valueRange)
                except Exception as err:
                    CnnAbs.printLog(err)
                    isSpurious = True
                CnnAbs.printLog("Found {} CEX in step {}/{}.".format("spurious" if isSpurious else "real", i, len(absRefineBatches)-1))
                if not isSpurious:
                    successful = i
                    break
                elif i+1 == len(absRefineBatches):
                    resultObj = ResultObj("error")
                    break 
            elif resultObj.unsat():
                CnnAbs.printLog("Found UNSAT in step {}/{}.".format(i, len(absRefineBatches)-1))
                successful = i
                break
            else:
                raise NotImplementedError

            if not self.tickGtimeout():
                return self.returnGtimeout()
        globalTimeout = self.isGlobalTimedOut()    
        if globalTimeout:
            resultObj = ResultObj("gtimeout")
        success = not resultObj.timedOut() and (successful is not None)
        if resultObj.sat() and not success:
            resultObj = ResultObj("spurious")
    
        if success:
            CnnAbs.printLog("successful={}/{}".format(successful, self.numSteps-1)) if (successful+1) < self.numSteps else CnnAbs.printLog("successful=Full")
            self.resultsJson["successfulRuntime"] = self.resultsJson["subResults"][-1]["runtime"]
            self.resultsJson["successfulRun"] = successful + 1        
        self.resultsJson["totalRuntime"] = time.time() - self.startTotal
        self.resultsJson["SAT"] = resultObj.sat()
        self.resultsJson["Result"] = resultObj.result.name
        self.dumpResultsJson()
        return resultObj.returnResult()
    

    def abstractionRefinementBatches(self, model, modelTF, policy, prop):
        if policy.policy is Policy.Vanilla:
            return [set()]
        layerList, layerTypes = InputQueryUtils.divideToLayers(model)
        assert all([len(t) == 1 for t in layerTypes])
        if self.abstractFirst:
            absLayer = next(i for i,t in enumerate(layerTypes) if 'Relu' in t)
            absLayerLayerName = 'c1'
        else:
            absLayer = [i for i,t in enumerate(layerTypes) if 'Max' in t][-1]
            absLayerLayerName = next(layer.name for layer in modelTF.layers[::-1] if isinstance(layer, tf.keras.layers.MaxPooling2D) or isinstance(layer, tf.keras.layers.MaxPooling1D))
        modelTFUpToAbsLayer = ModelUtils.intermidModel(modelTF, absLayerLayerName)
        absLayerActivation = modelTFUpToAbsLayer.predict(self.ds.x_test)

        netPriorToAbsLayer = list(set().union(*layerList[:absLayer+1]))
        netPriorToAbsLayer.sort()
        modelUpToAbsLayer = copy.deepcopy(model)
        modelUpToAbsLayer.outputVars = np.sort(np.array(list(layerList[absLayer])))
        _ , _ , varsMapping = InputQueryUtils.removeVariables(modelUpToAbsLayer, netPriorToAbsLayer, keepInputShape=True)

        cwd = os.getcwd()
        os.chdir(self.logDir)        
        for j in random.sample(range(len(self.ds.x_test)), 10):
            tfout = absLayerActivation[j]
            mbouout = modelUpToAbsLayer.evaluate(self.ds.x_test[j])
            assert np.all( np.isclose(CnnAbs.flattenTF(tfout), mbouout, atol=1e-4) )
            assert np.all( np.isclose(tfout, CnnAbs.reshapeMbouOut(mbouout, tfout.shape), atol=1e-4 ) )
        os.chdir(cwd)
        
        absLayerRankAcsending = [varsMapping[v] for v in policy.rankAbsLayer(modelUpToAbsLayer, prop, absLayerActivation)]
        steps = list(policy.steps(len(absLayerRankAcsending)))
        batchSizes = [len(absLayerRankAcsending) - sum(steps[:i+1]) for i in range(len(steps))]
        return [set(absLayerRankAcsending[:batchSize]) for batchSize in batchSizes]

    @staticmethod
    def flattenTF(tfout):
        assert len(tfout.shape) == 3
        if tfout.shape[2] > 1:
            tfout = np.swapaxes(tfout, 0, 2)
            tfout = np.swapaxes(tfout, 1, 2)
        return tfout.flatten()

    @staticmethod    
    def reshapeMbouOut(mbouout, shape):
        assert len(shape) == 3        
        shape = np.roll(shape, 1)
        return np.swapaxes(np.swapaxes(mbouout.reshape(shape), 1, 2), 0, 2)
                
    def abstractAndPrune(self, model, abstractNeurons, boundDict):
        modelAbstract = copy.deepcopy(model)
        modelAbstract.equList  = [eq for eq in modelAbstract.equList  if eq.addendList[-1][1] not in abstractNeurons]
        modelAbstract.maxList  = [pw for pw in modelAbstract.maxList  if pw[1]                not in abstractNeurons]        
        modelAbstract.reluList = [pw for pw in modelAbstract.reluList if pw[1]                not in abstractNeurons]
        modelAbstract.absList  = [pw for pw in modelAbstract.absList  if pw[1]                not in abstractNeurons]
        modelAbstract.signList = [pw for pw in modelAbstract.signList if pw[1]                not in abstractNeurons]
        modelAbstract.inputVars.append(np.array(list(abstractNeurons)))
        for v in abstractNeurons:
            modelAbstract.setLowerBound(v, boundDict[v][0])
            modelAbstract.setUpperBound(v, boundDict[v][1])        
        inputVarsMapping, outputVarsMapping, varsMapping = InputQueryUtils.pruneUnreachableNeurons(modelAbstract, modelAbstract.outputVars.flatten().tolist())
        return modelAbstract, inputVarsMapping, outputVarsMapping, varsMapping

    def runMarabou(self, model, prop, runName="runMarabouOnKeras", inputVarsMapping=None, outputVarsMapping=None, varsMapping=None, modelTF=None, originalQueryStats=None):

        startLocal = time.time()    
        self.subResultAppend()
        finalQueryStats = self.dumpQueryStats(model, "finalQueryStats_" + runName)
        
        CnnAbs.printLog("----- Start Solving {}".format(runName))
        if self.optionsObj._timeoutInSeconds <= 0:
            self.optionsObj._timeoutInSeconds = self.gtimeout
        else:
            self.optionsObj._timeoutInSeconds = int(min(self.optionsObj._timeoutInSeconds, self.gtimeout))
        cwd = os.getcwd()
        os.chdir(self.logDir)
        vals, stats = Marabou.solve_query(model.getMarabouQuery(), verbose=False, options=self.optionsObj)
        os.chdir(cwd)
        CnnAbs.printLog("----- Finished Solving {}".format(runName))
        sat = len(vals) > 0
        timedOut = stats.hasTimedOut()
        if not sat:
            if timedOut:
                result = ResultObj("timeout")
                CnnAbs.printLog("----- Timed out in {}".format(runName))
            else:
                result = ResultObj("unsat")
                CnnAbs.printLog("----- UNSAT in {}".format(runName))
        else:
            cex, cexPrediction, inputDict, outputDict = ModelUtils.cexToImage(vals, prop, inputVarsMapping, outputVarsMapping, useMapping=True, valueRange=self.ds.valueRange)
            self.dumpCex(cex, cexPrediction, prop, runName, modelTF)
            self.dumpJson(inputDict, "DICT_runMarabouOnKeras_InputDict")
            self.dumpJson(outputDict, "DICT_runMarabouOnKeras_OutputDict")
            result = ResultObj("sat")
            result.setCex(cex, cexPrediction, inputDict, outputDict)
            result.vals = vals
            result.varsMapping = varsMapping
            CnnAbs.printLog("----- SAT in {}".format(runName))
        result.setStats(originalQueryStats, finalQueryStats)            

        endLocal = time.time()
        self.subResultUpdate(runtime=endLocal-startLocal, runtimeTotal=time.time() - self.startTotal, sat=result.sat(), timedOut=result.timedOut(), originalQueryStats=originalQueryStats, finalQueryStats=finalQueryStats)
        if not self.tickGtimeout():
            return ResultObj("gtimeout")
        return result

    def propagateBounds(self, mbouModel):
        mbouModelCopy = copy.deepcopy(mbouModel)
        return self.processInputQuery(mbouModelCopy) 
        
    def processInputQuery(self, net):
        net.saveQuery(self.logDir + "processInputQuery")
        cwd = os.getcwd()
        os.chdir(self.logDir)
        ipq = MarabouCore.preprocess(net.getMarabouQuery(), self.optionsObj)
        os.chdir(cwd)
        return ipq    

    def setLogger(suffix='', logDir=''):
        logging.basicConfig(level = logging.DEBUG, format = "%(asctime)s - %(name)s - %(levelname)s - %(message)s", filename = logDir + 'cnnAbsTB{}.log'.format(suffix), filemode = "w")        
        CnnAbs.logger = logging.getLogger('cnnAbsTB{}'.format(suffix))
        logging.getLogger('matplotlib.font_manager').disabled = True
        
    @staticmethod
    def printLog(s):
        if CnnAbs.logger:
            CnnAbs.logger.info(s)
        print(s)

    def dumpNpArray(self, npArray, name, saveDir='', saveAtLog=False):
        if not saveDir:
            if saveAtLog:
                saveDir = self.logDir
            else:
                saveDir = self.dumpDir
        if not name.endswith(".npy"):
            name += ".npy"
        if not saveDir.endswith("/"):
            saveDir += "/"
        with open(saveDir + name, "wb") as f:
            np.save(f, npArray)

    def dumpQueryStats(self, mbouNet, name):
        queryStats = ModelUtils.marabouNetworkStats(mbouNet)
        self.dumpJson(queryStats, name)
        return queryStats

    def dumpJson(self, data, name, saveDir=''):
        if not saveDir:
            saveDir = self.dumpDir
        if saveDir and not saveDir.endswith("/"):
            saveDir += "/"            
        if not name.endswith(".json"):
            name += ".json"
        with open(saveDir + name, "w") as f:
            json.dump(data, f, indent = 4)

    def dumpCex(self, cex, cexPrediction, prop, runName, model):
        if model is not None:
            modelPrediction = model.predict(np.array([cex])).argmax()
        else:
            modelPrediction = None
        mbouPrediction = cexPrediction.argmax()
        plt.figure()
        plt.title('CEX, yMax={}, ySecond={}, MbouPredicts={}, modelPredicts={}'.format(prop.yMax, prop.ySecond, mbouPrediction, modelPrediction))
        plt.imshow(np.squeeze(cex), cmap='Greys')
        plt.colorbar()
        plt.savefig(self.logDir + "Cex_{}".format(runName) + ".png")
        self.dumpNpArray(cex, "Cex_{}".format(runName), saveAtLog=True)
        
        diff = np.abs(cex - prop.xAdv)
        #assert np.max(diff) <= prop.inDist
        plt.figure()
        plt.title('Distance between pixels: CEX and adv. sample')
        plt.imshow(np.squeeze(diff), cmap='Greys')
        plt.colorbar()
        plt.savefig(self.logDir + "DiffCexXAdv_{}".format(runName) + ".png")
        self.dumpNpArray(diff, "DiffCexXAdv_{}".format(runName), saveAtLog=True)

    @staticmethod
    def dumpCoi(inputVarsMapping, runName):
        plt.title('COI_{}'.format(runName))
        plt.imshow(np.array([0 if i == -1 else 1 for i in np.nditer(inputVarsMapping.flatten())]).reshape(inputVarsMapping.shape[1:-1]), cmap='Greys')
        plt.savefig(self.logDir + 'COI_{}'.format(runName))

    def loadJson(self, name, loadDir=''):
        if not loadDir:
            loadDir = self.dumpDir
        if not name.endswith(".json"):
            name += ".json"
        if not loadDir.endswith("/"):
            loadDir += "/"
        with open(loadDir + name, "r") as f:
            return json.load(f)

    def loadNpArray(self, name, loadDir=''):
        if not loadDir:
            loadDir = self.dumpDir
        if not name.endswith(".npy"):
            name += ".npy"
        if not loadDir.endswith("/"):
            loadDir += "/"
        with open(loadDir + name, "rb") as f:
            return np.load(f, allow_pickle=True)

    def dumpResultsJson(self):
        self.dumpJson(self.resultsJson, CnnAbs.resultsFile, saveDir=self.logDir)
        brief = dict()
        brief["Result"] = self.resultsJson["Result"] if "Result" in self.resultsJson else None
        brief["totalRuntime"] = self.resultsJson["totalRuntime"] if "totalRuntime" in self.resultsJson else None        
        self.dumpJson(brief, CnnAbs.resultsFileBrief, saveDir=self.logDir)

    def setGtimeout(self, val):
        if val <= 0:
            self.gtimeout = 1 #Because 0 is used in timeout to signal no timeout.
        else:
            self.gtimeout = int(val)

    def decGtimeout(self, val):
        self.setGtimeout(self.gtimeout - val)

    def tickGtimeout(self):
        currentTime = time.time()
        self.decGtimeout(currentTime - self.prevTimeStamp)
        self.prevTimeStamp = currentTime
        if self.isGlobalTimedOut():
            self.resultsJson["Result"] = "GTIMEOUT"
            self.resultsJson["totalRuntime"] = time.time() - self.startTotal
            self.dumpResultsJson()
            return False
        return True

    def isGlobalTimedOut(self):
        return self.gtimeout <= 1

    def returnGtimeout(self):        
        return ResultObj("gtimeout").returnResult()

    def subResultAppend(self, runtime=None, runtimeTotal=None, originalQueryStats=None, finalQueryStats=None, sat=None, timedOut=None):
        self.resultsJson["subResults"].append({"outOf" : self.numSteps-1,
                                               "runtime" : runtime,
                                               "runtimeTotal":runtimeTotal,
                                               "originalQueryStats" : originalQueryStats,
                                               "finalQueryStats" : finalQueryStats,
                                               "SAT" : sat,
                                               "timedOut" : timedOut})
        self.dumpResultsJson()

    def subResultUpdate(self, runtime=None, runtimeTotal=None, originalQueryStats=None, finalQueryStats=None, sat=None, timedOut=None):
        self.resultsJson["subResults"][-1] = {"outOf" : self.numSteps-1,
                                              "runtime" : runtime,
                                              "runtimeTotal":runtimeTotal,
                                              "originalQueryStats" : originalQueryStats,
                                              "finalQueryStats" : finalQueryStats,
                                              "SAT" : sat,
                                              "timedOut" : timedOut}
            
        self.dumpResultsJson()
        

#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################
#################################################################################################################

def myLoss(labels, logits):
    return tf.keras.losses.sparse_categorical_crossentropy(labels, logits, from_logits=True)

class ModelUtils:
    
    numClones = 0

    def __init__(self, ds, optionsObj, logDir):
        self.ds = ds
        self.optionsObj = optionsObj
        self.logDir = logDir

    @staticmethod
    def outputModelPath(m, suffix=""):
        if suffix:
            suffix = "_" + suffix
        return "./{}{}.onnx".format(m.name, suffix)
        
    def tf2Model(self, model):
        modelOnnx = keras2onnx.convert_keras(model, model.name + "_onnx", debug_mode=0)
        modelOnnxName = ModelUtils.outputModelPath(model)
        keras2onnx.save_model(modelOnnx, self.logDir + modelOnnxName)
        return MarabouNetworkONNX.MarabouNetworkONNX(self.logDir + modelOnnxName)
                       
    def intermidModel(model, layerName):
        if layerName not in [l.name for l in model.layers]:
            layerName = list([l.name for l in reversed(model.layers) if l.name.startswith(layerName)])[0]
        return tf.keras.Model(inputs=model.input, outputs=model.get_layer(name=layerName).output)        

    def loadModel(self, path):
        model = load_model(os.path.abspath(path), custom_objects={'myLoss': myLoss})
        model.summary()
        score = model.evaluate(self.ds.x_test, self.ds.y_test, verbose=0)
        CnnAbs.printLog("(Original) Test loss:{}".format(score[0]))
        CnnAbs.printLog("(Original) Test accuracy:{}".format(score[1]))
        return model        
        

    @staticmethod
    def cexToImage(valDict, prop, inputVarsMapping=None, outputVarsMapping=None, useMapping=True, valueRange=None):
        if useMapping:
            lBounds = InputQueryUtils.getBoundsInftyBall(prop.xAdv, prop.inDist, valueRange=valueRange)[0]
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
    def isCEXSpurious(model, prop, cex, spuriousStrict=True, valueRange=None):
        yCorrect = prop.yMax
        yBad = prop.ySecond
        inBounds, violations =  InputQueryUtils.inBoundsInftyBall(prop.xAdv, prop.inDist, cex, valueRange=valueRange)
        if not inBounds:
            differences = cex - prop.xAdv
            if np.all(np.absolute(differences)[violations.nonzero()] <= np.full_like(differences[violations.nonzero()], 1e-10, dtype=np.double)):
                cex[violations.nonzero()] = prop.xAdv[violations.nonzero()]
        inBounds, violations =  InputQueryUtils.inBoundsInftyBall(prop.xAdv, prop.inDist, cex, valueRange=valueRange)                
        if not inBounds:
            raise Exception("CEX out of bounds, violations={}, values={}, cex={}, prop.xAdv={}".format(np.transpose(violations.nonzero()), np.absolute(cex-prop.xAdv)[violations.nonzero()], cex[violations.nonzero()], prop.xAdv[violations.nonzero()]))
        prediction = model.predict(np.array([cex]))
        #If I will require ySecond to be max, spurious definition will have to change to force it.
        if not spuriousStrict:
            return prediction.argmax() == yCorrect
        return prediction[0,yBad] + prop.outSlack < prediction[0,yCorrect]

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
        varsWithIngoingEdgesOrInputs = set([v.item() for nparr in net.inputVars for v in np.nditer(nparr, flags=["zerosize_ok"])])
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
    def divideToLayers(net):
        layerMappingReverse = dict()
        layer = set(net.outputVars.flatten().tolist())
        layerNum = 0
        lastMapped = 0
        nextLayerWas0 = False
        layerType = list()
        while len(layerMappingReverse) < net.numVars:
            nextLayer = set()
            for var in layer:
                assert var not in layerMappingReverse
                layerMappingReverse[var] = layerNum
            layerType.insert(0, set())
            for eq in net.equList:
                if (eq.addendList[-1][1] in layer) and (eq.EquationType == MarabouCore.Equation.EQ):
                    [nextLayer.add(el[1]) for el in eq.addendList[:-1] if el[0] != 0]
                    layerType[0].add("Linear")
            for maxArgs, maxOut in net.maxList:
                if maxOut in layer:
                    [nextLayer.add(arg) for arg in maxArgs]
                    layerType[0].add("Max")
            [(nextLayer.add(vin), layerType[0].add("Relu"))for vin,vout in net.reluList if vout in layer]
            [(nextLayer.add(vin), layerType[0].add("Abs")) for vin,vout in net.absList  if vout in layer]
            [(nextLayer.add(vin), layerType[0].add("Sign")) for vin,vout in net.signList if vout in layer]
            if not layerType[0]:
                layerType[0].add("Input")
            layer = nextLayer
            assert not nextLayerWas0
            nextLayerWas0 = len(nextLayer) == 0
            layerNum += 1
        layerMapping = dict()
        for v,l in layerMappingReverse.items():
            layerMapping[v] = layerNum - l - 1
        assert all([i in layerMapping for i in net.inputVars[0].flatten().tolist()])
        layerList = [set() for i in range(layerNum)]
        [layerList[l].add(var) for var,l in layerMapping.items()]
        return layerList, layerType

    @staticmethod        
    def removeVariables(net, varSet, keepSet=True, keepInputShape=False): # If keepSet then remove every variable not in keepSet. Else, remove variables in varSet.

        if not keepSet:        
            net.reluList = [(vin,vout) for vin,vout in net.reluList if (vin not in varSet) and (vout not in varSet)]
            net.absList  = [(vin,vout) for vin,vout in net.absList  if (vin not in varSet) and (vout not in varSet)]
            net.signList = [(vin,vout) for vin,vout in net.signList if (vin not in varSet) and (vout not in varSet)]
            varSet = {v for v in range(net.numVars) if v not in varSet}
    
        varSetList = list(varSet)
        varSetList.sort()
        varSetDict = {v:i for i,v in enumerate(varSetList)}
        assert set(varSet) == set(varSetDict.keys()) and len(set(varSet)) == len(varSet)
        tr = lambda v: varSetDict[v] if v in varSetDict else -1
        varsMapping = {tr(v) : v for v in range(net.numVars) if tr(v) != -1}
        
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
                if all([v != -1 for (c, v) in newEq.addendList]):
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
        if keepInputShape:
            assert all([v in varSet for inp in net.inputVars for v in inp.flatten().tolist()])
        else:
            net.inputVars  = [np.array([tr(v) for v in inp.flatten().tolist() if v in varSet]) for inp in net.inputVars]
        net.outputVars = np.array([tr(v) for v in net.outputVars.flatten().tolist() if v in varSet])
        net.numVars = len(varSetList)
        if inputVarsMapping is None or outputVarsMapping is None: #I made this change now to test inputVarsMapping==None
            raise Exception("None input/output varsMapping")
        return inputVarsMapping, outputVarsMapping, varsMapping

    @staticmethod
    def pruneUnreachableNeurons(net, init):

        origNumVars = net.numVars
    
        reach = set(init)
        lastLen = 0
        while len(reach) > lastLen:
            lastLen = len(reach)
            reachPrev = reach.copy()
            for eqi, eq in enumerate(net.equList):
                tw, tv = eq.addendList[-1]
                if (tv in reachPrev) and (tw == -1) and (eq.EquationType == MarabouCore.Equation.EQ):
                    [reach.add(v) for w,v in eq.addendList[:-1] if w != 0]
                elif (eq.EquationType != MarabouCore.Equation.EQ) or (eq.addendList[-1][0] != -1):
                    [reach.add(v) for w,v in eq.addendList]
            for maxArgs, maxOut in net.maxList:
                if maxOut in reachPrev:
                    [reach.add(arg) for arg in maxArgs]
            [reach.add(vin) for vin,vout in net.reluList if vout in reachPrev]
            [reach.add(vin) for vin,vout in net.absList  if vout in reachPrev]
            [reach.add(vin) for vin,vout in net.signList if vout in reachPrev]
            if len(net.disjunctionList) > 0:
                raise Exception("Not implemented")
        unreach = set([v for v in range(net.numVars) if v not in reach])
    
        inputVarsMapping, outputVarsMapping, varsMapping = InputQueryUtils.removeVariables(net, reach)
        for eq in net.equList:
            for w,v in eq.addendList:
                if v > net.numVars:
                    CnnAbs.printLog("eq.addendList={}, eq.scalar={}, eq.EquationType={}".format(eq.addendList, eq.scalar, eq.EquationType))
        CnnAbs.printLog("Number of vars in abstract network out of original network = {}".format(net.numVars / float(origNumVars)))
        return inputVarsMapping, outputVarsMapping, varsMapping

    @staticmethod
    def getBoundsInftyBall(x, r, floatingPointErrorGap=0, valueRange=None):
        assert valueRange is not None
        assert floatingPointErrorGap >= 0
        if valueRange is not None:
            l, u = np.maximum(x - r,np.full_like(x, valueRange[0])), np.minimum(x + r,np.full_like(x, valueRange[1]))
        else:
            l, u = x - r, x + r
        assert np.all(u - l > 2 * floatingPointErrorGap)
        u -= floatingPointErrorGap
        l += floatingPointErrorGap
        return l, u

    @staticmethod    
    def inBoundsInftyBall(x, r, p, valueRange=None):
        assert p.shape == x.shape
        l,u = InputQueryUtils.getBoundsInftyBall(x,r, valueRange=valueRange)
        geqLow = np.less_equal(l,p)
        leqUp  = np.less_equal(p,u)
        inBounds = np.logical_and(geqLow, leqUp)
        violations = np.logical_not(inBounds)
        assert violations.shape == x.shape
        return np.all(inBounds), violations

    @staticmethod
    def setAdversarial(net, x, inDist, outSlack, yCorrect, yBad, valueRange=None):
        inAsNP = np.array(net.inputVars[0])
        x = x.reshape(inAsNP.shape)
        xDown, xUp = InputQueryUtils.getBoundsInftyBall(x, inDist, floatingPointErrorGap=0.0025, valueRange=valueRange)
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
