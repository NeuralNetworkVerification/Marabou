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
from tensorflow.keras.models import load_model
import matplotlib.pyplot as plt

tf.compat.v1.enable_v2_behavior()

optionsLocal   = Marabou.createOptions(snc=False, verbosity=2, solveWithMILP=False, timeoutInSeconds=100, milpTightening="lp", dumpBounds=True, tighteningStrategy="sbt")

def asONNX(model):
    modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=0)
    modelOnnxName = mnistProp.output_model_path(model)
    keras2onnx.save_model(modelOnnx, modelOnnxName)
    modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
    return modelOnnxMarabou
    

def trainedModel():
    inputShape = (5,5,1)
    
    modelOrig = tf.keras.Sequential(
        [
            tf.keras.Input(shape=inputShape),
            layers.Conv2D(1, kernel_size=(2, 2), activation="relu", name="c1"),
            layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
            layers.Flatten(name="f1"),
            layers.Dense(2, activation="relu", name="fc1"),
            layers.Dense(4, activation=None, name="sm1"),
        ],
        name="modelCOI"
    )
    modelOrig.build(input_shape=inputShape)
    modelOrig.compile(optimizer=mnistProp.optimizer, loss=myLoss, metrics=[tf.keras.metrics.SparseCategoricalAccuracy()])
    
    modelOrig.summary()
    #xAdv = np.random.random(inputShape) Shows some bug
    xAdv = np.array(range(25)).reshape(inputShape)
    
    return modelOrig, xAdv
    #modelOrigDense = cloneAndMaskConvModel(modelOrig, "mp1", np.ones((4,4)))

def handCraftedModel():    
    inputShape = (4,4,1)
    numInputs = np.prod(np.array(inputShape))
    numOutputs = 4
    model = MarabouNetwork()
    inputs = [model.getNewVariable() for i in range((numInputs))]
    outputs = np.array([model.getNewVariable() for i in range((numOutputs))])

    firstLayer = [model.getNewVariable() for i in range(9)]
    
    
    xAdv = np.random.random(inputShape)
    return model, xAdv

modelTF, xAdv = trainedModel()
mask = np.ones((4,4))
mask[tuple(np.transpose([(0,0),(0,1),(0,2),(0,3)]))] = 0
#mask[tuple(np.transpose([(0,0),(0,1),(1,0),(1,1)]))] = 0
print("mask={}".format(mask))
modelTFDense = cloneAndMaskConvModel(modelTF, "c1", mask,inputShape=(5,5,1), evaluate=False)
model      = asONNX(modelTF)
modelDense = asONNX(modelTFDense)
#model, xAdv = handCraftedModel()
#exit()

inDist = 0.05
yMax = 3
ySecond = 2
outSlack = 0

#boundDict = {v : (l,u) if model.}
#boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}

modelDense.saveQuery("COITestbench_Dense_Not_Proccessed")

setAdversarial(modelDense, xAdv, inDist, outSlack, yMax, ySecond)
inputVarsMapping, outputVarsMapping, varsMapping = setCOIBoundes(modelDense, modelDense.outputVars.flatten().tolist())
#setBounds(model, boundDict)
#setUnconnectedAsInputs(modelDense)

modelDense.saveQuery("COITestbench_Dense_Proccessed")

ipq = modelDense.getMarabouQuery()
vals, stats = Marabou.solve_query(ipq, verbose=False, options=optionsLocal)
sat = len(vals) > 0
timedOut = stats.hasTimedOut()
unsat = not timedOut and not sat
print("SAT = {}".format(sat))
print("UNSAT = {}".format(unsat))
print("TIMEDOUT = {}".format(timedOut))
