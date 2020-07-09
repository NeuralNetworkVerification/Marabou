'''
DNC Example
====================

Top contributors (to current version):
  - Andrew Wu
  - Kyle Julian
  
This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.
'''

import sys
import numpy as np

# Append to path if needed
#sys.path.append(</Path/To/Marabou/>)

from maraboupy import Marabou

# %%
# Load an example network and place an output constraint
nnet_file_name = "../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"
net = Marabou.read_nnet(nnet_file_name)
net.setLowerBound(net.outputVars[0][0], .5)

# %%
# Solve the query with DNC mode turned on, which should return satisfying variable values
options = Marabou.createOptions(dnc=True, verbosity=0, initialDivides=2);
vals, stats = net.solve(options=options)
assert len(vals) > 0