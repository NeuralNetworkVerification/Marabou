# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

from maraboupy import Marabou
from maraboupy import MarabouCore
import numpy as np
import os

# Global settings
OPT = Marabou.createOptions(verbosity = 0) # Turn off printing
TOL = 1e-4                                 # Set tolerance for checking Marabou evaluations

# Test adapted from https://github.com/NeuralNetworkVerification/Marabou/issues/607
def test_unit_equation1():
    # x0 = 1
    # Disj(-x0 >= 0)

    inputQuery = MarabouCore.InputQuery()
    inputQuery.setNumberOfVariables(1)

    inputQuery.setLowerBound(0,-1)
    inputQuery.setUpperBound(0,1)

    eq0 = MarabouCore.Equation()
    eq0.addAddend(1,0)
    eq0.setScalar(1)
    inputQuery.addEquation(eq0)

    eq1 = MarabouCore.Equation(MarabouCore.Equation.EquationType.GE)
    eq1.addAddend(-1, 0)
    eq1.setScalar(0)

    MarabouCore.addDisjunctionConstraint(inputQuery, [[eq1]])

    r, _, _ = MarabouCore.solve(inputQuery, OPT, "")
    assert(r == "unsat")

def test_unit_equation2():
    # x0 = 1
    # Disj(x0 <= 0)

    inputQuery = MarabouCore.InputQuery()
    inputQuery.setNumberOfVariables(1)

    inputQuery.setLowerBound(0,-1)
    inputQuery.setUpperBound(0,1)

    eq0 = MarabouCore.Equation()
    eq0.addAddend(1,0)
    eq0.setScalar(1)
    inputQuery.addEquation(eq0)

    eq1 = MarabouCore.Equation(MarabouCore.Equation.EquationType.LE)
    eq1.addAddend(1, 0)
    eq1.setScalar(0)

    MarabouCore.addDisjunctionConstraint(inputQuery, [[eq1]])

    r, _, _ = MarabouCore.solve(inputQuery, OPT, "")
    assert(r == "unsat")

def test_unit_equation3():
    # x0 = 1
    # Disj(-x0 <= 0)
    inputQuery = MarabouCore.InputQuery()
    inputQuery.setNumberOfVariables(1)

    inputQuery.setLowerBound(0,-1)
    inputQuery.setUpperBound(0,1)

    eq0 = MarabouCore.Equation()
    eq0.addAddend(1,0)
    eq0.setScalar(1)
    inputQuery.addEquation(eq0)

    eq1 = MarabouCore.Equation(MarabouCore.Equation.EquationType.LE)
    eq1.addAddend(-1, 0)
    eq1.setScalar(0)

    MarabouCore.addDisjunctionConstraint(inputQuery, [[eq1]])

    r, vals, _ = MarabouCore.solve(inputQuery, OPT, "")
    assert(r == "sat")
    assert(vals[0] == 1)

def test_unit_equation4():
    # x0 = 1
    # Disj(x0 >= 0)
    inputQuery = MarabouCore.InputQuery()
    inputQuery.setNumberOfVariables(1)

    inputQuery.setLowerBound(0,-1)
    inputQuery.setUpperBound(0,1)

    eq0 = MarabouCore.Equation()
    eq0.addAddend(1,0)
    eq0.setScalar(1)
    inputQuery.addEquation(eq0)

    eq1 = MarabouCore.Equation(MarabouCore.Equation.EquationType.GE)
    eq1.addAddend(1, 0)
    eq1.setScalar(0)

    MarabouCore.addDisjunctionConstraint(inputQuery, [[eq1]])

    r, vals, _ = MarabouCore.solve(inputQuery, OPT, "")
    assert(r == "sat")
    assert(vals[0] == 1)
