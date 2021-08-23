
import sys
import os
import tensorflow as tf
import numpy as np
import onnx
import onnxruntime
import argparse
import json

from maraboupy import MarabouNetworkONNX as monnx
from maraboupy import MarabouCore
from maraboupy import MarabouUtils
from maraboupy import Marabou

import matplotlib.pyplot as plt

parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--sample", type=int, default=0, help="Sample number")
parser.add_argument("--dist", type=float, default=0.03, help="Property distance")
parser.add_argument("--onnx", type=str, default='fullVanilla.onnx', help="Onnx net file")
parser.add_argument("--type", type=str, default='lp', choices=['lp', 'milp'], help="Gurbi solver type")
args = parser.parse_args()

(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
x_train, x_test = x_train / 255.0, x_test / 255.0
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)

xAdv = x_test[args.sample]
mbouNet  = monnx.MarabouNetworkONNX(args.onnx)
epsilon = args.dist
for i,x in enumerate(np.nditer(xAdv)):
    mbouNet.setUpperBound(i,max(x + epsilon, 0))    
    mbouNet.setLowerBound(i,max(x - epsilon, 0))

processInputQuery(modelOnnxMarabou)
options = Marabou.createOptions(verbosity=2, timeoutInSeconds=0, milpTightening=args.lpType, dumpBounds=True, tighteningStrategy='none')
ipq = MarabouCore.preprocess(net.getMarabouQuery(), options)
print(ipq.getNumberOfVariables())
if ipq.getNumberOfVariables() == 0:
    print("UNSAT on first LP bound tightening")
    exit()
else:
    newName = "dumpBounds_{}_{}.json".format(args.sample, args.dist.replace('.','-'))
    os.rename("dumpBounds.json", newName)
    print("dumped={}".format(newName))
