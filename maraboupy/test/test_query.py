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
TOL = 1e-4                                                                 # Tolerance for Marabou evaluations
ONNX_FILE = "../../resources/onnx/fc1.onnx"                                # File for test onnx network
ACAS_FILE = "../../resources/nnet/acasxu/ACASXU_experimental_v2a_1_1.nnet" # File for test nnet network

def test_sat_query(tmpdir):
    """
    Test that a query generated from Maraboupy can be saved and loaded correctly and return sat
    """
    network = load_onnx_network()
    
    # Set output constraint
    outputVars = network.outputVars.flatten()
    outputVar = outputVars[1]
    minOutputValue = 70.0
    network.setLowerBound(outputVar, minOutputValue)
    
    # Save this query to a temporary file, and reload the query
    queryFile = tmpdir.mkdir("query").join("query.txt").strpath
    network.saveQuery(queryFile)
    ipq = Marabou.load_query(queryFile)
    
    # Solve the query loaded from the file and compare to the solution of the original query
    # The result should be the same regardless of verbosity options used, or if a file redirect is used
    tempFile = tmpdir.mkdir("redirect").join("marabouRedirect.log").strpath
    opt = Marabou.createOptions(verbosity = 0)
    vals_net, _ = network.solve(filename = tempFile)
    vals_ipq, _ = Marabou.solve_query(ipq, filename = tempFile)
    
    # The two value dictionaries should have the same number of variables, 
    # the same keys, and the values assigned should be within some tolerance of each other
    assert len(vals_net) == len(vals_ipq)
    for k in vals_net:
        assert k in vals_ipq
        assert np.abs(vals_ipq[k] - vals_net[k]) < TOL
        
def test_unsat_query(tmpdir):
    """
    Test that a query generated from Maraboupy can be saved and loaded correctly and return unsat
    """
    network = load_onnx_network()
    
    # Set output constraint
    outputVars = network.outputVars.flatten()
    outputVar = outputVars[0]
    minOutputValue = 2000.0
    network.setLowerBound(outputVar, minOutputValue)
    
    # Save this query to a temporary file, and reload the query):
    queryFile = tmpdir.mkdir("query").join("query.txt").strpath
    network.saveQuery(queryFile)
    ipq = Marabou.load_query(queryFile)
    
    # Solve the query loaded from the file and compare to the solution of the original query
    opt = Marabou.createOptions(verbosity = 0)
    vals_net, stats_net = network.solve(options = opt)
    vals_ipq, stats_ipq = Marabou.solve_query(ipq, options = opt)
    
    # Assert the value dictionaries are both empty, and both queries have not timed out (unsat)
    assert len(vals_net) == 0
    assert len(vals_ipq) == 0
    assert not stats_net.hasTimedOut()
    assert not stats_ipq.hasTimedOut()
    
def test_to_query(tmpdir):
    """
    Test that a query generated from Maraboupy can be saved and loaded correctly and return timeout.
    This query is expected to be UNSAT but is currently unsolveable within one second.
    If future improvements allow the query to be solved within a second, then this test will need to be updated.
    """
    network = load_acas_network()
    
    # Set output constraint
    outputVars = network.outputVars.flatten()
    outputVar = outputVars[0]
    minOutputValue = 1500.0
    network.setLowerBound(outputVar, minOutputValue)
    
    # Save this query to a temporary file, and reload the query):
    queryFile = tmpdir.mkdir("query").join("query.txt").strpath
    network.saveQuery(queryFile)
    ipq = Marabou.load_query(queryFile)
    
    # Solve the query loaded from the file and compare to the solution of the original query
    opt = Marabou.createOptions(verbosity = 0, timeoutInSeconds = 1)
    vals_net, stats_net = network.solve(options = opt)
    vals_ipq, stats_ipq = Marabou.solve_query(ipq, options = opt)
    
    # Assert timeout
    assert stats_net.hasTimedOut()
    assert stats_ipq.hasTimedOut()


def test_get_marabou_query(tmpdir):
    '''
    Tests that input query generated from a network in Maraboupy is identical to the input query generated directly by
    Marabou from the same network file (relies on saveQuery, compares the output of saveQuery on the two queries).
    '''
    network = load_acas_network()
    ipq1 = network.getMarabouQuery()

    ipq2 = MarabouCore.InputQuery()
    network_filename = os.path.join(os.path.dirname(__file__), ACAS_FILE)
    MarabouCore.createInputQuery(ipq2, network_filename, '')

    ipq1_filename = tmpdir.mkdir("query").join("query1.txt").strpath
    ipq2_filename = tmpdir.join("query2.txt").strpath

    MarabouCore.saveQuery(ipq1, ipq1_filename)
    MarabouCore.saveQuery(ipq1, ipq2_filename)
    diff = call(['diff', ipq1_filename, ipq2_filename])
    assert not diff


def load_onnx_network():
    """
    The test network fc1.onnx is used, which has two input variables and two output variables.
    The network was trained such that the first output approximates the sum of the absolute
    values of the inputs, while the second output approximates the sum of the squares of the inputs
    for inputs in the range [-10.0, 10.0].
    """
    filename = os.path.join(os.path.dirname(__file__), ONNX_FILE)
    network = Marabou.read_onnx(filename)
    
    # Get the input and output variable numbers; [0] since first dimension is batch size
    inputVars = network.inputVars[0][0]

    # Set input bounds
    network.setLowerBound(inputVars[0],-10.0)
    network.setUpperBound(inputVars[0], 10.0)
    network.setLowerBound(inputVars[1],-10.0)
    network.setUpperBound(inputVars[1], 10.0)
    return network

def load_acas_network():
    """
    Load one of the acas networks. This network is larger than fc1.onnx, making it a better test case
    for testing timeout.
    """
    filename = os.path.join(os.path.dirname(__file__), ACAS_FILE)
    return Marabou.read_nnet(filename, normalize=True)
