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
inputShape = (10,10,1)

modelOrig = tf.keras.Sequential(
    [
        tf.keras.Input(shape=inputShape),
        layers.Conv2D(1, kernel_size=(3, 3), activation="relu", name="c1"),
        layers.MaxPooling2D(pool_size=(2, 2), name="mp1"),
        layers.Flatten(name="f1"),
        layers.Dense(4, activation="relu", name="fc1"),
        layers.Dense(mnistProp.num_classes, activation=None, name="sm1"),
    ],
    name="modelCOI"
)

modelOrig.summary()
#modelOrigDense = cloneAndMaskConvModel(modelOrig, "mp1", np.ones((4,4)))        
#boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}

model = modelOrig

xAdv = np.random.random(inputShape)
inDist = 0.05
yMax = 3
ySecond = 2
outSlack = 0

modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=0)
modelOnnxName = mnistProp.output_model_path(model)
keras2onnx.save_model(modelOnnx, modelOnnxName)
modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
setAdversarial(modelOnnxMarabou, xAdv, inDist, outSlack, yMax, ySecond)
#setBounds(modelOnnxMarabou, boundDict)
setUnconnectedAsInputs(modelOnnxMarabou)
modelOnnxMarabou.saveQuery("COITestbench")
ipq = modelOnnxMarabou.getMarabouQuery()
vals, stats = Marabou.solve_query(ipq, verbose=False, options=optionsLocal)
sat = len(vals) > 0
timedOut = stats.hasTimedOut()
unsat = not timedOut and not sat
print("SAT = {}".format(sat))
print("UNSAT = {}".format(unsat))
print("TIMEDOUT = {}".format(timedOut))
