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
NUM_SAMPLES = 5

def test_incremental():
    """
    Test that the result of incremental solving is the same as non-incremental solving
    """
    filename = os.path.join(os.path.dirname(__file__), ONNX_FILE)


    random_images = [np.random.random((784)) for _ in range(NUM_SAMPLES)]

    result_noninc = []
    for img in random_images:
        network = Marabou.read_onnx(filename)
        print(network.inputVars[0])
        for i, x in enumerate(np.array(network.inputVars[0][0]).flatten()):
            network.setLowerBound(x, max(0, img[i] - EPSILON))
            network.setUpperBound(x, min(1, img[i] + EPSILON))
        outputVars = network.outputVars.flatten()
        network.addInequality([outputVars[0],
                               outputVars[1]],
                              [1, -1], 0)
        res, _, _ = network.solve(timeoutInSeconds=10)
        result_noninc.append(res)
        print(res)

test_incremental()
