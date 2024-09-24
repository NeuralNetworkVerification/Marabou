"""
Pythonic API
====================

Top contributors (to current version):
  - Min Wu

This file is part of the Marabou project.
Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.
"""

import os
from maraboupy import Marabou
from maraboupy.MarabouPythonic import *

"""
A realistic example on the MNIST dataset to set input/output constraints.
the 1st part shows how to use the new Pythonic API to set constraints; 
The 2nd part shows how to set the same constraints using the base method.

"""

# %%
# use the same mnist model in onnx format as an example
NETWORK_FOLDER = "../../resources/onnx/"
filename = "cnn_max_mninst2.onnx"
filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)

# %%
# the 1st part shows the new Pythonic API to add constraints
network_a = Marabou.read_onnx(filename)
invars = network_a.inputVars[0][0].flatten()
# set constraints on the input variables
for i in invars:
    v = Var(i)
    network_a.addConstraint(v <= 7.5)
    network_a.addConstraint(v >= 4)
outvars = network_a.outputVars[0].flatten()
u = Var(outvars[-1])
# set constraints on the output variables
for j in outvars[:-1]:
    v = Var(j)
    network_a.addConstraint(v <= 30)
    network_a.addConstraint(v >= 2)
    network_a.addConstraint(v <= u)
    network_a.addConstraint(v + u >= 20)
    network_a.addConstraint(7 * v - 11 * u <= 4)

# %%
# the 2nd part shows the base method to add the same constraints
network_b = Marabou.read_onnx(filename)
invars = network_a.inputVars[0][0].flatten()
# set the same constraints on the input variables
for i in invars:
    network_b.setUpperBound(i, 7.5)
    network_b.setLowerBound(i, 4)
outvars = network_a.outputVars[0].flatten()
u = Var(outvars[-1])
# set the same constraints on the output variables
for j in outvars[:-1]:
    network_b.setUpperBound(j, 30)
    network_b.setLowerBound(j, 2)
    network_b.addInequality([j, outvars[-1]], [1, -1], 0)
    network_b.addInequality([j, outvars[-1]], [-1, -1], -20)
    network_b.addInequality([j, outvars[-1]], [7, -11], 4)
