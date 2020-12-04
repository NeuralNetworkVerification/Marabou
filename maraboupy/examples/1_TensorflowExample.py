'''
Tensorflow Example
====================

Top contributors (to current version):
  - Christopher Lazarus
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
# This network has inputs x0, x1, and was trained to create outputs that approximate
# y0 = abs(x0) + abs(x1), y1 = x0^2 + x1^2
filename = "../../resources/tf/frozen_graph/fc1.pb"
network = Marabou.read_tf(filename)

# %%
# Or, you can specify the operation names of the input and output operations.
# The default chooses the placeholder operations as input and the last operation as output
inputNames = ['Placeholder']
outputName = 'y_out'
network = Marabou.read_tf(filename = filename, inputNames = inputNames, outputName = outputName)

# %%
# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0][0]
outputVars = network.outputVars[0]

# %%
# Set input bounds on both input variables
network.setLowerBound(inputVars[0],-10.0)
network.setUpperBound(inputVars[0], 10.0)
network.setLowerBound(inputVars[1],-10.0)
network.setUpperBound(inputVars[1], 10.0)

# %%
# Set output bounds on the second output variable
network.setLowerBound(outputVars[1], 194.0)
network.setUpperBound(outputVars[1], 210.0)

# %%
# Call to C++ Marabou solver
vals, stats = network.solve("marabou.log")