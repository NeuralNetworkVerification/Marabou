# Supress warnings caused by tensorflow
import warnings
warnings.filterwarnings('ignore', category = DeprecationWarning)
warnings.filterwarnings('ignore', category = PendingDeprecationWarning)

import pytest
from maraboupy import MarabouCore
from maraboupy.Marabou import createOptions

# Global settings
OPT = createOptions(verbosity = 0) # Turn off printing
LARGE = 100.0                      # Large value for defining bounds

def test_solve_partial_arguments():
    """
    This function tests that MarabouCore.solve can be called with partial arguments, 
    and checks that an UNSAT query is solved correctly.
    """
    ipq = define_ipq(-2.0)
    # Test partial arguments to solve
    vals, stats = MarabouCore.solve(ipq, OPT)
    # Assert that Marabou returned UNSAT
    assert not stats.hasTimedOut()
    assert len(vals) == 0

def test_dump_query():
    """
    This function tests that MarabouCore.solve can be called with all arguments and
    checks that a SAT query is solved correctly. This also tests the InputQuery dump() method
    as well as bound tightening during solving.
    """
    ipq = define_ipq(3.0)

    # An upper bound for variable 2 was not given, so Marabou uses float max, which
    # is much larger than LARGE
    assert ipq.getUpperBound(2) > LARGE

    # Solve
    vals, stats = MarabouCore.solve(ipq, OPT, "")

    # Test dump
    ipq.dump()

    # Marabou should return SAT values, and the dictionary of values should
    # satisfy all upper and lower bounds
    assert not stats.hasTimedOut()
    assert len(vals) > 0
    for var in vals:
        assert vals[var] >= ipq.getLowerBound(var)
        assert vals[var] <= ipq.getUpperBound(var)

    # Marabou should find tighter bounds than LARGE after bound propagation, including
    # for variable 2, where no upper bound was explicitly given
    assert ipq.getUpperBound(1) < LARGE
    assert ipq.getLowerBound(2) > -LARGE
    assert ipq.getUpperBound(2) < LARGE

def define_ipq(property_bound):
    """
    This function defines a simple input query directly through MarabouCore
    Arguments:
        property_bound: (float) value of upper bound for x + y
    Returns:
        ipq (MarabouCore.InputQuery) input query object representing network and constraints
    """
    ipq = MarabouCore.InputQuery()
    ipq.setNumberOfVariables(3)

    # x
    ipq.setLowerBound(0, -1)
    ipq.setUpperBound(0, 1)

    # relu(x)
    ipq.setLowerBound(1, 0)
    ipq.setUpperBound(1, LARGE)

    # y
    ipq.setLowerBound(2, -LARGE)
    # if an upper/lower bound is not supplied to Marabou, Marabou uses float min/max

    MarabouCore.addReluConstraint(ipq, 0, 1)

    # y - relu(x) = 0
    output_equation = MarabouCore.Equation()
    output_equation.addAddend(1, 2)
    output_equation.addAddend(-1, 1)
    output_equation.setScalar(0)
    ipq.addEquation(output_equation)

    # x + y <= property_bound
    property_eq = MarabouCore.Equation(MarabouCore.Equation.LE)
    property_eq.addAddend(1, 0)
    property_eq.addAddend(1, 2)
    property_eq.setScalar(property_bound)
    ipq.addEquation(property_eq)
    return ipq
