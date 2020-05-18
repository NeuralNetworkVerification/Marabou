# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = FutureWarning)
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from .. import Marabou
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-4                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/onnx/"   # Folder for test networks
np.random.seed(123)                        # Seed random numbers for repeatability
NUM_RAND = 20                              # Number of random test points per example

def test_fc1():
    """
    Test a fully-connected neural network, exported from tensorflow
    Uses Gemm, Relu, and Identity layers
    """
    filename =  "fc1.onnx"
    evaluateFile(filename)

def test_fc2():
    """
    Test a fully-connected neural network, exported from pytorch
    Uses Flatten and Gemm layers
    """
    filename =  "fc2.onnx"
    evaluateFile(filename)
    
def test_KJ_TaxiNet():
    """
    Test a convolutional network, exported from tensorflow
    Uses Transpose, Conv, Add, Relu, Cast, Reshape, 
    Matmul, and Identity layers
    """
    filename =  "KJ_TinyTaxiNet.onnx"
    evaluateFile(filename)
    
def test_conv_mp1():
    """
    Test a convolutional network using max pool, exported from pytorch
    Uses Conv, Relu, MaxPool, Constant, Reshape, Transpose, 
    Matmul, and Add layers
    """
    filename =  "conv_mp1.onnx"
    evaluateFile(filename)   
        
def evaluateFile(filename, testInputs = None):
    """
    Load network and evaluate testInputs with and without Marabou
    Args:
        filename (str): name of network file without path
    """
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_onnx(filename)
    
    # Create test points if none provided. This creates a list of test points.
    # Each test point is itself a list, representing the values for each input array.
    if not testInputs:
        testInputs = [[np.random.random(inVars.shape) for inVars in network.inputVars] for _ in range(NUM_RAND)]
    
    # Evaluate test points using both Marabou and ONNX
    for testInput in testInputs:
        marabouEval = network.evaluateWithMarabou(testInput, options = OPT, filename = "").flatten()
        onnxEval = network.evaluateWithoutMarabou(testInput).flatten()

        # Assert that both evaluations are the same within the set tolerance
        assert max(abs(marabouEval.flatten() - onnxEval.flatten())) < TOL
    
