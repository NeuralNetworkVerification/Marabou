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

def test_concat():
    """
    Tests a concat.
    """
    options = Marabou.createOptions(verbosity = 0)

    # axis = 0
    filename =  "concat/concat_axis_0.onnx"
    network = loadNetworkInONNX(filename)

    # Y = concat(X1, X2) + X3
    assert network.shapeMap['X1'] == [2, 2, 2]
    assert network.shapeMap['X2'] == [2, 2, 2]
    assert network.shapeMap['X3'] == [4, 2, 2]
    assert network.shapeMap['Y'] == [4, 2, 2]

    inputVars = network.inputVars
    outputVars = network.outputVars[0]

    # set bounds for X1 and X2
    num = 1
    for i in range(len(inputVars[0])):
        for j in range(len(inputVars[0][i])):
            for k in range(len(inputVars[0][i][j])):
                network.setLowerBound(inputVars[0][i][j][k], num)
                network.setUpperBound(inputVars[0][i][j][k], num)
                network.setLowerBound(inputVars[1][i][j][k], num + 10)
                network.setUpperBound(inputVars[1][i][j][k], num + 10)
                num += 1

    # set bounds for X3
    for i in range(len(inputVars[2])):
        for j in range(len(inputVars[2][i])):
            for k in range(len(inputVars[2][i][j])):
                network.setLowerBound(inputVars[2][i][j][k], 0)
                network.setUpperBound(inputVars[2][i][j][k], 0)

    _, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0][0]] - 1.0) < TOL
    assert abs(vals[outputVars[0][0][1]] - 2.0) < TOL
    assert abs(vals[outputVars[0][1][0]] - 3.0) < TOL
    assert abs(vals[outputVars[0][1][1]] - 4.0) < TOL
    assert abs(vals[outputVars[1][0][0]] - 5.0) < TOL
    assert abs(vals[outputVars[1][0][1]] - 6.0) < TOL
    assert abs(vals[outputVars[1][1][0]] - 7.0) < TOL
    assert abs(vals[outputVars[1][1][1]] - 8.0) < TOL
    assert abs(vals[outputVars[2][0][0]] - 11.0) < TOL
    assert abs(vals[outputVars[2][0][1]] - 12.0) < TOL
    assert abs(vals[outputVars[2][1][0]] - 13.0) < TOL
    assert abs(vals[outputVars[2][1][1]] - 14.0) < TOL
    assert abs(vals[outputVars[3][0][0]] - 15.0) < TOL
    assert abs(vals[outputVars[3][0][1]] - 16.0) < TOL
    assert abs(vals[outputVars[3][1][0]] - 17.0) < TOL
    assert abs(vals[outputVars[3][1][1]] - 18.0) < TOL

    # axis = 1
    filename =  "concat/concat_axis_1.onnx"
    network = loadNetworkInONNX(filename)

    # Y = concat(X1, X2) + X3
    assert network.shapeMap['X1'] == [2, 2, 2]
    assert network.shapeMap['X2'] == [2, 2, 2]
    assert network.shapeMap['X3'] == [2, 4, 2]
    assert network.shapeMap['Y'] == [2, 4, 2]

    inputVars = network.inputVars
    outputVars = network.outputVars[0]

    # set bounds for X1 and X2
    num = 1
    for i in range(len(inputVars[0])):
        for j in range(len(inputVars[0][i])):
            for k in range(len(inputVars[0][i][j])):
                network.setLowerBound(inputVars[0][i][j][k], num)
                network.setUpperBound(inputVars[0][i][j][k], num)
                network.setLowerBound(inputVars[1][i][j][k], num + 10)
                network.setUpperBound(inputVars[1][i][j][k], num + 10)
                num += 1

    # set bounds for X3
    for i in range(len(inputVars[2])):
        for j in range(len(inputVars[2][i])):
            for k in range(len(inputVars[2][i][j])):
                network.setLowerBound(inputVars[2][i][j][k], 0)
                network.setUpperBound(inputVars[2][i][j][k], 0)

    _, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0][0]] - 1.0) < TOL
    assert abs(vals[outputVars[0][0][1]] - 2.0) < TOL
    assert abs(vals[outputVars[0][1][0]] - 3.0) < TOL
    assert abs(vals[outputVars[0][1][1]] - 4.0) < TOL
    assert abs(vals[outputVars[0][2][0]] - 11.0) < TOL
    assert abs(vals[outputVars[0][2][1]] - 12.0) < TOL
    assert abs(vals[outputVars[0][3][0]] - 13.0) < TOL
    assert abs(vals[outputVars[0][3][1]] - 14.0) < TOL
    assert abs(vals[outputVars[1][0][0]] - 5.0) < TOL
    assert abs(vals[outputVars[1][0][1]] - 6.0) < TOL
    assert abs(vals[outputVars[1][1][0]] - 7.0) < TOL
    assert abs(vals[outputVars[1][1][1]] - 8.0) < TOL
    assert abs(vals[outputVars[1][2][0]] - 15.0) < TOL
    assert abs(vals[outputVars[1][2][1]] - 16.0) < TOL
    assert abs(vals[outputVars[1][3][0]] - 17.0) < TOL
    assert abs(vals[outputVars[1][3][1]] - 18.0) < TOL

    # axis = 2
    filename =  "concat/concat_axis_2.onnx"
    network = loadNetworkInONNX(filename)

    # Y = concat(X1, X2) + X3
    assert network.shapeMap['X1'] == [2, 2, 2]
    assert network.shapeMap['X2'] == [2, 2, 2]
    assert network.shapeMap['X3'] == [2, 2, 4]
    assert network.shapeMap['Y'] == [2, 2, 4]

    inputVars = network.inputVars
    outputVars = network.outputVars[0]

    # set bounds for X1 and X2
    num = 1
    for i in range(len(inputVars[0])):
        for j in range(len(inputVars[0][i])):
            for k in range(len(inputVars[0][i][j])):
                network.setLowerBound(inputVars[0][i][j][k], num)
                network.setUpperBound(inputVars[0][i][j][k], num)
                network.setLowerBound(inputVars[1][i][j][k], num + 10)
                network.setUpperBound(inputVars[1][i][j][k], num + 10)
                num += 1

    # set bounds for X3
    for i in range(len(inputVars[2])):
        for j in range(len(inputVars[2][i])):
            for k in range(len(inputVars[2][i][j])):
                network.setLowerBound(inputVars[2][i][j][k], 0)
                network.setUpperBound(inputVars[2][i][j][k], 0)

    _, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0][0]] - 1.0) < TOL
    assert abs(vals[outputVars[0][0][1]] - 2.0) < TOL
    assert abs(vals[outputVars[0][0][2]] - 11.0) < TOL
    assert abs(vals[outputVars[0][0][3]] - 12.0) < TOL
    assert abs(vals[outputVars[0][1][0]] - 3.0) < TOL
    assert abs(vals[outputVars[0][1][1]] - 4.0) < TOL
    assert abs(vals[outputVars[0][1][2]] - 13.0) < TOL
    assert abs(vals[outputVars[0][1][3]] - 14.0) < TOL
    assert abs(vals[outputVars[1][0][0]] - 5.0) < TOL
    assert abs(vals[outputVars[1][0][1]] - 6.0) < TOL
    assert abs(vals[outputVars[1][0][2]] - 15.0) < TOL
    assert abs(vals[outputVars[1][0][3]] - 16.0) < TOL
    assert abs(vals[outputVars[1][1][0]] - 7.0) < TOL
    assert abs(vals[outputVars[1][1][1]] - 8.0) < TOL
    assert abs(vals[outputVars[1][1][2]] - 17.0) < TOL
    assert abs(vals[outputVars[1][1][3]] - 18.0) < TOL

def test_split():
    """
    Tests a split.
    """
    options = Marabou.createOptions(verbosity = 0)

    # input: 1d
    # split: 2 (scalar)
    # axis: 0
    filename =  'split/split_1d_split-2_axis-0.onnx'

    # output1
    network = loadNetworkInONNX(filename, outputName='Y1')
    assert network.shapeMap['X'] == [6]
    assert network.shapeMap['Y1'] == (2,)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        network.setLowerBound(inputVars[i], i + 1)
        network.setUpperBound(inputVars[i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0]] - 1.0) < TOL
    assert abs(vals[outputVars[1]] - 2.0) < TOL

    # output2
    network = loadNetworkInONNX(filename, outputName='Y2')
    assert network.shapeMap['X'] == [6]
    assert network.shapeMap['Y2'] == (2,)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        network.setLowerBound(inputVars[i], i + 1)
        network.setUpperBound(inputVars[i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0]] - 6.0) < TOL
    assert abs(vals[outputVars[1]] - 8.0) < TOL

    # output3
    network = loadNetworkInONNX(filename, outputName='Y3')
    assert network.shapeMap['X'] == [6]
    assert network.shapeMap['Y3'] == (2,)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        network.setLowerBound(inputVars[i], i + 1)
        network.setUpperBound(inputVars[i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0]] - 15.0) < TOL
    assert abs(vals[outputVars[1]] - 18.0) < TOL

    # input: 1d
    # split: [2, 4] (array)
    # axis: 0
    filename =  'split/split_1d_split-2-4_axis-0.onnx'

    # output1
    network = loadNetworkInONNX(filename, outputName='Y1')
    assert network.shapeMap['X'] == [6]
    assert network.shapeMap['Y1'] == (2,)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        network.setLowerBound(inputVars[i], i + 1)
        network.setUpperBound(inputVars[i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0]] - 1.0) < TOL
    assert abs(vals[outputVars[1]] - 2.0) < TOL

    # output2
    network = loadNetworkInONNX(filename, outputName='Y2')
    assert network.shapeMap['X'] == [6]
    assert network.shapeMap['Y2'] == (4,)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        network.setLowerBound(inputVars[i], i + 1)
        network.setUpperBound(inputVars[i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0]] - 6.0) < TOL
    assert abs(vals[outputVars[1]] - 8.0) < TOL
    assert abs(vals[outputVars[2]] - 10.0) < TOL
    assert abs(vals[outputVars[3]] - 12.0) < TOL

    # input: 2d
    # split: 3 (scalar)
    # axis: 1
    filename =  'split/split_2d_split-3_axis-1.onnx'

    # output1
    network = loadNetworkInONNX(filename, outputName='Y1')
    assert network.shapeMap['X'] == [2, 6]
    assert network.shapeMap['Y1'] == (2, 3)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        for j in range(len(inputVars[i])):
            network.setLowerBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
            network.setUpperBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0]] - 1.0) < TOL
    assert abs(vals[outputVars[0][1]] - 2.0) < TOL
    assert abs(vals[outputVars[0][2]] - 3.0) < TOL
    assert abs(vals[outputVars[1][0]] - 7.0) < TOL
    assert abs(vals[outputVars[1][1]] - 8.0) < TOL
    assert abs(vals[outputVars[1][2]] - 9.0) < TOL

    # output2
    network = loadNetworkInONNX(filename, outputName='Y2')
    assert network.shapeMap['X'] == [2, 6]
    assert network.shapeMap['Y2'] == (2, 3)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        for j in range(len(inputVars[i])):
            network.setLowerBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
            network.setUpperBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0]] - 8.0) < TOL
    assert abs(vals[outputVars[0][1]] - 10.0) < TOL
    assert abs(vals[outputVars[0][2]] - 12.0) < TOL
    assert abs(vals[outputVars[1][0]] - 20.0) < TOL
    assert abs(vals[outputVars[1][1]] - 22.0) < TOL
    assert abs(vals[outputVars[1][2]] - 24.0) < TOL

    # input: 2d
    # split: [2, 4] (array)
    # axis: 1
    filename =  'split/split_2d_split-2-4_axis-1.onnx'

    # output1
    network = loadNetworkInONNX(filename, outputName='Y1')
    assert network.shapeMap['X'] == [2, 6]
    assert network.shapeMap['Y1'] == (2, 2)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        for j in range(len(inputVars[i])):
            network.setLowerBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
            network.setUpperBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0]] - 1.0) < TOL
    assert abs(vals[outputVars[0][1]] - 2.0) < TOL
    assert abs(vals[outputVars[1][0]] - 7.0) < TOL
    assert abs(vals[outputVars[1][1]] - 8.0) < TOL

    # output2
    network = loadNetworkInONNX(filename, outputName='Y2')
    assert network.shapeMap['X'] == [2, 6]
    assert network.shapeMap['Y2'] == (2, 4)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(len(inputVars)):
        for j in range(len(inputVars[i])):
            network.setLowerBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
            network.setUpperBound(inputVars[i][j], (len(inputVars[i])) * i + j + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0]] - 6.0) < TOL
    assert abs(vals[outputVars[0][1]] - 8.0) < TOL
    assert abs(vals[outputVars[0][2]] - 10.0) < TOL
    assert abs(vals[outputVars[0][3]] - 12.0) < TOL
    assert abs(vals[outputVars[1][0]] - 18.0) < TOL
    assert abs(vals[outputVars[1][1]] - 20.0) < TOL
    assert abs(vals[outputVars[1][2]] - 22.0) < TOL
    assert abs(vals[outputVars[1][3]] - 24.0) < TOL

    # For the case used in YOLOv5
    # input: 5d
    # split: [2, 2, 81] (array)
    # axis: 4
    filename =  'split/split_5d_split-2-2-81_axis-4.onnx'

    # output1
    network = loadNetworkInONNX(filename, outputName='Y1')
    assert network.shapeMap['X'] == [1, 1, 1, 1, 85]
    assert network.shapeMap['Y1'] == (1, 1, 1, 1, 2)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(85):
        network.setLowerBound(inputVars[0][0][0][0][i], i + 1)
        network.setUpperBound(inputVars[0][0][0][0][i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0][0][0][0]] - 1.0) < TOL
    assert abs(vals[outputVars[0][0][0][0][1]] - 2.0) < TOL

    # output2
    network = loadNetworkInONNX(filename, outputName='Y2')
    assert network.shapeMap['X'] == [1, 1, 1, 1, 85]
    assert network.shapeMap['Y2'] == (1, 1, 1, 1, 2)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(85):
        network.setLowerBound(inputVars[0][0][0][0][i], i + 1)
        network.setUpperBound(inputVars[0][0][0][0][i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    assert abs(vals[outputVars[0][0][0][0][0]] - 6.0) < TOL
    assert abs(vals[outputVars[0][0][0][0][1]] - 8.0) < TOL

    # output3
    network = loadNetworkInONNX(filename, outputName='Y3')
    assert network.shapeMap['X'] == [1, 1, 1, 1, 85]
    assert network.shapeMap['Y3'] == (1, 1, 1, 1, 81)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(85):
        network.setLowerBound(inputVars[0][0][0][0][i], i + 1)
        network.setUpperBound(inputVars[0][0][0][0][i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    
    for i in range(81):
        assert abs(vals[outputVars[0][0][0][0][i]] - (i + 5) * 3) < TOL
        assert abs(vals[outputVars[0][0][0][0][i]] - (i + 5) * 3) < TOL

    #
    # Split -> Mul -> Concat
    #
    filename =  'split/split_5d_split-2-2-81_axis-4-mul-concat.onnx'

    # output1
    network = loadNetworkInONNX(filename, outputName='Y')
    assert network.shapeMap['X'] == [1, 1, 1, 1, 85]
    assert network.shapeMap['Y'] == (1, 1, 1, 1, 85)

    inputVars = network.inputVars[0]
    outputVars = network.outputVars[0]
    for i in range(85):
        network.setLowerBound(inputVars[0][0][0][0][i], i + 1)
        network.setUpperBound(inputVars[0][0][0][0][i], i + 1)
    exitCode, vals, _ = network.solve(options = options)
    for i in range(85):
        assert abs(vals[outputVars[0][0][0][0][i]] - (i + 1.0) * 2) < TOL

def test_resize():
    """
    Tests a resize.
    """
    options = Marabou.createOptions(verbosity = 0)
    filename =  'resize/resize_4dims.onnx'

    network = loadNetworkInONNX(filename, outputName='Y')
    assert network.shapeMap['X'] == [1, 3, 2, 2]
    assert network.shapeMap['Y'] == (1, 3, 4, 4)

    inputVars = network.inputVars[0]
    inputValues = np.array(
        [[[[ 1.,  2.],
          [ 3.,  4.]],

         [[ 5.,  6.],
          [ 7.,  8.]],

         [[ 9., 10.],
          [11., 12.]]]])

    # set upper and lower bounds
    for i in range(len(inputVars)):
        for j in range(len(inputVars[i])):
            for k in range(len(inputVars[i][j])):
                for l in range(len(inputVars[i][j][k])):
                    network.setLowerBound(inputVars[i][j][k][l], inputValues[i][j][k][l])
                    network.setUpperBound(inputVars[i][j][k][l], inputValues[i][j][k][l])

    # solve
    _, vals, _ = network.solve(options = options)

    outputVars = network.outputVars[0]
    expectedOutputValues = np.array(
        [[[[ 1.,  1.,  2.,  2.],
          [ 1.,  1.,  2.,  2.],
          [ 3.,  3.,  4.,  4.],
          [ 3.,  3.,  4.,  4.]],

         [[ 5.,  5.,  6.,  6.],
          [ 5.,  5.,  6.,  6.],
          [ 7.,  7.,  8.,  8.],
          [ 7.,  7.,  8.,  8.]],

         [[ 9.,  9., 10., 10.],
          [ 9.,  9., 10., 10.],
          [11., 11., 12., 12.],
          [11., 11., 12., 12.]]]])

    for i in range(len(outputVars)):
        for j in range(len(outputVars[i])):
            for k in range(len(outputVars[i][j])):
                for l in range(len(outputVars[i][j][k])):
                    assert abs(vals[outputVars[i][j][k][l]] - expectedOutputValues[i][j][k][l]) < TOL

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

def loadNetworkInONNX(filename, inputNames=None, outputName=None):
    # Load network in onnx relative to this file's location
    filename = os.path.join(os.path.dirname(__file__), NETWORK_ONNX_FOLDER, filename)
    return Marabou.read_onnx(filename, inputNames, outputName)

def evaluateNetwork(network, testInputs, testOutputs):
    """
    Load network and evaluate testInputs with Marabou
    """

    for testInput, testOutput in zip(testInputs, testOutputs):
        marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "")[0].flatten()
        assert max(abs(marabouEval - testOutput)) < TOL

