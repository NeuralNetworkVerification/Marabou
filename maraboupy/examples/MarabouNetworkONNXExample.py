'''
/* *******************                                                        */
/*! \file MarabouNetworkTFExample.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/
'''

from maraboupy import Marabou
from maraboupy import MarabouCore
import numpy as np

# Set the Marabou option to restrict printing
options = MarabouCore.Options()
options._verbosity = 0

### FULLY CONNECTED NETWORK EXAMPLE ###
# Network corresponds to inputs x0, x1
# Outputs: y0 = |x0| + |x1|, y1 = x0^2 + x1^2
print("Fully Connected Network Example")
filename = './networks/graph_test_medium.onnx'
network = Marabou.read_onnx(filename)

## Or, you can specify the operation names of the input and output operations
## By default chooses the only placeholder as input, last op as output
#inputName = 'Placeholder:0'
#outputName = 'y_out:0'
#network = Marabou.read_onnx(filename=filename, inputNames=[inputName], outputName = outputName)

# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0][0]
outputVars = network.outputVars

# Set input bounds
network.setLowerBound(inputVars[0],-10.0)
network.setUpperBound(inputVars[0], 10.0)
network.setLowerBound(inputVars[1],-10.0)
network.setUpperBound(inputVars[1], 10.0)

# Set output bounds
network.setLowerBound(outputVars[1], 194.0)
network.setUpperBound(outputVars[1], 210.0)

# Call to Marabou solver
vals, stats = network.solve(options = options)


### CONVOLUTIONAL NETWORK EXAMPLE ###
# Network maps 8x16 grayscale images two values
print("\nConvolutional Network Example")
filename = './networks/AutoTaxi.onnx'
network = Marabou.read_onnx(filename)

# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0][0]
outputVars = network.outputVars[0]

delta = 0.03
for h in range(inputVars.shape[0]):
    for w in range(inputVars.shape[1]):
        network.setLowerBound(inputVars[h][w][0], 0.5-delta)
        network.setUpperBound(inputVars[h][w][0], 0.5+delta)

# Set output bounds
network.setLowerBound(outputVars[0], 6.0)

# Call to Marabou solver (should be SAT)
print("Check query with less restrictive output constraint (Should be SAT)")
vals, stats = network.solve(options = options)


# Set more restrictive output bounds
network.setLowerBound(outputVars[0], 7.0)

# Call to Marabou solver (should be UNSAT)
print("Check query with more restrictive output constraint (Should be UNSAT)")
vals, stats = network.solve(options = options)


### CONVOLUTIONAL NETWORK WITH MAX-POOL EXAMPLE ###
print("\nConvolutional Network with Max Pool Example")
filename = './networks/graph_test_cnn.onnx'
network = Marabou.read_onnx(filename)

# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0]
outputVars = network.outputVars

marabouEval = network.evaluateWithMarabou([np.ones(inputVars.shape)], options = options)
onnxEval = network.evaluateWithoutMarabou([np.ones(inputVars.shape)])

print("Marabou Evaluation:")
print(marabouEval)
print("\nONNX Evaluation:")
print(onnxEval)
print("\nDifference:")
print(onnxEval - marabouEval)