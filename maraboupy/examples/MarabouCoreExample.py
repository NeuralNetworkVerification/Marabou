'''
/* *******************                                                        */
/*! \file MarabouCoreExample.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Kyle Julian, Andrew Wu
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

from maraboupy import MarabouCore
from maraboupy.Marabou import createOptions

# The example from the CAV'17 paper:
#   0   <= x0  <= 1
#   0   <= x1f
#   0   <= x2f
#   1/2 <= x3  <= 1
#
#  x0 - x1b = 0
#  x0 + x2b = 0
#  x1f + x2f - x3 = 0
#
#  x1f = Relu(x1b)
#  x2f = Relu(x2b)
#
#   x0: x0
#   x1: x1b
#   x2: x1f
#   x3: x2b
#   x4: x2f
#   x5: x3

large = 10.0

inputQuery = MarabouCore.InputQuery()

inputQuery.setNumberOfVariables(6)

inputQuery.setLowerBound(0, 0)
inputQuery.setUpperBound(0, 1)

inputQuery.setLowerBound(1, -large)
inputQuery.setUpperBound(1, large)

inputQuery.setLowerBound(2, 0)
inputQuery.setUpperBound(2, large)

inputQuery.setLowerBound(3, -large)
inputQuery.setUpperBound(3, large)

inputQuery.setLowerBound(4, 0)
inputQuery.setUpperBound(4, large)

inputQuery.setLowerBound(5, 0.5)
inputQuery.setUpperBound(5, 1)

equation1 = MarabouCore.Equation()
equation1.addAddend(1, 0)
equation1.addAddend(-1, 1)
equation1.setScalar(0)
inputQuery.addEquation(equation1)

equation2 = MarabouCore.Equation()
equation2.addAddend(1, 0)
equation2.addAddend(1, 3)
equation2.setScalar(0)
inputQuery.addEquation(equation2)

equation3 = MarabouCore.Equation()
equation3.addAddend(1, 2)
equation3.addAddend(1, 4)
equation3.addAddend(-1, 5)
equation3.setScalar(0)
inputQuery.addEquation(equation3)

MarabouCore.addReluConstraint(inputQuery, 1, 2)
MarabouCore.addReluConstraint(inputQuery, 3, 4)

options = createOptions()
vars1, stats1 = MarabouCore.solve(inputQuery, options, "")
if len(vars1) > 0:
    print("sat")
    print(vars1)
else:
    print("unsat")
