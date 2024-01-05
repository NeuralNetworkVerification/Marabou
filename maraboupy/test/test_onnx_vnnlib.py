# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from maraboupy import Marabou
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-4                                 # Set tolerance for checking Marabou evaluations
NETWORK_FOLDER = "../../resources/onnx/"   # Folder for test networks


def test_exp_notation():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_exp_notation.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check input bounds:
    assert network.equList[-3].addendList == [(-1, 0)] and network.equList[-3].scalar == 1e0
    assert network.equList[-2].addendList == [(1, 0)] and network.equList[-2].scalar == 1e0

    # Check output bound
    assert network.equList[-1].addendList == [(-1, 1)] and network.equList[-1].scalar == -0.5

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert (exit_code == "sat" and -1 - TOL <= vals[0] <= 1 + TOL and vals[1] >= 0.5 - TOL) or exit_code == "unsat"


def test_and():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_and.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check input bounds:
    assert network.equList[-3].addendList == [(-1, 0)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(1, 0)] and network.equList[-2].scalar == 1

    # Check output bound
    assert network.equList[-1].addendList == [(-1, 1)] and network.equList[-1].scalar == -1

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert (exit_code == "sat" and  -TOL <= vals[0] <= 1 + TOL and vals[1] >= 0.5 - TOL) or exit_code == "unsat"

def test_command_not_implemented():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_command_not_implemented.vnnlib")

    with pytest.raises(NotImplementedError):
        Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

def test_assert_operator_not_implemented():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_assert_operator_not_implemented.vnnlib")

    with pytest.raises(NotImplementedError):
        Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

def test_add_const():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_add_const.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check input bounds:
    assert network.equList[-3].addendList == [(-1, 0)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(1, 0)] and network.equList[-2].scalar == 1

    # Check output bound
    assert network.equList[-1].addendList == [(-1, 1)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert (exit_code == "sat" and  -TOL <= vals[0] <= 1 + TOL and vals[1] >= -TOL) or exit_code == "unsat"

def test_add_var():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_sat_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_add_var.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 10
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == (i, "Real")

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == (i + 5, "Real")

    # Check input bounds:
    assert network.equList[-14].addendList == [(1, 0)] and network.equList[-14].scalar == -0.29855281193475053
    assert network.equList[-13].addendList == [(-1, 0)] and network.equList[-13].scalar == 0.30353115613746867

    assert network.equList[-12].addendList == [(1, 1)] and network.equList[-12].scalar == 0.009549296585513092
    assert network.equList[-11].addendList == [(-1, 1)] and network.equList[-11].scalar == 0.009549296585513092

    assert network.equList[-10].addendList == [(1, 2)] and network.equList[-10].scalar == 0.49999999998567607
    assert network.equList[-9].addendList == [(-1, 2)] and network.equList[-9].scalar == -0.4933803235848431

    assert network.equList[-8].addendList == [(1, 3)] and network.equList[-8].scalar == 0.5
    assert network.equList[-7].addendList == [(-1, 3)] and network.equList[-7].scalar == -0.3

    assert network.equList[-6].addendList == [(1, 4)] and network.equList[-6].scalar == 0.5
    assert network.equList[-5].addendList == [(-1, 4)] and network.equList[-5].scalar == -0.3

    # Check output bound
    assert network.equList[-4].addendList == [(-1, 5)] and network.equList[-4].scalar == 0
    assert network.equList[-3].addendList == [(-1, 6)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(-1, 7)] and network.equList[-2].scalar == 0
    assert network.equList[-1].addendList == [(1, 5), (1, 6)] and network.equList[-1].scalar == 1

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert exit_code == "unsat" or (exit_code == "sat" and
           -0.30353115613746867 - TOL <= vals[0] <= -0.29855281193475053 + TOL and
           -0.009549296585513092 - TOL <= vals[1] <= 0.009549296585513092 + TOL and
           0.4933803235848431 - TOL <= vals[2] <= 0.49999999998567607 + TOL and
           0.3 - TOL <= vals[3] <= 0.5 + TOL and
           0.3 - TOL <= vals[4] <= 0.5 + TOL and
           vals[5] >= -TOL and vals[6] >= -TOL and vals[7] >= -TOL and vals[5] + vals[6] <= 1 + TOL)

def test_sub_const():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_sub_const.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check input bounds:
    assert network.equList[-3].addendList == [(-1, 0)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(1, 0)] and network.equList[-2].scalar == 1

    # Check output bound
    assert network.equList[-1].addendList == [(-1, 1)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert (exit_code == "sat" and  -TOL <= vals[0] <= 1 + TOL and vals[1] >= -TOL) or exit_code == "unsat"


def test_sub_var():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_sat_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_sub_var.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 10
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == (i, "Real")

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == (i + 5, "Real")

    # Check input bounds:
    assert network.equList[-14].addendList == [(1, 0)] and network.equList[-14].scalar == -0.29855281193475053
    assert network.equList[-13].addendList == [(-1, 0)] and network.equList[-13].scalar == 0.30353115613746867

    assert network.equList[-12].addendList == [(1, 1)] and network.equList[-12].scalar == 0.009549296585513092
    assert network.equList[-11].addendList == [(-1, 1)] and network.equList[-11].scalar == 0.009549296585513092

    assert network.equList[-10].addendList == [(1, 2)] and network.equList[-10].scalar == 0.49999999998567607
    assert network.equList[-9].addendList == [(-1, 2)] and network.equList[-9].scalar == -0.4933803235848431

    assert network.equList[-8].addendList == [(1, 3)] and network.equList[-8].scalar == 0.5
    assert network.equList[-7].addendList == [(-1, 3)] and network.equList[-7].scalar == -0.3

    assert network.equList[-6].addendList == [(1, 4)] and network.equList[-6].scalar == 0.5
    assert network.equList[-5].addendList == [(-1, 4)] and network.equList[-5].scalar == -0.3

    # Check output bound
    assert network.equList[-4].addendList == [(-1, 5)] and network.equList[-4].scalar == 0
    assert network.equList[-3].addendList == [(-1, 6)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(-1, 7)] and network.equList[-2].scalar == 0
    assert network.equList[-1].addendList == [(1, 5), (-1, 6)] and network.equList[-1].scalar == 1

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert exit_code == "unsat" or (exit_code == "sat" and
           -0.30353115613746867 - TOL <= vals[0] <= -0.29855281193475053 + TOL and
           -0.009549296585513092 - TOL <= vals[1] <= 0.009549296585513092 + TOL and
           0.4933803235848431 - TOL <= vals[2] <= 0.49999999998567607 + TOL and
           0.3 - TOL <= vals[3] <= 0.5 + TOL and
           0.3 - TOL <= vals[4] <= 0.5 + TOL and
           vals[5] >= -TOL and vals[6] >= -TOL and vals[7] >= -TOL and vals[5] - vals[6] >= 1 - TOL)

def test_mul_var_const():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_mul_var_const.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check input bounds:
    assert network.equList[-4].addendList == [(-2, 0)] and network.equList[-4].scalar == 0
    assert network.equList[-3].addendList == [(2, 0)] and network.equList[-3].scalar == 2
    assert network.equList[-2].addendList == [(0, 0)] and network.equList[-2].scalar == 1000

    # Check output bound
    assert network.equList[-1].addendList == [(-1, 1)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert (exit_code == "sat" and  2 * vals[0] >= -TOL and -2 * vals[0] >= -2 and vals[1] >= -TOL) or exit_code == "unsat"

def test_mul_var_var():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_mul_var_var.vnnlib")

    with pytest.raises(RuntimeError):
        Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

def test_nano_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_nano_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check input bounds:
    assert network.equList[-3].addendList == [(-1, 0)] and network.equList[-3].scalar == 1
    assert network.equList[-2].addendList == [(1, 0)] and network.equList[-2].scalar == 1

    # Check output bound
    assert network.equList[-1].addendList == [(1, 1)] and network.equList[-1].scalar == -1

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert (exit_code == "sat" and -1 - TOL <= vals[0] <= 1 + TOL and vals[1] <= -1 + TOL) or exit_code == "unsat"

def test_tiny_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_tiny_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_tiny_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check disjunction
    assert len(network.disjunctionList) == 1
    assert len(network.disjunctionList[0]) == 1
    assert len(network.disjunctionList[0][0]) == 3
    # TODO: Uncomment next asserts after fixing MarabouUtils.Equation for Disjunction constraint
    # assert network.disjunctionList[0][0][0].addendList == [(-1, 0)] and network.disjunctionList[0][0][0].scalar == 1
    # assert network.disjunctionList[0][0][1].addendList == [(1, 0)] and network.disjunctionList[0][0][1].scalar == 1
    # assert network.disjunctionList[0][0][2].addendList == [(-1, 1)] and network.disjunctionList[0][0][2].scalar == -100

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    # TODO: Uncomment next assert after fixing disjunction constraint bug with input vars in a disjunct
    # assert (exit_code == "sat" and -1 - TOL <= vals[0] <= 1 + TOL and vals[1] >= 100 - TOL) or exit_code == "unsat"

def test_small_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_small_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_small_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 2
    assert "X_0" in network.vnnlibMap and network.vnnlibMap["X_0"] == (0, "Real")
    assert "Y_0" in network.vnnlibMap and network.vnnlibMap["Y_0"] == (1, "Real")

    # Check disjunction
    assert len(network.disjunctionList) == 1
    assert len(network.disjunctionList[0]) == 1
    assert len(network.disjunctionList[0][0]) == 3
    # TODO: Uncomment next asserts after fixing MarabouUtils.Equation for Disjunction constraint
    # assert network.disjunctionList[0][0][0].addendList == [(-1, 0)] and network.disjunctionList[0][0][0].scalar == 1
    # assert network.disjunctionList[0][0][1].addendList == [(1, 0)] and network.disjunctionList[0][0][1].scalar == 1
    # assert network.disjunctionList[0][0][2].addendList == [(-1, 1)] and network.disjunctionList[0][0][2].scalar == -100

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    # TODO: Uncomment next assert after fixing disjunction constraint bug with input vars in a disjunct
    # assert (exit_code == "sat" and -1 - TOL <= vals[0] <= 1 + TOL and vals[1] >= 100 - TOL) or exit_code == "unsat"

def test_sat_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_sat_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_prop_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 10
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == (i, "Real")

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == (i + 5, "Real")

    # Check input bounds:
    assert network.equList[-14].addendList == [(1, 0)] and network.equList[-14].scalar == -0.29855281193475053
    assert network.equList[-13].addendList == [(-1, 0)] and network.equList[-13].scalar == 0.30353115613746867

    assert network.equList[-12].addendList == [(1, 1)] and network.equList[-12].scalar == 0.009549296585513092
    assert network.equList[-11].addendList == [(-1, 1)] and network.equList[-11].scalar == 0.009549296585513092

    assert network.equList[-10].addendList == [(1, 2)] and network.equList[-10].scalar == 0.49999999998567607
    assert network.equList[-9].addendList == [(-1, 2)] and network.equList[-9].scalar == -0.4933803235848431

    assert network.equList[-8].addendList == [(1, 3)] and network.equList[-8].scalar == 0.5
    assert network.equList[-7].addendList == [(-1, 3)] and network.equList[-7].scalar == -0.3

    assert network.equList[-6].addendList == [(1, 4)] and network.equList[-6].scalar == 0.5
    assert network.equList[-5].addendList == [(-1, 4)] and network.equList[-5].scalar == -0.3

    # Check output bound
    assert network.equList[-4].addendList == [(1, 5), (-1, 6)] and network.equList[-4].scalar == 0
    assert network.equList[-3].addendList == [(1, 5), (-1, 7)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(1, 5), (-1, 8)] and network.equList[-2].scalar == 0
    assert network.equList[-1].addendList == [(1, 5), (-1, 9)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert exit_code == "sat" and \
           -0.30353115613746867 - TOL <= vals[0] <= -0.29855281193475053 + TOL and \
           -0.009549296585513092 - TOL <= vals[1] <= 0.009549296585513092 + TOL and \
           0.4933803235848431 - TOL <= vals[2] <= 0.49999999998567607 + TOL and \
           0.3 - TOL <= vals[3] <= 0.5 + TOL and \
           0.3 - TOL <= vals[4] <= 0.5 + TOL and \
           vals[5] <= min([vals[i] for i in range(6, 10)]) + TOL

def test_unsat_vnncomp():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_unsat_vnncomp.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "test_prop_vnncomp.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 10
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == (i, "Real")

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == (i + 5, "Real")

    # Check input bounds:
    assert network.equList[-14].addendList == [(1, 0)] and network.equList[-14].scalar == -0.29855281193475053
    assert network.equList[-13].addendList == [(-1, 0)] and network.equList[-13].scalar == 0.30353115613746867

    assert network.equList[-12].addendList == [(1, 1)] and network.equList[-12].scalar == 0.009549296585513092
    assert network.equList[-11].addendList == [(-1, 1)] and network.equList[-11].scalar == 0.009549296585513092

    assert network.equList[-10].addendList == [(1, 2)] and network.equList[-10].scalar == 0.49999999998567607
    assert network.equList[-9].addendList == [(-1, 2)] and network.equList[-9].scalar == -0.4933803235848431

    assert network.equList[-8].addendList == [(1, 3)] and network.equList[-8].scalar == 0.5
    assert network.equList[-7].addendList == [(-1, 3)] and network.equList[-7].scalar == -0.3

    assert network.equList[-6].addendList == [(1, 4)] and network.equList[-6].scalar == 0.5
    assert network.equList[-5].addendList == [(-1, 4)] and network.equList[-5].scalar == -0.3

    # Check output bound
    assert network.equList[-4].addendList == [(1, 5), (-1, 6)] and network.equList[-4].scalar == 0
    assert network.equList[-3].addendList == [(1, 5), (-1, 7)] and network.equList[-3].scalar == 0
    assert network.equList[-2].addendList == [(1, 5), (-1, 8)] and network.equList[-2].scalar == 0
    assert network.equList[-1].addendList == [(1, 5), (-1, 9)] and network.equList[-1].scalar == 0

    # Check constraints held on Marabou:
    exit_code, vals, _ = network.solve(options=OPT)
    assert exit_code == "unsat"

def test_acasxu():
    filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "acasxu", "ACASXU_experimental_v2a_1_1.onnx")
    vnnlib_filename = os.path.join(os.path.dirname(__file__), NETWORK_FOLDER, "vnnlib", "acasxu_prop1.vnnlib")

    network = Marabou.read_onnx(filename, vnnlibFilename=vnnlib_filename)

    # Check input and output variables in vnnlib map:
    assert len(network.vnnlibMap) == 10
    for i in range(5):
        assert f"X_{i}" in network.vnnlibMap and network.vnnlibMap[f"X_{i}"] == (i, "Real")

    for i in range(5):
        assert f"Y_{i}" in network.vnnlibMap and network.vnnlibMap[f"Y_{i}"] == (i + 5, "Real")

    # Check input bounds:
    assert network.equList[-11].addendList == [(1, 0)] and network.equList[-11].scalar == 0.679857769
    assert network.equList[-10].addendList == [(-1, 0)] and network.equList[-10].scalar == -0.6

    assert network.equList[-9].addendList == [(1, 1)] and network.equList[-9].scalar == 0.5
    assert network.equList[-8].addendList == [(-1, 1)] and network.equList[-8].scalar == 0.5

    assert network.equList[-7].addendList == [(1, 2)] and network.equList[-7].scalar == 0.5
    assert network.equList[-6].addendList == [(-1, 2)] and network.equList[-6].scalar == 0.5

    assert network.equList[-5].addendList == [(1, 3)] and network.equList[-5].scalar == 0.5
    assert network.equList[-4].addendList == [(-1, 3)] and network.equList[-4].scalar == -0.45

    assert network.equList[-3].addendList == [(1, 4)] and network.equList[-3].scalar == -0.45
    assert network.equList[-2].addendList == [(-1, 4)] and network.equList[-2].scalar == 0.5

    # Check output bound
    assert network.equList[-1].addendList == [(-1, 5)] and network.equList[-1].scalar == -3.991125645861615