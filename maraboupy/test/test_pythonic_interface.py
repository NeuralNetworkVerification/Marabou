# Tests Marabou's Pythonic API
import os
import pytest
from maraboupy import Marabou
from maraboupy.MarabouPythonic import *

NETWORK_FOLDER = "../../resources/onnx/"


def test_pythonic_interface():
    """
    Test the Pythonic API for adding various constraints to neurons of a network.

    :return: pass the test if everything works.
    """
    filename = "fc1.onnx"
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network_a = Marabou.read_onnx(filename)
    network_b = Marabou.read_onnx(filename)
    """
    Highlight various supported constraints on a neuron.
    """
    for i in range(network_a.numVars):
        v = Var(i)
        network_a.addConstraint(v <= 7.5)
        network_b.setUpperBound(i, 7.5)
        network_a.addConstraint(v >= 4)
        network_b.setLowerBound(i, 4)
        network_a.addConstraint(v + 6 >= 8)
        network_b.setLowerBound(i, 8 - 6)
        network_a.addConstraint(4 >= 14 - v)
        network_b.addInequality([i], [-1], 4 - 14)
        network_a.addConstraint(5 * v <= 8)
        network_b.addInequality([i], [5], 8)
        network_a.addConstraint(v * 6 >= -10)
        network_b.addInequality([i], [-6], 10)
        network_a.addConstraint(- v * 10 >= 8)
        network_b.addInequality([i], [10], -8)
        network_a.addConstraint(v * 6 + 10 == 12)
        network_b.addEquality([i], [6], 12 - 10)
        network_a.addConstraint(v * 10 - 5 * v <= -8)
        network_b.addInequality([i], [10 - 5], -8)
        network_a.addConstraint(5 * (2 * v + 8) <= 6)
        network_b.addInequality([i], [5 * 2], 6 - 5 * 8)
    assert network_a.isEqualTo(network_b)

    filename = "fc2.onnx"
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network_a = Marabou.read_onnx(filename)
    network_b = Marabou.read_onnx(filename)
    """
    Highlight supported/unsupported constraints on more than one neurons.
    """
    for i in range(10):
        v = Var(i)
        u = Var(i + 1)
        w = Var(i + 2)
        network_a.addConstraint(v >= u)
        network_b.addInequality([i + 1, i], [1, -1], 0)
        network_a.addConstraint(u <= w)
        network_b.addInequality([i + 1, i + 2], [1, -1], 0)
        network_a.addConstraint(v + u >= 20)
        network_b.addInequality([i, i + 1], [-1, -1], -20)
        network_a.addConstraint(u - w <= 30)
        network_b.addInequality([i + 1, i + 2], [1, -1], 30)
        network_a.addConstraint(3 * v + 2 * u - 4 * w <= 50)
        network_b.addInequality([i, i + 1, i + 2], [3, 2, -4], 50)
        network_a.addConstraint(v * 4 - w <= 6 * u)
        network_b.addInequality([i, i + 2, i + 1], [4, -1, -6], 0)
        with pytest.raises(NotImplementedError,
                           match="Only linear constraints are supported."):
            network_a.addConstraint(v * u >= w)
    assert network_a.isEqualTo(network_b)

    filename = "cnn_max_mninst2.onnx"
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, filename)
    network_a = Marabou.read_onnx(filename)
    network_b = Marabou.read_onnx(filename)
    """
    A realistic example on the MNIST dataset to set input/output constraints.
    """
    invars = network_a.inputVars[0][0].flatten()
    for i in invars:
        v = Var(i)
        network_a.addConstraint(v <= 7.5)
        network_b.setUpperBound(i, 7.5)
        network_a.addConstraint(v >= 4)
        network_b.setLowerBound(i, 4)
    outvars = network_a.outputVars[0].flatten()
    u = Var(outvars[-1])
    for j in outvars[:-1]:
        v = Var(j)
        network_a.addConstraint(v <= 30)
        network_b.setUpperBound(j, 30)
        network_a.addConstraint(v >= 2)
        network_b.setLowerBound(j, 2)
        network_a.addConstraint(v <= u)
        network_b.addInequality([j, outvars[-1]], [1, -1], 0)
        network_a.addConstraint(v + u >= 20)
        network_b.addInequality([j, outvars[-1]], [-1, -1], -20)
        network_a.addConstraint(7 * v - 11 * u <= 4)
        network_b.addInequality([j, outvars[-1]], [7, -11], 4)
    assert network_a.isEqualTo(network_b)
