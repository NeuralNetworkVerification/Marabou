# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = FutureWarning)
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from maraboupy import Marabou
from maraboupy import MarabouCore
import numpy as np

# Global settings
OPT = MarabouCore.Options()        # Define options 
OPT._verbosity = 0                 # Turn off printing
TOL = 1e-5                         # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "resources/onnx/" # Folder for test networks

def test_fc1():
    """
    Test a simple fully-connected neural network
    Uses Gemm, ReLU, and Identity layers
    """
    # Load network
    filename = NETWORK_FOLDER + "fc1.onnx"
    network = Marabou.read_onnx(filename)

    # Evaluate example point using both Marabou and ONNX
    testInput = np.ones(network.inputVars[0].shape)
    marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "").flatten()
    onnxEval = network.evaluateWithoutMarabou([testInput]).flatten()

    # Assert that both evaluations are the same within the set tolerance
    assert max(abs(marabouEval.flatten() - onnxEval.flatten())) < TOL
