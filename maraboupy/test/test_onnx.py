# Supress warnings caused by tensorflow
import warnings
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
NUM_RAND = 10                              # Default number of random test points per example

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
    
def test_KJ_TinyTaxiNet():
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
    evaluateFile(filename, inputNames = ['X'], outputName = 'Y')

def test_intermediate_input_output():
    """
    This function tests the parser's ability to use intermediate layers as the network
    inputs or output, effectively truncating the network. ONNX does not allow arbitrary
    layers to be used as inputs or outputs, which complicates testing.
    
    The network "conv_mp1.onnx" was modified to add the intermediate layer and first fully-connected
    layer, named '11', to the list of outputs, and saved as conv_mp1_intermediateOutput.onnx
    """
    filename =  "conv_mp1_intermediateOutput.onnx"
    evaluateIntermediateLayer(filename, inputNames=['X'], outputName = 'Y', intermediateName = '11')

def test_matMul():
    """
    Test a custom network that has uses MatMul in a few different ways
    """
    filename =  "fc_matMul.onnx"
    evaluateFile(filename)

def test_twoBranches():
    """
    Test a custom network that has one input used in two separate branches of computation,
    which are eventually added together
    """
    filename =  "oneInput_twoBranches.onnx"
    evaluateFile(filename, outputName = 'Y')
    evaluateIntermediateLayer(filename, inputNames = ['X'], outputName = 'Y', intermediateName = 'add1')
    
def test_multiInput_add(tmpdir):
    """
    Test a custom network that has two input arrays, which have separate Gemm and Relu operations
    before being added together. The rest of the network tries different types of Add oerations
    """
    filename =  "multiInput_add.onnx"
    evaluateFile(filename, inputNames = ['X1', 'X2'], outputName = 'Y')
    
    # If the intermediate layer we set to be the output depends on only one (or a subset) of
    # the inputs, we can omit the other inputs. ONNX, however, must receive values for 
    # all inputs in the graph, even if they have no effect on the output
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network_oneIn = Marabou.read_onnx(filename, inputNames = ['X1'], outputName = 'relu1')
    network_twoIn = Marabou.read_onnx(filename, inputNames = ['X1', 'X2'], outputName = 'relu1')
    
    testInputs = [[np.random.random(inVars.shape) for inVars in network_twoIn.inputVars] for _ in range(NUM_RAND)]
    
    # Evaluate test points using both Marabou and ONNX for both types of networks
    # Also, test evaluating with Marabou without explicitly specifying options
    # The result should be the same regardless of options used, or if a file redirect is used
    tempFile = tmpdir.mkdir("redirect").join("marabouRedirect.log").strpath
    for testInput in testInputs:
        marabouEval_oneIn = network_oneIn.evaluateWithMarabou([testInput[0]], filename = tempFile).flatten()
        marabouEval_twoIn = network_twoIn.evaluateWithMarabou(testInput, options = OPT, filename = "").flatten()
        onnxEval = network_twoIn.evaluateWithoutMarabou(testInput).flatten()

        # Assert that both networks produce the same result, and that they match the ONNX result
        assert max(abs(marabouEval_oneIn.flatten() - marabouEval_twoIn.flatten())) < TOL
        assert max(abs(marabouEval_oneIn.flatten() - onnxEval.flatten())) < TOL
        
        # Catch ONNX errors when not all input values are given to the graph
        with pytest.raises(RuntimeError, match = r"There are.*inputs to network"):
            network_oneIn.evaluateWithoutMarabou([testInput[0]])
    
def test_errors():
    """
    This function tests that the ONNX parser catches errors.
    """
    filename =  "conv_mp1.onnx"
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    
    filename_multiOutput =  "conv_mp1_intermediateOutput.onnx"
    filename_multiOutput = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename_multiOutput)
    
    # Test that we catch if inputNames or outputName are not in the model
    with pytest.raises(RuntimeError, match=r"multiple outputs defined"):
        Marabou.read_onnx(filename_multiOutput)
    with pytest.raises(RuntimeError, match=r"Input.*not found"):
        Marabou.read_onnx(filename, inputNames = ['BAD_NAME'], outputName = 'Y')
    with pytest.raises(RuntimeError, match=r"Output.*not found"):
        Marabou.read_onnx(filename, outputName = 'BAD_NAME')
        
    # The layer "12" is in the graph, but refers to a constant, so it should not be used 
    # as the network input or output.
    with pytest.raises(RuntimeError, match=r"input variables could not be found"):
        Marabou.read_onnx(filename, inputNames = ['12'], outputName = 'Y')
    with pytest.raises(RuntimeError, match=r"Output variable.*is a constant"):
        Marabou.read_onnx(filename, outputName = '12')
        
    # Evaluating with ONNX instead of Marabou gives errors when using a layer that is not already
    # defined as part of the model inputs or outputs.
    with pytest.raises(NotImplementedError, match=r"ONNX does not allow.*as inputs"):
        network = Marabou.read_onnx(filename, inputNames = ['11'], outputName = 'Y')
        network.evaluateWithoutMarabou([])
    with pytest.raises(NotImplementedError, match=r"ONNX does not allow.*the output"):
        network = Marabou.read_onnx(filename, inputNames = ['X'], outputName = '11')
        network.evaluateWithoutMarabou([])
        
def evaluateFile(filename, inputNames = None, outputName = None, testInputs = None, numPoints = NUM_RAND):
    """
    Load network and evaluate testInputs with and without Marabou
    Args:
        filename (str): name of network file without path
        inputNames (list of str): name of input layers
        outputName (str): name of output layer
        testInputs (list of arrays): test points to evaluate. Points created if none provided
        numPoints (int): Number of test points to create if testInputs is none
    """
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_onnx(filename, inputNames = inputNames, outputName = outputName)
    
    # Create test points if none provided. This creates a list of test points.
    # Each test point is itself a list, representing the values for each input array.
    if not testInputs:
        testInputs = [[np.random.random(inVars.shape) for inVars in network.inputVars] for _ in range(numPoints)]
    
    # Evaluate test points using both Marabou and ONNX, and assert that the error is less than TOL
    for testInput in testInputs:
        assert max(network.findError(testInput, options = OPT, filename = "").flatten()) < TOL
    
def evaluateIntermediateLayer(filename, inputNames = None, outputName = None, intermediateName = None, testInputs = None, numPoints = None):
    """
    This function loads three networks: the full network, the initial portion up to the intermediate layer, and 
    the final portion beginning with the intermediate layer. This function compares the Marabou networks to ONNX
    when possible, and checks that the two portions together give the same result as the full network.
    Args:
        filename (str): name of network file without path
        inputNames (list of str): name of input layers
        outputName (str): name of output layer
        intermediateName (str): name of intermediate layer. Must be added to graph.output in onnx model!
        testInputs (list of arrays): test points to evaluate. Points created if none provided
        numPoints (int): Number of test points to create if testInputs is none
    """
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    fullNetwork = Marabou.read_onnx(filename, inputNames = inputNames, outputName = outputName)
    startNetwork = Marabou.read_onnx(filename, inputNames = inputNames, outputName = intermediateName)
    endNetwork = Marabou.read_onnx(filename, inputNames = [intermediateName], outputName = outputName)
    
    testInputs = [[np.random.random(inVars.shape) for inVars in fullNetwork.inputVars] for _ in range(NUM_RAND)]
    for testInput in testInputs:
        marabouEval_full = fullNetwork.evaluateWithMarabou(testInput, options = OPT, filename = "")
        onnxEval_full = fullNetwork.evaluateWithoutMarabou(testInput)
        assert max(abs(marabouEval_full.flatten() - onnxEval_full.flatten())) < TOL
        
        marabouEval_start = startNetwork.evaluateWithMarabou(testInput, options = OPT, filename = "")
        onnxEval_start = startNetwork.evaluateWithoutMarabou(testInput)
        assert max(abs(marabouEval_start.flatten() - onnxEval_start.flatten())) < TOL
        
        marabouEval_end = endNetwork.evaluateWithMarabou([marabouEval_start], options = OPT, filename = "")
        assert max(abs(marabouEval_end.flatten() - onnxEval_full.flatten())) < TOL
