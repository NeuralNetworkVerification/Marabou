# tests for add_complement_output_sets

import numpy as np
from maraboupy import Marabou
from maraboupy.MarabouUtils import *
from maraboupy.MarabouNetwork import MarabouNetwork
import os

def make_network():
	# construct a simple network where there is just 1 matrix multiply
	network = MarabouNetwork()
	X = network.getNewVariable() # input var
	outputs = [network.getNewVariable() for i in range(4)] # output vars
	for i in range(4):
		e = Equation()
		# c*X + -1.0*v = 0  => v = c*X 
		e.addAddend(i, X)
		e.addAddend(-1.0, outputs[i])
		e.setScalar(0.0)
		network.addEquation(e)
	# coeffs will be 0,1,2,3
	return network, X, outputs

def test_1():
	network, X, outputs = make_network()
	# set input bounds
	network.setLowerBound(X, -1.0)
	network.setUpperBound(X, 1.0)
	# outputs will be [0,0], [-1.0,1.0], [-2.0, 2.0], [-3.0, 3.0]
	#
	# set complement output bounds that should all be UNSAT at same time
	tup_list = []
	eps = 1e-2
	for i in range(4):
		tup_list.append((-i-eps, i+eps, outputs[i]))
	addComplementOutputSets(network, tup_list)
	#
	# should generate: UNSAT
	#
	# solve!
	vals, stats, exitcode = network.solve()
	print(exitcode)
	print(vals)
	print(stats)
	return exitcode

def test_2():
	network, X, outputs = make_network()
	# set input bounds
	network.setLowerBound(X, -1.0)
	network.setUpperBound(X, 1.0)
	# outputs will be [0,0], [-1.0,1.0], [-2.0, 2.0], [-3.0, 3.0]
	#
	# set complement output bounds where 3/4 should be unsat but the 4th should be satisfiable and the overall result should be SAT
	tup_list = []
	eps = 1e-2
	for i in range(4):
		if i == 3:
			# we will definitely violate this bound and leave the interval [-0.5, 0.5] because if the input is allowed to vary over [-1, 1] this output should vary between [-3, 3]
			tup_list.append((-0.5, 0.5, outputs[i]))
		else:
			tup_list.append((-i-eps, i+eps, outputs[i]))
	addComplementOutputSets(network, tup_list)
	#
	# should generate: SAT
	#
	# solve!
	vals, stats, exitcode = network.solve()
	print(exitcode)
	print(vals)
	print(stats) 
	return exitcode

assert test_1() == 0 # code for UNSAT
assert test_2() == 1 # code for SAT
print("Tests pass!")






