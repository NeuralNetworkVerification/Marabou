'''
Disjunction Constraint Example
====================

Top contributors (to current version):
  - Haoze Andrew Wu

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
from maraboupy import MarabouCore


# %%
# Path to NNet file
nnetFile = "./resources/nnet/mnist/mnist10x10.nnet"

# %%
# Load the network from NNet file, and set a lower bound on first output variable
net1 = Marabou.read_nnet(nnetFile)

# %%
# Require that a input variable is either 0 or 1
for var in net1.inputVars[0]:
    # eq1: 1 * var = 0
    eq1 = MarabouCore.Equation(MarabouCore.Equation.EQ);
    eq1.addAddend(1, var);
    eq1.setScalar(0);

    # eq2: 1 * var = 1
    eq2 = MarabouCore.Equation(MarabouCore.Equation.EQ);
    eq2.addAddend(1, var);
    eq2.setScalar(1);

    # ( var = 0) \/ (var = 1)
    disjunction = [[eq1], [eq2]]
    net1.addDisjunctionConstraint(disjunction)

    # add additional bounds for the variable
    net1.setLowerBound(var, 0)
    net1.setUpperBound(var, 1)

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
# Test that the satisfying assignment found is a real one.
for i in range(784):
    assert(abs(vals1[i] - 1) < 0.0000001 or abs(vals1[i]) < 0.0000001)
