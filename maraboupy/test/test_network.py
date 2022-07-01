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
NETWORK_ONNX_FOLDER = "../../resources/onnx/"   # Folder for test networks in onnx format

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

    disj1 = MarabouCore.Equation()

    # Replace output variables with their's absolute value
    for out in [0, 2]:
        abs_out = network.getNewVariable()
        network.addAbsConstraint(network.outputVars[0][0][out], abs_out)
        network.outputVars[0][0][out] = abs_out

    abs_inp = network.getNewVariable()
    network.outputVars = np.array([list(network.outputVars[0][0])+[abs_inp]])
    network.addAbsConstraint(network.inputVars[0][0][0], abs_inp)

    evaluateNetwork(network, testInputs, testOutputs)

def test_disjunction_constraint():
    """
    Tests the disjunction constraint.
    Based on the acas_1_1 test, with disjunction constraint added to the inputs.
    """
    filename =  "mnist/mnist10x10.nnet"

    network = loadNetwork(filename)

    test_out = network.getNewVariable()

    for var in network.inputVars[0][0]:
        # eq1: 1 * var = 0
        eq1 = MarabouCore.Equation(MarabouCore.Equation.EQ)
        eq1.addAddend(1, var)
        eq1.setScalar(0)

        # eq2: 1 * var = 1
        eq2 = MarabouCore.Equation(MarabouCore.Equation.EQ)
        eq2.addAddend(1, var)
        eq2.setScalar(1)

        # ( var = 0) \/ (var = 1)
        disjunction = [[eq1], [eq2]]
        network.addDisjunctionConstraint(disjunction)

        # add additional bounds for the variable
        network.setLowerBound(var, 0)
        network.setUpperBound(var, 1)

    exitCode1, vals1, stats1 = network.solve()

    for var in network.inputVars[0][0]:
        assert(abs(vals1[var] - 1) < 0.0000001 or abs(vals1[var]) < 0.0000001)

def test_sigmoid_constraint():
    """
    Tests the sigmoid constraint.
    """
    filename =  "fc_2-2sigmoids-3.onnx"

    network = loadNetworkInONNX(filename)

    assert len(network.sigmoidList) == 2

    for sigmoid in network.sigmoidList:
        assert network.lowerBounds[sigmoid[1]] == 0
        assert network.upperBounds[sigmoid[1]] == 1

def test_batch_norm():
    """
    Tests a batch normalization.
    """
    filename =  "linear2-3_bn1-linear3-1.onnx"

    network = loadNetworkInONNX(filename)
    options = Marabou.createOptions(verbosity = 0)

    inputVars = network.inputVars[0][0]
    outputVars = network.outputVars[0][0]

    network.setLowerBound(inputVars[0], 1)
    network.setUpperBound(inputVars[0], 1)
    network.setLowerBound(inputVars[1], 1)
    network.setUpperBound(inputVars[1], 1)

    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0]] - 9.9999799728) < TOL

def test_local_robustness_unsat():
    """
    Tests local robustness of an nnet network. (UNSAT)
    """
    filename =  "fc_2-2-3.nnet"

    network = loadNetwork(filename)
    options = Marabou.createOptions(verbosity = 0)

    input = np.array([1, 0])
    vals, stats, maxClass = network.evaluateLocalRobustness(input=input, epsilon=0.1, originalClass=0, options=options, targetClass=None)

    # should be local robustness
    assert(len(vals) == 0)

def test_local_robustness_sat():
    """
    Tests local robustness of an nnet network. (SAT)
    """
    filename =  "fc_2-2-3.nnet"

    network = loadNetwork(filename)
    options = Marabou.createOptions(verbosity = 0)

    input = np.array([1, -2])
    vals, stats, maxClass = network.evaluateLocalRobustness(input=input, epsilon=0.1, originalClass=0, options=options, targetClass=None)

    # should be not local robustness
    assert(len(vals) > 0)

def  test_local_robustness_unsat_of_onnx():
    """
    Tests local robustness of an onnx network. (UNSAT)
    """
    filename =  "fc_2-2-3.onnx"

    network = loadNetworkInONNX(filename)
    options = Marabou.createOptions(verbosity = 0)

    input = np.array([1, 0])
    vals, stats, maxClass = network.evaluateLocalRobustness(input=input, epsilon=0.1, originalClass=0, options=options, targetClass=None)

    # should be local robustness
    assert(len(vals) == 0)

def test_local_robustness_sat_of_onnx():
    """
    Tests local robustness of an onnx network. (SAT)
    """
    filename = "fc_2-2-3.onnx"

    network = loadNetworkInONNX(filename)
    options = Marabou.createOptions(verbosity = 0)

    input = np.array([1, 0])
    vals, stats, maxClass = network.evaluateLocalRobustness(input=input, epsilon=1.0, originalClass=0, options=options, targetClass=None)

    # should be not local robustness
    assert(len(vals) > 0)

def test_local_robustness_sat_with_target_class():
    """
    Tests local robustness of an onnx network. (SAT)
    """
    filename = "fc_2-2-3.onnx"

    network = loadNetworkInONNX(filename)
    options = Marabou.createOptions(verbosity = 0)

    input = np.array([1, -2])
    targetClass = 1
    vals, stats, maxClass = network.evaluateLocalRobustness(input=input, epsilon=0.1, originalClass=0, options=options, targetClass=targetClass)

    # should be not local robustness
    assert(len(vals) > 0)
    
    # maxClass should be equal to targetClass
    assert(maxClass == targetClass)

def test_local_robustness_sat_of_onnx_3D():
    """
    Tests local robustness of an onnx network which input's dimension is 3. (SAT)
    """
    filename = "KJ_TinyTaxiNet.onnx"

    network = loadNetworkInONNX(filename)
    options = Marabou.createOptions(verbosity = 0)

    input = np.ones([8, 16, 1]) * 0.5
    vals, stats, maxClass = network.evaluateLocalRobustness(input=input, epsilon=1.0, originalClass=0, options=options, targetClass=None)

    # should be not local robustness
    assert(len(vals) > 0)

def loadNetwork(filename):
    # Load network relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    return Marabou.read_nnet(filename)

def loadNetworkInONNX(filename):
    # Load network in onnx relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_ONNX_FOLDER, filename)
    return Marabou.read_onnx(filename)

def evaluateNetwork(network, testInputs, testOutputs):
    """
    Load network and evaluate testInputs with Marabou
    """

    for testInput, testOutput in zip(testInputs, testOutputs):
        marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "")[0].flatten()
        assert max(abs(marabouEval - testOutput)) < TOL

