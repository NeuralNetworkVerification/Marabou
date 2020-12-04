'''
ONNX Example
====================

Top contributors (to current version):
  - Kyle Julian
  
This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.
'''

from maraboupy import Marabou
import numpy as np

# %%
# Set the Marabou option to restrict printing
options = Marabou.createOptions(verbosity = 0)

# %%
# Fully-connected network example
# -------------------------------
#
# This network has inputs x0, x1, and was trained to create outputs that approximate
# y0 = abs(x0) + abs(x1), y1 = x0^2 + x1^2
print("Fully Connected Network Example")
filename = "../../resources/onnx/fc1.onnx"
network = Marabou.read_onnx(filename)

# %%
# Or, you can specify the operation names of the input and output operations.
# The default chooses the placeholder operations as inputs and the last operation as output
inputName = 'Placeholder:0'
outputName = 'y_out:0'
network = Marabou.read_onnx(filename=filename, inputNames=[inputName], outputName = outputName)

# %%
# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0][0]
outputVars = network.outputVars[0]

# %%
# Set input bounds
network.setLowerBound(inputVars[0],-10.0)
network.setUpperBound(inputVars[0], 10.0)
network.setLowerBound(inputVars[1],-10.0)
network.setUpperBound(inputVars[1], 10.0)

# %%
# Set output bounds
network.setLowerBound(outputVars[1], 194.0)
network.setUpperBound(outputVars[1], 210.0)

# %%
# Call to Marabou solver
vals, stats = network.solve(options = options)


# %%
# Convolutional neural network example
# ------------------------------------
#
# Network maps 8x16 grayscale images two values
print("\nConvolutional Network Example")
filename = '../../resources/onnx/KJ_TinyTaxiNet.onnx'
network = Marabou.read_onnx(filename)

# %%
# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0][0]
outputVars = network.outputVars[0]

# %%
# Setup a local robustness query
delta = 0.03
for h in range(inputVars.shape[0]):
    for w in range(inputVars.shape[1]):
        network.setLowerBound(inputVars[h][w][0], 0.5-delta)
        network.setUpperBound(inputVars[h][w][0], 0.5+delta)

# %%
# Set output bounds
network.setLowerBound(outputVars[0], 6.0)

# %%
# Call to Marabou solver (should be SAT)
print("Check query with less restrictive output constraint (Should be SAT)")
vals, stats = network.solve(options = options)
assert len(vals) > 0

# %%
# Set more restrictive output bounds
network.setLowerBound(outputVars[0], 7.0)

# %%
# Call to Marabou solver (should be UNSAT)
print("Check query with more restrictive output constraint (Should be UNSAT)")
vals, stats = network.solve(options = options)
assert len(vals) == 0


# %%
# Convolutional network with max-pool example
# -------------------------------------------
print("\nConvolutional Network with Max Pool Example")
filename = '../../resources/onnx/conv_mp1.onnx'
network = Marabou.read_onnx(filename)

# %%
# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0]
outputVars = network.outputVars

# %% 
# Test Marabou equations against onnxruntime at an example input point
inputPoint = np.ones(inputVars.shape)
marabouEval = network.evaluateWithMarabou([inputPoint], options = options)
onnxEval = network.evaluateWithoutMarabou([inputPoint])

# %%
# The two evaluations should produce the same result
print("Marabou Evaluation:")
print(marabouEval)
print("\nONNX Evaluation:")
print(onnxEval)
print("\nDifference:")
print(onnxEval - marabouEval)
assert max(abs(onnxEval - marabouEval)) < 1e-6