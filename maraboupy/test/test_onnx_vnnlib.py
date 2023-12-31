# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from maraboupy import Marabou
from maraboupy import MarabouCore
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-4                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/onnx/"   # Folder for test networks

def test_acasxu():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "acasxu", "ACASXU_experimental_v2a_1_1.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "acasxu_prop1.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == i

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == i + 5

    # Check input bounds:
    assert network.equList[-11].EquationType == MarabouCore.Equation.LE and network.equList[-11].addendList == [(1, 0)] and network.equList[-11].scalar == 0.679857769
    assert network.equList[-10].EquationType == MarabouCore.Equation.GE and network.equList[-10].addendList == [(1, 0)] and network.equList[-10].scalar == 0.6

    assert network.equList[-9].EquationType == MarabouCore.Equation.LE and network.equList[-9].addendList == [(1, 1)] and network.equList[-9].scalar == 0.5
    assert network.equList[-8].EquationType == MarabouCore.Equation.GE and network.equList[-8].addendList == [(1, 1)] and network.equList[-8].scalar == -0.5

    assert network.equList[-7].EquationType == MarabouCore.Equation.LE and network.equList[-7].addendList == [(1, 2)] and network.equList[-7].scalar == 0.5
    assert network.equList[-6].EquationType == MarabouCore.Equation.GE and network.equList[-6].addendList == [(1, 2)] and network.equList[-6].scalar == -0.5

    assert network.equList[-5].EquationType == MarabouCore.Equation.LE and network.equList[-5].addendList == [(1, 3)] and network.equList[-5].scalar == 0.5
    assert network.equList[-4].EquationType == MarabouCore.Equation.GE and network.equList[-4].addendList == [(1, 3)] and network.equList[-4].scalar == 0.45

    assert network.equList[-3].EquationType == MarabouCore.Equation.LE and network.equList[-3].addendList == [(1, 4)] and network.equList[-3].scalar == -0.45
    assert network.equList[-2].EquationType == MarabouCore.Equation.GE and network.equList[-2].addendList == [(1, 4)] and network.equList[-2].scalar == -0.5

    # Check output bound
    assert network.equList[-1].EquationType == MarabouCore.Equation.GE and network.equList[-1].addendList == [(1, 5)] and network.equList[-1].scalar == 3.991125645861615

    # Check constraints held on Marabou:
    # exit_code, vals, _ = network.solve()
    # assert (exit_code == "sat" and vals[5] >= 3.991125645861615) or exit_code == "unsat"


def test_nano_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == 0
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == 1

    # Check input bounds:
    assert network.equList[-3].EquationType == MarabouCore.Equation.GE and network.equList[-3].addendList == [(1, 0)] and network.equList[-3].scalar == -1
    assert network.equList[-2].EquationType == MarabouCore.Equation.LE and network.equList[-2].addendList == [(1, 0)] and network.equList[-2].scalar == 1

    # Check output bound
    assert network.equList[-1].EquationType == MarabouCore.Equation.LE and network.equList[-1].addendList == [(1, 1)] and network.equList[-1].scalar == -1

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve()
    assert (exit_code == "sat" and vals[1] <= -1) or exit_code == "unsat"

def test_tiny_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_tiny_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_tiny_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == 0
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == 1

    # TODO: Uncomment assertions after fixing disjunction constraint bug
    # Check disjunction
    # assert len(network.disjunctionList) == 1
    # assert len(network.disjunctionList[0]) == 1
    # assert len(network.disjunctionList[0][0]) == 3
    # assert network.disjunctionList[0][0][0].EquationType == MarabouCore.Equation.GE and network.disjunctionList[0][0][0].addendList == [(1, 0)] and network.disjunctionList[0][0][0].scalar == -1
    # assert network.disjunctionList[0][0][1].EquationType == MarabouCore.Equation.LE and network.disjunctionList[0][0][1].addendList == [(1, 0)] and network.disjunctionList[0][0][1].scalar == 1
    # assert network.disjunctionList[0][0][2].EquationType == MarabouCore.Equation.GE and network.disjunctionList[0][0][2].addendList == [(1, 1)] and network.disjunctionList[0][0][2].scalar == 100

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve()
    # assert (exit_code == "sat" and vals[1] >= 100) or exit_code == "unsat"

def test_small_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_small_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_small_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == 0
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == 1

    # TODO: Uncomment assertions after fixing disjunction constraint bug
    # Check disjunction
    # assert len(network.disjunctionList) == 1
    # assert len(network.disjunctionList[0]) == 1
    # assert len(network.disjunctionList[0][0]) == 3
    # assert network.disjunctionList[0][0][0].EquationType == MarabouCore.Equation.GE and network.disjunctionList[0][0][0].addendList == [(1, 0)] and network.disjunctionList[0][0][0].scalar == -1
    # assert network.disjunctionList[0][0][1].EquationType == MarabouCore.Equation.LE and network.disjunctionList[0][0][1].addendList == [(1, 0)] and network.disjunctionList[0][0][1].scalar == 1
    # assert network.disjunctionList[0][0][2].EquationType == MarabouCore.Equation.GE and network.disjunctionList[0][0][2].addendList == [(1, 1)] and network.disjunctionList[0][0][2].scalar == 100

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve()
    # assert (exit_code == "sat" and vals[1] >= 100) or exit_code == "unsat"

def test_sat_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_sat_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_prop_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == i

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == i + 5

    # Check input bounds:
    assert network.equList[-14].EquationType == MarabouCore.Equation.LE and network.equList[-14].addendList == [(1, 0)] and network.equList[-14].scalar == -0.29855281193475053
    assert network.equList[-13].EquationType == MarabouCore.Equation.GE and network.equList[-13].addendList == [(1, 0)] and network.equList[-13].scalar == -0.30353115613746867

    assert network.equList[-12].EquationType == MarabouCore.Equation.LE and network.equList[-12].addendList == [(1, 1)] and network.equList[-12].scalar == 0.009549296585513092
    assert network.equList[-11].EquationType == MarabouCore.Equation.GE and network.equList[-11].addendList == [(1, 1)] and network.equList[-11].scalar == -0.009549296585513092

    assert network.equList[-10].EquationType == MarabouCore.Equation.LE and network.equList[-10].addendList == [(1, 2)] and network.equList[-10].scalar == 0.49999999998567607
    assert network.equList[-9].EquationType == MarabouCore.Equation.GE and network.equList[-9].addendList == [(1, 2)] and network.equList[-9].scalar == 0.4933803235848431

    assert network.equList[-8].EquationType == MarabouCore.Equation.LE and network.equList[-8].addendList == [(1, 3)] and network.equList[-8].scalar == 0.5
    assert network.equList[-7].EquationType == MarabouCore.Equation.GE and network.equList[-7].addendList == [(1, 3)] and network.equList[-7].scalar == 0.3

    assert network.equList[-6].EquationType == MarabouCore.Equation.LE and network.equList[-6].addendList == [(1, 4)] and network.equList[-6].scalar == 0.5
    assert network.equList[-5].EquationType == MarabouCore.Equation.GE and network.equList[-5].addendList == [(1, 4)] and network.equList[-5].scalar == 0.3

    # Check output bound
    assert network.equList[-4].EquationType == MarabouCore.Equation.LE and network.equList[-4].addendList == [(1, 5), (-1, 6)] and network.equList[-4].scalar == 0
    assert network.equList[-3].EquationType == MarabouCore.Equation.LE and network.equList[-3].addendList == [(1, 5), (-1, 7)] and network.equList[-3].scalar == 0
    assert network.equList[-2].EquationType == MarabouCore.Equation.LE and network.equList[-2].addendList == [(1, 5), (-1, 8)] and network.equList[-2].scalar == 0
    assert network.equList[-1].EquationType == MarabouCore.Equation.LE and network.equList[-1].addendList == [(1, 5), (-1, 9)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve()
    assert exit_code == "sat" and vals[5] <= min([vals[i] for i in range(6, 10)])

def test_unsat_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_unsat_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_prop_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == i

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == i + 5

    # Check input bounds:
    assert network.equList[-14].EquationType == MarabouCore.Equation.LE and network.equList[-14].addendList == [(1, 0)] and network.equList[-14].scalar == -0.29855281193475053
    assert network.equList[-13].EquationType == MarabouCore.Equation.GE and network.equList[-13].addendList == [(1, 0)] and network.equList[-13].scalar == -0.30353115613746867

    assert network.equList[-12].EquationType == MarabouCore.Equation.LE and network.equList[-12].addendList == [(1, 1)] and network.equList[-12].scalar == 0.009549296585513092
    assert network.equList[-11].EquationType == MarabouCore.Equation.GE and network.equList[-11].addendList == [(1, 1)] and network.equList[-11].scalar == -0.009549296585513092

    assert network.equList[-10].EquationType == MarabouCore.Equation.LE and network.equList[-10].addendList == [(1, 2)] and network.equList[-10].scalar == 0.49999999998567607
    assert network.equList[-9].EquationType == MarabouCore.Equation.GE and network.equList[-9].addendList == [(1, 2)] and network.equList[-9].scalar == 0.4933803235848431

    assert network.equList[-8].EquationType == MarabouCore.Equation.LE and network.equList[-8].addendList == [(1, 3)] and network.equList[-8].scalar == 0.5
    assert network.equList[-7].EquationType == MarabouCore.Equation.GE and network.equList[-7].addendList == [(1, 3)] and network.equList[-7].scalar == 0.3

    assert network.equList[-6].EquationType == MarabouCore.Equation.LE and network.equList[-6].addendList == [(1, 4)] and network.equList[-6].scalar == 0.5
    assert network.equList[-5].EquationType == MarabouCore.Equation.GE and network.equList[-5].addendList == [(1, 4)] and network.equList[-5].scalar == 0.3

    # Check output bound
    assert network.equList[-4].EquationType == MarabouCore.Equation.LE and network.equList[-4].addendList == [(1, 5), (-1, 6)] and network.equList[-4].scalar == 0
    assert network.equList[-3].EquationType == MarabouCore.Equation.LE and network.equList[-3].addendList == [(1, 5), (-1, 7)] and network.equList[-3].scalar == 0
    assert network.equList[-2].EquationType == MarabouCore.Equation.LE and network.equList[-2].addendList == [(1, 5), (-1, 8)] and network.equList[-2].scalar == 0
    assert network.equList[-1].EquationType == MarabouCore.Equation.LE and network.equList[-1].addendList == [(1, 5), (-1, 9)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve()
    assert exit_code == "unsat"