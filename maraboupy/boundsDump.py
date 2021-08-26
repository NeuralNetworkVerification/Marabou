import random
import sys
import os
import tensorflow as tf
import numpy as np
import onnx
import onnxruntime
import argparse
import json

from maraboupy import MarabouNetworkONNX as monnx
from maraboupy import MarabouNetwork as mnet
from maraboupy import MarabouCore
from maraboupy import MarabouUtils
from maraboupy import Marabou

import matplotlib.pyplot as plt

import gurobipy

parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--sample", type=int, default=0, help="Sample number")
parser.add_argument("--dist", type=float, default=0.03, help="Property distance")
parser.add_argument("--onnx", type=str, default='fullVanilla.onnx', help="Onnx net file")
parser.add_argument("--gurobi", type=str, default='lp', choices=['lp', 'milp'], help="Gurbi solver type")
parser.add_argument("--flag", type=str, default='', help="Addition to the dump file's name")
args = parser.parse_args()

(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
x_train, x_test = x_train / 255.0, x_test / 255.0
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)

xAdv = x_test[args.sample]        

options = Marabou.createOptions(verbosity=2, timeoutInSeconds=0, milpTightening='lp', dumpBounds=True, tighteningStrategy='none', milpSolverTimeout=100)

print("\n\n\n**************** Max 1 ****************")
mbouNet = mnet.MarabouNetwork()
mbouNet.numVars = 7
[mbouNet.setLowerBound(i, -1.) for i in range(1)]
[mbouNet.setUpperBound(i, 1.) for i in range(1)]
mbouNet.setLowerBound(5, -0.5)
mbouNet.setUpperBound(5, 0.5)
mbouNet.inputVars = [np.array([0,5])]
mbouNet.addEquality([0, 1], [1, -1], -0.5)
mbouNet.addEquality([0, 2], [3, -1], -0.5)    
mbouNet.addRelu(1,3)
mbouNet.addRelu(2,4)
mbouNet.addMaxConstraint({3,4,5}, 6)
mbouNet.outputVars = np.array([6])
ipq = MarabouCore.preprocess(mbouNet.getMarabouQuery(), options)
if ipq.getNumberOfVariables() == 0:
    #print("UNSAT on first LP bound tightening")
    pass
else:
    maxUB = max([ipq.getUpperBound(j) for j in [3, 4, 5]])
    if round(maxUB,4) != round(ipq.getUpperBound(6),4):
        print("maxUB = {}".format(maxUB))
        print("ipq.getUpperBound(6) = {}".format(ipq.getUpperBound(6)))        


print("\n\n\n**************** Sign ****************")
mbouNet = mnet.MarabouNetwork()
mbouNet.numVars = 6
mbouNet.setLowerBound(0, -1)
mbouNet.setUpperBound(0, 1)
mbouNet.inputVars = [np.array([0])]
mbouNet.addEquality([0, 1], [-3, 1], 1)
mbouNet.addEquality([0, 2], [4, 1], 2)
mbouNet.addSignConstraint(1,3)
mbouNet.addSignConstraint(2,4)
mbouNet.addEquality([3, 4, 5], [-1, -1, 1], 0)
mbouNet.outputVars = np.array([5])
ipq = MarabouCore.preprocess(mbouNet.getMarabouQuery(), options)
if ipq.getNumberOfVariables() == 0:
    pass
else:
    maxUB = max([ipq.getUpperBound(j) for j in [3, 4, 5]])
    if round(maxUB,4) != round(ipq.getUpperBound(5),4):
        print("maxUB = {}".format(maxUB))
        print("ipq.getUpperBound(5) = {}".format(ipq.getUpperBound(5)))

        
print("\n\n\n**************** Max 2 ****************")
mbouNet = mnet.MarabouNetwork()
mbouNet.numVars = 10
mbouNet.setLowerBound(0, -1.)
mbouNet.setUpperBound(0,  1.)
mbouNet.setLowerBound(1, -1.)
mbouNet.setUpperBound(1,  1.)
mbouNet.setLowerBound(2, -2.)
mbouNet.setUpperBound(2,  2.)
mbouNet.setLowerBound(3, -2.)
mbouNet.setUpperBound(3,  2.)
w1 = 1.
w2 = -2. #-1.
b = 0.
mbouNet.inputVars = [np.array(list(range(4)))]
mbouNet.addEquality([0, 1, 4], [-w1, -w2, 1], b)
mbouNet.addEquality([1, 2, 5], [-w1, -w2, 1], b)
mbouNet.addEquality([2, 3, 6], [-w1, -w2, 1], b)
mbouNet.addMaxConstraint({4,5}, 7)
mbouNet.addMaxConstraint({5,6}, 8)
mbouNet.addEquality([7, 8, 9], [-1, -1, 1], 0)
mbouNet.outputVars = np.array([9])
ipq = MarabouCore.preprocess(mbouNet.getMarabouQuery(), options)
if ipq.getNumberOfVariables() == 0:
    print("UNSAT on first LP bound tightening")
else:
    pass
#    maxUB = max([ipq.getUpperBound(j) for j in [0,1,2]])
#    if round(maxUB,4) != round(ipq.getUpperBound(5),4):
#        print("maxUB = {}".format(maxUB))
#        print("ipq.getUpperBound(5) = {}".format(ipq.getUpperBound(5)))

