'''
NNet Example
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

import sys
import numpy as np

## %
# Path to Marabou folder if you did not export it

# sys.path.append('/home/USER/git/Marabou')
from maraboupy import Marabou


# %%
# Path to NNet file
nnetFile = "../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"

# %%
# Load the network from NNet file, and set a lower bound on first output variable
net1 = Marabou.read_nnet(nnetFile)
net1.setLowerBound(net1.outputVars[0][0], .5)

# %%
# Solve Marabou query
vals1, stats1 = net1.solve()


# %%
# Example statistics
stats1.getNumSplits()
stats1.getTotalTime()


# %%
# Eval example
#
# Test that when the upper/lower bounds of input variables are fixed at the
# same value, with no other input/output constraints, Marabou returns the 
# outputs found by evaluating the network at the input point.
inputs = np.array([-0.328422874212265,
                    0.40932923555374146,
                   -0.017379289492964745,
                   -0.2747684121131897,
                   -0.30628132820129395])

outputsExpected = np.array([0.49999678, -0.18876659,  0.80778555, -2.76422264, -0.12984317])

net2 = Marabou.read_nnet(nnetFile)
outputsMarabou = net2.evaluateWithMarabou([inputs])
assert max(abs(outputsMarabou.flatten() - outputsExpected)) < 1e-8