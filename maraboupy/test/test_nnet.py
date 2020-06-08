# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from .. import Marabou
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-6                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/nnet/"   # Folder for test networks
    
def test_acas_1_1():
    """
    Test the 1,1 experimental ACAS Xu network. 
    Properties are defined in the normalized input/output spaces, which is the default behavior for Marabou.
    """
    filename =  "acasxu/ACASXU_experimental_v2a_1_1.nnet"
    testInputs = [
        [-0.31182839647533234, 0.0, -0.2387324146378273, -0.5, -0.4166666666666667],
        [-0.16247807039378703, -0.4774648292756546, -0.2387324146378273, -0.3181818181818182, -0.25],
        [-0.2454504737724233, -0.4774648292756546, 0.0, -0.3181818181818182, 0.0]
    ]
    testOutputs = [
        [0.45556007, 0.44454904, 0.49616356, 0.38924966, 0.50136678],
        [-0.02158248, -0.01885345, -0.01892334, -0.01892597, -0.01893113],
        [0.05990158, 0.05273383, 0.10029709, 0.01883183, 0.10521622]
    ]
    network = evaluateFile(filename, testInputs, testOutputs)
    
    # Make sure input bounds are defined in the normalized space
    assert abs(network.getInputMaximum(0) - 0.6798577687061284) < TOL
    assert abs(network.getInputMinimum(1) + 0.5000000551328638) < TOL

def test_acas_1_1_normInput():
    """
    Test the 1,1 experimental ACAS Xu network.
    The network's stored input normalization terms are used to normalize the input points,
      so this test defines the same input points as the previous test, but in the original input space.
    Input points are normalized manually before solving with Marabou
    """
    filename =  "acasxu/ACASXU_experimental_v2a_1_1.nnet"
    testInputs = [
        [1000.0, 0.0, -1.5, 100.0, 100.0],
        [10000.0, -3.0, -1.5, 300.0, 300.0],
        [5000.0, -3.0, 0.0, 300.0, 600.0]
    ]
    testOutputs = [
        [0.45556007, 0.44454904, 0.49616356, 0.38924966, 0.50136678],
        [-0.02158248, -0.01885345, -0.01892334, -0.01892597, -0.01893113],
        [0.05990158, 0.05273383, 0.10029709, 0.01883183, 0.10521622]
    ]
    evaluateFile(filename, testInputs, testOutputs, normInput = True)     

def test_acas_1_1_manualNorm():
    """
    Test the 1,1 experimental ACAS Xu network.
    The network's input normalization terms are used to normalize the input points, and the network's output
      normalization terms are use to de-normalize the network output.
    These test points are defined in the original input/outputs spaces, but the network inputs/outputs must be
      manually normalized before calling Marabou, resulting in the same queries as the previous test.
    """
    filename =  "acasxu/ACASXU_experimental_v2a_1_1.nnet"
    testInputs = [
        [1000.0, 0.0, -1.5, 100.0, 100.0],
        [10000.0, -3.0, -1.5, 300.0, 300.0],
        [5000.0, -3.0, 0.0, 300.0, 600.0]
    ]
    testOutputs = [
        [177.87553729, 173.75796115, 193.05920806, 153.07876146, 195.00495022],
        [-0.55188079, 0.46863711, 0.44250383, 0.44151988, 0.43959133],
        [29.9190734, 27.2386958, 45.02497222, 14.5610455, 46.86448056]
    ]
    evaluateFile(filename, testInputs, testOutputs, normInput = True, denormOutput = True) 

def test_acas_1_1_normalize():
    """
    Test the 1,1 experimental ACAS Xu network.
    By passing "normalize=true" to read_nnet, Marabou adjusts the parameters of the first and last layers of the
      network to incorporate the normalization. 
    As a result, properties can be defined in the original input/output spaces without any manual normalization.
    """
    filename =  "acasxu/ACASXU_experimental_v2a_1_1.nnet"
    testInputs = [
        [1000.0, 0.0, -1.5, 100.0, 100.0],
        [10000.0, -3.0, -1.5, 300.0, 300.0],
        [5000.0, -3.0, 0.0, 300.0, 600.0]
    ]
    testOutputs = [
        [177.87553729, 173.75796115, 193.05920806, 153.07876146, 195.00495022],
        [-0.55188079, 0.46863711, 0.44250383, 0.44151988, 0.43959133],
        [29.9190734, 27.2386958, 45.02497222, 14.5610455, 46.86448056]
    ]
    network = evaluateFile(filename, testInputs, testOutputs, normalize = True)
    
    # Make sure input bounds are defined in original space
    assert abs(network.getInputMaximum(0) - 60760.0) < TOL
    assert abs(network.getInputMinimum(1) + 3.141593) < TOL
  
def test_acas_2_9_normalize():
    """
    Test the 2,9 experimental ACAS Xu network.
    """
    filename =  "acasxu/ACASXU_experimental_v2a_2_9.nnet"
    testInputs = [
        [1000.0, 0.0, -1.5, 100.0, 100.0],
        [10000.0, -3.0, -1.5, 300.0, 300.0],
        [5000.0, -3.0, 0.0, 300.0, 600.0]
    ]
    testOutputs = [
        [16.39351627, -0.03539009, 17.6841334, 3.4743032, 17.9557385],
        [-0.36330718, 0.14691009, 14.34997679, 0.5791588, 14.34728435],
        [16.8433828, -0.78962074, 15.27730208, 1.46803769, 15.09660353]
    ]
    evaluateFile(filename, testInputs, testOutputs, normalize = True)

def evaluateFile(filename, testInputs, testOutputs, normalize = False, normInput = False, denormOutput = False):
    """
    Load network and evaluate testInputs with and without Marabou
    Args:
        filename (str): name of network file without path
        
    """
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_nnet(filename, normalize=normalize)
    
    # Evaluate test points using Marabou and compare to known output
    for testInput, testOutput in zip(testInputs, testOutputs):
        
        # Manually normalize input point using network's stored inputMeans and inputRanges
        if normInput:
            testInput = [(var - varMean) / varRange for var, varMean, varRange in zip(testInput, network.inputMeans, network.inputRanges)]
        marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "").flatten()
        
        # Manually de-normalize network output using network's stored outputMean and outputRange
        if denormOutput:
            marabouEval = marabouEval*network.outputRange + network.outputMean

        assert max(abs(marabouEval - testOutput)) < TOL
    return network