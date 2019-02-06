from maraboupy import MarabouCore

    #
    #   An example on a small network with ReLU activations
    #
    #             ReLU
    #         x1 ------> x2
    #      1 /             \ 1
    #       /               \
    #  --> x0                x5 -->
    #       \               /
    #     -1 \    ReLU     / 1
    #         x3 ------> x4
    #
    #
    #   Where the input and output constraints are:
    #
    #     0   <= x0 <= 1
    #     1/2 <= x1 <= 1
    #

inputQuery = MarabouCore.InputQuery()

inputQuery.setNumberOfVariables(6)

inputQuery.setLowerBound(0, 0)
inputQuery.setUpperBound(0, 1)

inputQuery.setLowerBound(2, 0)

inputQuery.setLowerBound(4, 0)

inputQuery.setLowerBound(5, 0.5)
inputQuery.setUpperBound(5, 1)

equation1 = MarabouCore.Equation()
equation1.addAddend(1, 0)
equation1.addAddend(-1, 1)
equation1.setScalar(0)
inputQuery.addEquation(equation1)

equation2 = MarabouCore.Equation()
equation2.addAddend(1, 0)
equation2.addAddend(1, 3)
equation2.setScalar(0)
inputQuery.addEquation(equation2)

equation3 = MarabouCore.Equation()
equation3.addAddend(1, 2)
equation3.addAddend(1, 4)
equation3.addAddend(-1, 5)
equation3.setScalar(0)
inputQuery.addEquation(equation3)

MarabouCore.addReluConstraint(inputQuery,1,2)
MarabouCore.addReluConstraint(inputQuery,3,4)

vars1, stats1 = MarabouCore.solve(inputQuery, "", 0)
if len(vars1)>0:
	print("SAT")
	print(vars1)
else:
	print("UNSAT")
