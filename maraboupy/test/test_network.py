# Tests MarabouNetwork features not tested by it's subclasses
import pytest
from .. import Marabou
from .. import MarabouCore
import os
import numpy as np

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-6                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/nnet/"   # Folder for test networks

def test_abs_constraint():
    """
    Tests the absolute value constraint.
    Based on the acas_1_1 test, with abs constraint added to the outputs.
    """
    filename =  "acasxu/ACASXU_experimental_v2a_1_1.nnet"
    testInputs = [
        [-0.31182839647533234, 0.0, -0.2387324146378273, -0.5, -0.4166666666666667],
        [-0.16247807039378703, -0.4774648292756546, -0.2387324146378273, -0.3181818181818182, -0.25],
        [-0.2454504737724233, -0.4774648292756546, 0.0, -0.3181818181818182, 0.0]
    ]
    testOutputs = [
        [abs(0.45556007), 0.44454904, abs(0.49616356), 0.38924966, 0.50136678, abs(testInputs[0][0])],
        [abs(-0.02158248), -0.01885345, abs(-0.01892334), -0.01892597, -0.01893113, abs(testInputs[1][0])],
        [abs(0.05990158), 0.05273383, abs(0.10029709), 0.01883183, 0.10521622, abs(testInputs[2][0])]
    ]

    network = loadNetwork(filename)

    disj1 = MarabouCore.Equation();

    # Replace output variables with their's absolute value
    for out in [0, 2]:
        abs_out = network.getNewVariable()
        network.addAbsConstraint(network.outputVars[0][out], abs_out)
        network.outputVars[0][out] = abs_out

    abs_inp = network.getNewVariable()
    network.outputVars = np.array([list(network.outputVars[0])+[abs_inp]]) 
    network.addAbsConstraint(network.inputVars[0][0], abs_inp)

    evaluateNetwork(network, testInputs, testOutputs)

def test_disjunction_constraint():
    """
    Tests the disjunction constraint.
    Based on the acas_1_1 test, with disjunction constraint added to the inputs.
    """
    filename =  "mnist/mnist10x10.nnet"

    network = loadNetwork(filename)

    test_out = network.getNewVariable()

    for var in network.inputVars[0]:
        # eq1: 1 * var = 0
        eq1 = MarabouCore.Equation(MarabouCore.Equation.EQ);
        eq1.addAddend(1, var);
        eq1.setScalar(0);

        # eq2: 1 * var = 1
        eq2 = MarabouCore.Equation(MarabouCore.Equation.EQ);
        eq2.addAddend(1, var);
        eq2.setScalar(1);

        # ( var = 0) \/ (var = 1)
        disjunction = [[eq1], [eq2]]
        network.addDisjunctionConstraint(disjunction)

        # add additional bounds for the variable
        network.setLowerBound(var, 0)
        network.setUpperBound(var, 1)

    vals1, stats1 = network.solve()

    for var in network.inputVars[0]:
        assert(abs(vals1[var] - 1) < 0.0000001 or abs(vals1[var]) < 0.0000001)

def loadNetwork(filename):
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    return Marabou.read_nnet(filename)

def evaluateNetwork(network, testInputs, testOutputs):
    """
    Load network and evaluate testInputs with Marabou
    """

    for testInput, testOutput in zip(testInputs, testOutputs):
        marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "").flatten()

    assert max(abs(marabouEval - testOutput)) < TOL
    return network

