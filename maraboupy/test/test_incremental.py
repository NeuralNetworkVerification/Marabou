# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from subprocess import call
from .. import Marabou
from .. import MarabouCore
import numpy as np
import os

# Global settings
ONNX_FILE = "../../resources/onnx/mnist2x10.onnx" # tiny mnist classifier
EPSILON = 0.02
LABEL = 0
NUM_SAMPLES = 5
OPT = Marabou.createOptions(timeoutInSeconds=10,verbosity=0)
SEED = 50

np.random.seed(SEED)

def test_incremental():
    """
    Test that the result of incremental solving is the same as non-incremental solving
    """
    filename = os.path.join(os.path.dirname(__file__), ONNX_FILE)

    random_images = [np.random.random((784,1)) for _ in range(NUM_SAMPLES)]

    # First check robustness with non-incremental mode.
    result_noninc = []
    for img in random_images:
        network = Marabou.read_onnx(filename)
        for i, x in enumerate(np.array(network.inputVars[0]).flatten()):
            network.setLowerBound(x, max(0, img[i] - EPSILON))
            network.setUpperBound(x, min(1, img[i] + EPSILON))
            outputVars = network.outputVars[0].flatten()
        for outputIndex in range(len(outputVars)):
            if outputIndex != LABEL:
                network.addInequality([outputVars[outputIndex],
                                       outputVars[LABEL]],
                                      [1, -1], 0)

        res, _, _ = network.solve(verbose=False, options=OPT)
        result_noninc.append(res)

    # Now check robustness with incremental mode.
    result_inc = []
    network = Marabou.read_onnx(filename)
    for img in random_images:
        network.clearProperty()
        for i, x in enumerate(np.array(network.inputVars[0]).flatten()):
            network.setLowerBound(x, max(0, img[i] - EPSILON))
            network.setUpperBound(x, min(1, img[i] + EPSILON))
            outputVars = network.outputVars[0].flatten()
        for outputIndex in range(len(outputVars)):
            if outputIndex != LABEL:
                network.addInequality([outputVars[outputIndex],
                                       outputVars[LABEL]],
                                      [1, -1], 0, isProperty=True)

        res, _, _ = network.solve(verbose=False, options=OPT)
        result_inc.append(res)

    for i in range(len(result_inc)):
        assert(result_noninc[i] == result_inc[i])
