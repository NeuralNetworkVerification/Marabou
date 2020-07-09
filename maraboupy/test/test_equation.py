# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from .. import Marabou
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0)     # Turn off printing
TOL = 1e-8                                     # Set tolerance for equality constraints
NETWORK_FILE = "../../resources/onnx/fc1.onnx" # File for test network
np.random.seed(123)                            # Seed random numbers for repeatability
NUM_RAND = 1                                   # Default number of random test points per example

def test_equality_output():
    """
    Test equality constraints applied to output variables.
    The first output variable should equal a randomly generated value
    """
    network = load_network()
    outputVars = network.outputVars.flatten()
    outputVar = outputVars[0]
    for _ in range(NUM_RAND):
        
        # Add output equality constraints
        outputValue = np.random.random() * 5 + 3
        network.addEquality([outputVar], [1.0], outputValue)

        # Call to Marabou solver
        vals, _ = network.solve(options = OPT, verbose = False)
        assert np.abs(vals[outputVar] - outputValue) < TOL
        
        # Remove inequality constraint, so that a new one can be applied in the next iteration
        network.equList = network.equList[:-1]

def test_equality_input():
    """
    Test equality constaints applied to input variables
    The average of the values of the input variables should equal 5.0
    The value of the second output variable must be greater than 70.0
    """
    network = load_network()
    
    # Input equality constraint
    inputVars = network.inputVars[0][0]
    weights = np.ones(inputVars.shape)/inputVars.size
    averageInputValue = 5.0
    network.addEquality(inputVars, weights, averageInputValue)
    
    # Lower bound on second output variable
    outputVars = network.outputVars.flatten()
    outputVar = outputVars[1]
    minOutputValue = 70.0
    network.setLowerBound(outputVar, minOutputValue)
    
    # Call to Marabou solver
    vals, _ = network.solve(options = OPT, verbose = False)
    assert np.abs(np.dot([vals[inVar] for inVar in inputVars], weights) - averageInputValue) < TOL
    assert vals[outputVar] >= minOutputValue

def test_inequality_output():
    """
    Test inequality constraints applied to output variables.
    The sum of the output variables must be less than or equal to a randomly generated value
    """
    network = load_network()
    
    outputVars = network.outputVars.flatten()
    weights = np.ones(outputVars.shape)
    for _ in range(NUM_RAND):
        
        # Output inequality constraint
        outputValue = np.random.random() * 90 + 10
        network.addInequality(outputVars, weights, outputValue)

        # Call to Marabou solver
        vals, _ = network.solve(options = OPT, verbose = False)
        assert np.dot([vals[outVar] for outVar in outputVars], weights) <= outputValue
        
        # Remove inequality constraint, so that a new one can be applied in the next iteration
        network.equList = network.equList[:-1]

def test_inequality_input():
    """
    Test inequality constraints applied to input variables.
    The sum of the input variables must be less than or equal to -5.0, and
    the second output variable must have a value of at least 70.0
    """
    network = load_network()        
    
    # Input inequality constraint
    inputVars = network.inputVars[0][0]
    weights = np.ones(inputVars.shape)/inputVars.size
    averageInputValue = -5.0
    network.addInequality(inputVars, weights, averageInputValue)
    
    # Add lower bound on second output variable
    outputVars = network.outputVars.flatten()
    outputVar = outputVars[1]
    minOutputValue = 70.0
    network.setLowerBound(outputVar, minOutputValue)
    
    # Call to Marabou solver
    vals, _ = network.solve(options = OPT, verbose = False)
    assert np.dot([vals[inVar] for inVar in inputVars], weights) <= averageInputValue
    assert vals[outputVar] >= minOutputValue
    
def load_network():
    """
    The test network fc1.onnx is used, which has two input variables and two output variables.
    The network was trained such that the first output approximates the sum of the absolute
    values of the inputs, while the second output approximates the sum of the squares of the inputs
    for inputs in the range [-10.0, 10.0].
    """
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FILE)
    network = Marabou.read_onnx(filename)
    
    # Get the input and output variable numbers; [0] since first dimension is batch size
    inputVars = network.inputVars[0][0]

    # Set input bounds
    network.setLowerBound(inputVars[0],-10.0)
    network.setUpperBound(inputVars[0], 10.0)
    network.setLowerBound(inputVars[1],-10.0)
    network.setUpperBound(inputVars[1], 10.0)
    
    return network

