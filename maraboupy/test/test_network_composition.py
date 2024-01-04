# Tests MarabouNetwork features not tested by it's subclasses
from maraboupy import Marabou
from maraboupy import MarabouCore
import os
import numpy as np

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-6                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/nnet/"   # Folder for test networks
NETWORK_ONNX_FOLDER = "../../resources/onnx/"   # Folder for test networks in onnx format

def test_zero_split_unknown():
    """
    Tests that a network with no splits is correctly read and solved as unknown
    """
    filename = 'fc1.onnx'
    filename = os.path.join(os.path.dirname(__file__), NETWORK_ONNX_FOLDER, filename)
    network = Marabou.read_onnx_with_threshold(filename, maxNumberOfLinearEquations=100)

    # check that the network has one split
    assert len(network.ipqs) == 1

    network.setLowerBound(network.inputVars[0][0][0], 3)
    network.setUpperBound(network.inputVars[0][0][0], 5)
    network.setLowerBound(network.inputVars[0][0][1], 3)
    network.setUpperBound(network.inputVars[0][0][1], 10)

    exitCode, _, _ = network.solve(options=OPT)

    assert exitCode == "UNKNOWN"
    
def test_zero_split_unsat():
    """ 
    Tests that a network with no splits is correctly read and solved as unsat
    """
    filename = 'fc1.onnx'
    filename = os.path.join(os.path.dirname(__file__), NETWORK_ONNX_FOLDER, filename)
    network = Marabou.read_onnx_with_threshold(filename, maxNumberOfLinearEquations=100)

    # check that the network has no splits
    assert len(network.ipqs) == 1

    network.setLowerBound(network.inputVars[0][0][0], 3)
    network.setUpperBound(network.inputVars[0][0][0], 5)
    network.setLowerBound(network.inputVars[0][0][1], 3)
    network.setUpperBound(network.inputVars[0][0][1], 10)

    network.setLowerBound(network.outputVars[0][0][0], 100)

    exitCode, _, _ = network.solve(options=OPT)

    assert exitCode == "unsat"
    
    network = Marabou.read_onnx(filename)
    network.setLowerBound(network.inputVars[0][0][0], 3)
    network.setUpperBound(network.inputVars[0][0][0], 5)
    network.setLowerBound(network.inputVars[0][0][1], 3)
    network.setUpperBound(network.inputVars[0][0][1], 10)

    network.setLowerBound(network.outputVars[0][0][0], 100)

    exitCode2, _, _ = network.calculateBounds(options=OPT)

    # exitCode2 should be also unsat
    assert exitCode == exitCode2

def test_one_split_unknown():
    """
    Tests that a network with one split is correctly read and solved as unknown
    """
    filename = 'fc1.onnx'
    filename = os.path.join(os.path.dirname(__file__), NETWORK_ONNX_FOLDER, filename)
    network = Marabou.read_onnx_with_threshold(filename, maxNumberOfLinearEquations=50)
    
    # check that the network has one split
    assert len(network.ipqs) == 2

    network.setLowerBound(network.inputVars[0][0][0], 3)
    network.setUpperBound(network.inputVars[0][0][0], 5)
    network.setLowerBound(network.inputVars[0][0][1], 3)
    network.setUpperBound(network.inputVars[0][0][1], 10)

    exitCode, _, _ = network.solve(options=OPT)

    assert exitCode == "UNKNOWN"

def test_one_split_unsat():
    """ 
    Tests that a network with one split is correctly read and solved as unsat
    """
    filename = 'fc1.onnx'
    filename = os.path.join(os.path.dirname(__file__), NETWORK_ONNX_FOLDER, filename)
    network = Marabou.read_onnx_with_threshold(filename, maxNumberOfLinearEquations=50)

    # check that the network has one split
    assert len(network.ipqs) == 2

    network.setLowerBound(network.inputVars[0][0][0], 3)
    network.setUpperBound(network.inputVars[0][0][0], 5)
    network.setLowerBound(network.inputVars[0][0][1], 3)
    network.setUpperBound(network.inputVars[0][0][1], 10)

    network.setLowerBound(network.outputVars[0][0][0], 100)

    exitCode, _, _ = network.solve(options=OPT)

    assert exitCode == "unsat"
    
    network = Marabou.read_onnx(filename)
    network.setLowerBound(network.inputVars[0][0][0], 3)
    network.setUpperBound(network.inputVars[0][0][0], 5)
    network.setLowerBound(network.inputVars[0][0][1], 3)
    network.setUpperBound(network.inputVars[0][0][1], 10)

    network.setLowerBound(network.outputVars[0][0][0], 100)

    exitCode2, _, _ = network.calculateBounds(options=OPT)

    # exitCode2 should be also unsat
    assert exitCode == exitCode2