# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)

import pytest
from .. import Marabou
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-4                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/tf/"     # Folder for test networks
np.random.seed(123)                        # Seed random numbers for repeatability
NUM_RAND = 20                              # Default number of random test points per example

def test_fc1():
    """
    Test a fully-connected neural network
    Uses Const, Identity, Placeholder, MutMul, Add, and Relu layers
    """
    filename =  "fc1.pb"
    evaluateFile(filename)

def test_KJ_TinyTaxiNet():
    """
    Test a convolutional network without max-pooling
    Uses Const, Identity, Placeholder, Conv2D, BiasAdd, Reshape, 
    MatMul, Add, and Relu layers
    """
    filename =  "KJ_TinyTaxiNet.pb"
    evaluateFile(filename)
    
def test_conv_mp1():
    """
    Test a convolutional network using max pool
    Uses Const, Identity, Placeholder, Conv2D, Add, Relu, and MaxPool layers
    The number of test points is decreased to reduce test time of this larger test network
    """
    filename =  "conv_mp1.pb"
    evaluateFile(filename, numPoints = 5)   
        
def evaluateFile(filename, testInputs = None, numPoints = NUM_RAND):
    """
    Load network and evaluate testInputs with and without Marabou
    Args:
        filename (str): name of network file without path
    """
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_tf(filename)
    
    # Create test points if none provided. This creates a list of test points.
    # Each test point is itself a list, representing the values for each input array.
    if not testInputs:
        testInputs = [[np.random.random(inVars.shape) for inVars in network.inputVars] for _ in range(numPoints)]
    
    # Evaluate test points using both Marabou and Tensorflow
    for testInput in testInputs:
        marabouEval = network.evaluateWithMarabou(testInput, options = OPT, filename = "").flatten()
        tfEval = network.evaluateWithoutMarabou(testInput).flatten()

        # Assert that both evaluations are the same within the set tolerance
        assert max(abs(marabouEval.flatten() - tfEval.flatten())) < TOL
    
