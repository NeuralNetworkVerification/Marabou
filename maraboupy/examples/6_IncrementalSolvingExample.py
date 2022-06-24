'''
Incremental solving Example
====================

Top contributors (to current version):
  - Haoze Andrew Wu

This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.
'''


# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

from maraboupy import Marabou
from maraboupy import MarabouCore
import numpy as np
import os

"""
A common use scenario is to evaluate multiple properties on the same network.
In that case, reloading networks repeatedly might be costly.
Instead, we could add the constraints to a separate list, which can be cleared
after invoking the solve() method.
"""

# Global settings
ONNX_FILE = "../../resources/onnx/mnist2x10.onnx" # tiny mnist classifier
EPSILON = 0.04
NUM_SAMPLES = 5
OPT = Marabou.createOptions(timeoutInSeconds=10,verbosity=0)
SEED = 50

np.random.seed(SEED)

filename = os.path.join(os.path.dirname(__file__), ONNX_FILE)

# The set of images to check local robustness against
random_images = [np.random.random((784,1)) for _ in range(NUM_SAMPLES)]

# Step 1: load the network
network = Marabou.read_onnx(filename)
# Step 2: iterate over image
for img in random_images:
    correctLabel = np.argmax(network.evaluateWithoutMarabou([img]))
    # Step 3: iterate over each adversarial label and check robustness
    outputVars = network.outputVars[0].flatten()
    for i, _ in enumerate(outputVars):
        if i == correctLabel:
            continue
        # Step 3.1: clear the user defined constraints in the last round.
        network.clearProperty()
        # Step 3.2: add new constraints
        for i, x in enumerate(np.array(network.inputVars[0]).flatten()):
            network.setLowerBound(x, max(0, img[i] - EPSILON))
            network.setUpperBound(x, min(1, img[i] + EPSILON))

        # y_correct - y_i <= 0
        network.addInequality([outputVars[correctLabel],
                               outputVars[i]],
                              [1, -1], 0,
                              addToAdditionalEquList=True)
        res, _, _ = network.solve(verbose=False, options=OPT)

        print(res)
