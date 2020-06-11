# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from .. import Marabou
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0)        # Turn off printing
TOL = 1e-4                                        # Set tolerance for checking Marabou evaluations
FG_FOLDER = "../../resources/tf/frozen_graph/"    # Folder for test networks written in frozen graph format
SM1_FOLDER = "../../resources/tf/saved_model_v1/" # Folder for test networks written in SavedModel format from tensorflow v1.X
SM2_FOLDER = "../../resources/tf/saved_model_v2/" # Folder for test networks written in SavedModel format from tensorflow v2.X
np.random.seed(123)                               # Seed random numbers for repeatability
NUM_RAND = 10                                     # Default number of random test points per example

def test_fc1():
    """
    Test a fully-connected neural network
    Uses Const, Identity, Placeholder, MutMul, Add, and Relu layers
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "fc1.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)

def test_KJ_TinyTaxiNet():
    """
    Test a convolutional network without max-pooling
    Uses Const, Identity, Placeholder, Conv2D, BiasAdd, Reshape, 
    MatMul, Add, and Relu layers
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "KJ_TinyTaxiNet.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network)
    
def test_conv_mp1():
    """
    Test a convolutional network using max pool
    Uses Const, Identity, Placeholder, Conv2D, Add, Relu, and MaxPool layers
    The number of test points is decreased to reduce test time of this larger test network
    """
    filename = os.path.join(os.path.dirname(__file__), FG_FOLDER, "conv_mp1.pb")
    network = Marabou.read_tf(filename)
    evaluateNetwork(network, numPoints = 5) 
    
def test_sm1_fc1():
    """
    Test a fully-connected neural network, written in the 
    SavedModel format created by tensorflow version 1.X
    """
    filename = os.path.join(os.path.dirname(__file__), SM1_FOLDER, "fc1")
    network = Marabou.read_tf(filename, modelType = "savedModel_v1", outputName = "add_3")
    evaluateNetwork(network)
    
def test_sm2_fc1():
    """
    Test a fully-connected neural network, written in the 
    SavedModel format created by tensorflow version 2.X
    """
    filename = os.path.join(os.path.dirname(__file__), SM2_FOLDER, "fc1")
    network = Marabou.read_tf(filename, modelType = "savedModel_v2")
    evaluateNetwork(network)
        
def evaluateNetwork(network, testInputs = None, numPoints = NUM_RAND):
    """
    Evaluate a network at random testInputs with and without Marabou
    Args:
        network (MarabouNetwork): network loaded into Marabou to be evaluated
    """    
    # Create test points if none provided. This creates a list of test points.
    # Each test point is itself a list, representing the values for each input array.
    if not testInputs:
        testInputs = [[np.random.random(inVars.shape) for inVars in network.inputVars] for _ in range(numPoints)]
    
    # Evaluate test points using both Marabou and Tensorflow, and assert that the max error is less than TOL
    for testInput in testInputs:
        assert max(network.findError(testInput, options = OPT, filename = "").flatten()) < TOL
