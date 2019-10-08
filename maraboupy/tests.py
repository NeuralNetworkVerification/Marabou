from maraboupy import MarabouCore
import pytest

large = 100
def define_network():
    network = MarabouCore.InputQuery()
    network.setNumberOfVariables(3)

    # x
    network.setLowerBound(0, -1)
    network.setUpperBound(0, 1)

    network.setLowerBound(1, 1)
    network.setUpperBound(1, 2)

    # y
    network.setLowerBound(2, -large)
    network.setUpperBound(2, large)

    MarabouCore.addReluConstraint(network, 0, 1)

    # y - relu(x) >= 0
    output_equation = MarabouCore.Equation()
    output_equation.addAddend(1, 2)
    output_equation.addAddend(-1, 1)
    output_equation.setScalar(0)
    # output_equation.dump()
    network.addEquation(output_equation)

    # y <= n * 0.01
    property_eq = MarabouCore.Equation(MarabouCore.Equation.LE)
    property_eq.addAddend(1, 1)
    property_eq.setScalar(3)

    return network

def test_dump_query():
    network = define_network()
    MarabouCore.solve(network, "", 0)
    network.dump()


if __name__ == "__main__":
    test_dump_query()
