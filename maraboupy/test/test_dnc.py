# Tests Marabou's DNC option
import pytest
from .. import Marabou
import os
import numpy as np

# Global settings
OPT = Marabou.createOptions(verbosity=0, snc=True, numWorkers=2) # Turn off printing, turn on DNC with two workers
TOL = 1e-6                                                       # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/nnet/acasxu"                   # Folder for test network

def test_dnc_unsat():
    """
    Test the 1,1 experimental ACAS Xu network. 
    Test a small input region with an output constraint that cannot be satisfied.
    """
    filename =  "ACASXU_experimental_v2a_1_1.nnet"
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_nnet(filename)
    centerPoint = [-0.2454504737724233, -0.4774648292756546, 0.0, -0.3181818181818182, 0.0]

    for var, val in zip(network.inputVars[0], centerPoint):
        network.setLowerBound(var, val - 0.002)
        network.setUpperBound(var, val + 0.002)

    # Set high lower bound on first output variable
    outVar = network.outputVars[0][0]
    network.setLowerBound(outVar, 0.1)

    # Expect UNSAT result
    vals, stats = network.solve(options = OPT, filename = "", verbose=False)
    assert len(vals) == 0
    
def test_dnc_sat():
    """
    Test the 1,1 experimental ACAS Xu network. 
    Test a small input region with an output constraint that can be satisfied.
    """
    filename =  "ACASXU_experimental_v2a_1_1.nnet"
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_nnet(filename)
    centerPoint = [-0.2454504737724233, -0.4774648292756546, 0.0, -0.3181818181818182, 0.0]

    for var, val in zip(network.inputVars[0], centerPoint):
        network.setLowerBound(var, val - 0.002)
        network.setUpperBound(var, val + 0.002)

    # Set a reduced lower bound on first output variable, which can be satisfied
    outVar = network.outputVars[0][0]
    network.setLowerBound(outVar, 0.0)

    # Expect SAT result, which should return a dictionary with a value for every network variable
    vals, stats = network.solve(options = OPT, filename = "", verbose=False)
    assert len(vals) == network.numVars

def test_dnc_eval():
    """
    Test the 1,1 experimental ACAS Xu network. 
    Evaluate the network at test points with DNC mode activated
    """
    filename =  "ACASXU_experimental_v2a_1_1.nnet"
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
    evaluateFile(filename, testInputs, testOutputs)

def evaluateFile(filename, testInputs, testOutputs):
    """ Load network and evaluate testInputs with and without Marabou
    
    Args:
        filename (str): name of network file without path
        testInputs (list of lists): list of inputs points to test
        testOutputs (list of lists): list of expected output values for the input points
    """
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network = Marabou.read_nnet(filename)

    if testOutputs:
        # Evaluate test points using Marabou and compare to known output
        for testInput, testOutput in zip(testInputs, testOutputs):
            marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "").flatten()
            assert max(abs(marabouEval - testOutput)) < TOL
