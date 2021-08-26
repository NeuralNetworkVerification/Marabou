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
import scipy.sparse as sp

import gurobipy as gp
from gurobipy import GRB
#
#model = gp.Model("signTest")
#x = model.addMVar(shape=1, vtype=GRB.CONTINUOUS, name="x")
#obj = np.array([1.0])
#model.setObjective(obj @ x, GRB.MAXIMIZE)
#
#val = np.array([1.0, -1.0])
#row = np.array([0, 1])
#col = np.array([0, 0])
#A = sp.csr_matrix((val, (row, col)), shape=(2,1))
#print(A.todense())
#B = np.array([3., -1.])
#model.addConstr(A @ x <= B, name="c")
#
#model.optimize()
#print(x.X)
#print('Obj: %g' % model.objVal)
#
#obj = np.array([1.0])
#model.setObjective(obj @ x, GRB.MINIMIZE)
#model.optimize()
#
#print(x.X)
#print('Obj: %g' % model.objVal)
#

model = gp.Model("signTest")
var  = model.addMVar(shape=12, vtype=GRB.CONTINUOUS, name="var")
obj = np.array([0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 1.0, -1.0])

#val = np.array([1.0, -1.0])
#row = np.array([0, 1])
#col = np.array([0, 0])
#A = sp.csr_matrix((val, (row, col)), shape=(2,1))
#print(A.todense())

# x <= 1
# x >= -1
# f1 <= 1
# f1 >= -1
# f2 <= 1
# f2 >= -1
# b1 <= 3x+1
# b1 >= 3x+1
# b2 <= -4x+2
# b2 >= -4x+2
# y <= f1+f2
# y >= f1+f2
# f1 <= b1+1
# f1 >= 0.5b1-1
# f2 <= b2+1
# f2 >= (1/3)b2-1

#[ 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],

a = np.array([[ 1.,-1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
              [-1., 1., 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.],
              [ 0., 0., 0., 0., 0., 0., 1.,-1., 0., 0., 0., 0.],
              [ 0., 0., 0., 0., 0., 0.,-1., 1., 0., 0., 0., 0.],
              [ 0., 0., 0., 0., 0., 0., 0., 0., 1.,-1., 0., 0.],
              [ 0., 0., 0., 0., 0., 0., 0., 0.,-1., 1., 0., 0.],               
              [-3., 3., 1.,-1., 0., 0., 0., 0., 0., 0., 0., 0.],
              [ 3.,-3.,-1., 1., 0., 0., 0., 0., 0., 0., 0., 0.],
              [ 4.,-4., 0., 0., 1.,-1., 0., 0., 0., 0., 0., 0.],
              [-4., 4., 0., 0.,-1., 1., 0., 0., 0., 0., 0., 0.],
              [ 0., 0., 0., 0., 0., 0.,-1., 1.,-1., 1., 1.,-1.],
              [ 0., 0., 0., 0., 0., 0., 1.,-1., 1.,-1.,-1., 1.],
              [ 0., 0.,-1., 1. , 0., 0. , 1.,-1., 0., 0., 0., 0.],
              [ 0., 0.,0.5,-0.5, 0., 0. ,-1., 1., 0., 0., 0., 0.],              
              [ 0., 0., 0., 0. ,-1., 1. , 0., 0., 1.,-1., 0., 0.],
              [ 0., 0., 0., 0. ,0.33333,-0.33333, 0., 0.,-1., 1., 0., 0.]
])
A = sp.csr_matrix(a)
print(A.shape)
B = np.array([1., 1., 1., 1., 1., 1., 1., -1., 2., -2., 0., 0., 1., 1., 1., 1.])
print(B.shape)

model.addConstr(A @ var <= B, name="c")
model.setObjective(obj @ var, GRB.MAXIMIZE)
model.optimize()
print(var.X)
print('Obj: %g' % model.objVal)

model.setObjective(obj @ var, GRB.MINIMIZE)
model.optimize()
print(var.X)
print('Obj: %g' % model.objVal)
