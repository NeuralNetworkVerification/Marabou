# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from .. import Marabou
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0)       # Turn off printing
TOL = 1e-4                                       # Set tolerance for checking Marabou evaluations
NETWORK_FILE = "../../resources/onnx/fc1.onnx"   # File for test network

def test_query(tmpdir):
    """
    Test that a query generated from Maraboupy can be saved and loaded correctly.
    """
    network = load_network()
    
    # Set output constraint
    outputVar = network.outputVars[1]
    minOutputValue = 70.0
    network.setLowerBound(outputVar, minOutputValue)
    
    # Save this query to a temporary file, and reload the query):
    queryFile = tmpdir.mkdir("query").join("query.txt").strpath
    network.saveQuery(queryFile)
    ipq = Marabou.load_query(queryFile)
    
    # Solve the query loaded from the file and compare to the solution of the original query
    vals_net, _ = network.solve(options = OPT)
    vals_ipq, _ = Marabou.solve_query(ipq, verbosity = 0)
    
    # The two value dictionaries should have the same number of variables, 
    # the same keys, and the values assigned should be within some tolerance of each other
    assert len(vals_net) == len(vals_ipq)
    for k in vals_net:
        assert k in vals_ipq
        assert np.abs(vals_ipq[k] - vals_net[k]) < TOL

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

