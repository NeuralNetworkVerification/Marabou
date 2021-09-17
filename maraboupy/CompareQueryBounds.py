import numpy as np
from CnnAbs_WIP import *
#from cnn_abs import *

import argparse
import json

#from itertools import product, chain
from maraboupy import MarabouNetworkONNX as monnx
import matplotlib.pyplot as plt

###########################################################################
####  _____              __ _                   _   _                  ####
#### /  __ \            / _(_)                 | | (_)                 ####
#### | /  \/ ___  _ __ | |_ _  __ _ _   _  __ _| |_ _  ___  _ __  ___  ####
#### | |    / _ \| '_ \|  _| |/ _` | | | |/ _` | __| |/ _ \| '_ \/ __| ####
#### | \__/\ (_) | | | | | | | (_| | |_| | (_| | |_| | (_) | | | \__ \ ####
####  \____/\___/|_| |_|_| |_|\__, |\__,_|\__,_|\__|_|\___/|_| |_|___/ ####
####                           __/ |                                   ####
####                          |___/                                    ####
###########################################################################

tf.compat.v1.enable_v2_behavior()

parser = argparse.ArgumentParser(description='Run MNIST based verification scheme using abstraction')
parser.add_argument("--q1",       type=str,  default="", help="firstQuery")
parser.add_argument("--q2",       type=str,  default="", help="firstQuery")

args = parser.parse_args()

cfg_q1 = args.q1
cfg_q2 = args.q2

ipq1 = Marabou.load_query("/cs/labs/guykatz/matanos/Marabou/maraboupy/" + cfg_q1)
ipq2 = Marabou.load_query("/cs/labs/guykatz/matanos/Marabou/maraboupy/" + cfg_q2)

numVar1 = ipq1.getNumberOfVariables()
numVar2 = ipq2.getNumberOfVariables()

assert numVar1 == numVar2

bounds1 = {v : (ipq1.getLowerBound(v), ipq1.getUpperBound(v)) for v in range(numVar1)}
bounds2 = {v : (ipq2.getLowerBound(v), ipq2.getUpperBound(v)) for v in range(numVar2)}

identical = all([bounds1[v] == bounds2[v] for v in range(numVar1)])
subsetList = lambda boundsSubset, boundsSuperset, numVar: [(boundsSuperset[v][0] <= boundsSubset[v][0]) and (boundsSubset[v][1] <= boundsSuperset[v][1]) for v in range(numVar)]
subset12 = all(subsetList(bounds1, bounds2, numVar1))
subset21 = all(subsetList(bounds2, bounds1, numVar1))

constantValue1 = sum([l == u for l,u in bounds1.values()])
constantValue2 = sum([l == u for l,u in bounds2.values()])

subset12Ratio = sum(subsetList(bounds1, bounds2, numVar1)) / numVar1
subset21Ratio = sum(subsetList(bounds2, bounds1, numVar1)) / numVar1

subset12Sum = sum(subsetList(bounds1, bounds2, numVar1))
subset21Sum = sum(subsetList(bounds2, bounds1, numVar1))

print('numVar1/2={}'.format(numVar1))
print('constantValue1={}'.format(constantValue1))
print('constantValue2={}'.format(constantValue2))
print('identical={}'.format(identical))
print('subset12={}'.format(subset12))
print('subset21={}'.format(subset21))
print('subset12Ratio={}'.format(subset12Ratio))
print('subset21Ratio={}'.format(subset21Ratio))
print('subset12Sum={}'.format(subset12Sum))
print('subset21Sum={}'.format(subset21Sum))
