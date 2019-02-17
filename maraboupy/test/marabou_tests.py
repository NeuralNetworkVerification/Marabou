# tests
from test_utils import *
import tensorflow as tf 
import numpy as np
from maraboupy import Marabou
from maraboupy.MarabouUtils import *

# test 1: test validity interface to maraboupy
# read super simple network
# set bounds using validity interface
# run marabou, output to console
n_id = "22"
fprefix = "../examples/networks/"
frozen_graph = os.path.join(fprefix, "simple_test_network_"+n_id+".pb")

def display_equations(network):
	zipped_eqs = zip([e.addendList for e in network.equList], [e.scalar for e in network.equList])
	print("Equations: ",[z for z in zipped_eqs])
	if len(network.maxList)>0:
		print("Max List: ", network.maxList)

# test 1: something that should return SAT (property does not hold)
def test1(frozen_graph):
	print("Begin test 1 ______________________________________")
	network = Marabou.read_tf(frozen_graph, outputName="matmul")
	# print out equations to check
	display_equations(network)
	output_vars = network.outputVars
	# input bounds
	network.setLowerBound(0, 0.) 
	network.setUpperBound(0, 1.)
	# output bounds
	for i in range(4):
		network.setUpperBound(output_vars[0][i], 4.)
		print("setting var ",output_vars[0][i]," to have an upper bound of ", 4.)

	# display bounds
	print("Upper bounds:", network.upperBounds)
	print("Lower bounds:", network.lowerBounds)

	# This should yield sat.
	# ( because it's impossible. You can't start in the region [0.,1.]
	# and then expect to end in the region [4,5])
	vals, stats = network.solve()
	return vals, stats

# test 2: something that should return UNSAT (property does not hold)
def test2(frozen_graph):
	print("Begin test 2 ______________________________________")
	network = Marabou.read_tf(frozen_graph, outputName="matmul")
	# print out equations to check
	display_equations(network)
	output_vars = network.outputVars
	# input bounds
	network.setLowerBound(0, 0.) 
	network.setUpperBound(0, 1.)
	# output bounds
	for i in range(4):
		network.setLowerBound(output_vars[0][i], 5.)
		print("setting var ",output_vars[0][i]," to have a lower bound of ", 5.)

	# display bounds
	print("Upper bounds:", network.upperBounds)
	print("Lower bounds:", network.lowerBounds)

	# should yield UNSAT
	vals, stats = network.solve()
	return vals, stats

# 1 input, 4 outputs, all identical to input 
def test3(frozen_graph):
	print("Begin test 3 ______________________________________")
	network = Marabou.read_tf(frozen_graph, outputName="matmul")
	display_equations(network)
	output_vars = network.outputVars
	print("original output vars:", output_vars)
	# input bounds
	network.setLowerBound(0, 0.)
	network.setUpperBound(0, 1.)
	# output bounds
	network.outputVars = None
	for i in range(4):
		addComplementOutputSet(network, LB=4., UB=5., x=output_vars[0][i])
	display_equations(network)
	# display bounds
	print("Upper bounds:", network.upperBounds)
	print("Lower bounds:", network.lowerBounds)
	# This should yield sat.
	# ( because it's impossible. You can't start in the region [0.,1.]
	# and then expect to end in the region [4,5])
	vals, stats = network.solve()
	return vals, stats

# 1 input, 4 outputs, all identical to input 
def test4(frozen_graph):
	print("Begin test 4 ______________________________________")
	network = Marabou.read_tf(frozen_graph, outputName="matmul")
	output_vars = network.outputVars
	# input bounds
	eps = 1e-5
	network.setLowerBound(0, 0.+eps)
	network.setUpperBound(0, 1.-eps)
	# output bounds
	network.outputVars = None
	for i in range(4):
		addComplementOutputSet(network, LB=0., UB=1., x=output_vars[0][i])
	display_equations(network)
	# display bounds
	print("Upper bounds:", network.upperBounds)
	print("Lower bounds:", network.lowerBounds)
	# This should yield unsat because it's true. It's in fact confined to a smaller region than [0,1], it's confined to 
	# [eps, 1-eps]
	vals, stats = network.solve()
	return vals, stats

# test 1: segmentation fault sometimes, but not all the time
# run a few times
# test lower bound
vals, stats = test1(frozen_graph) # comment to see the other tests run

# test 2: works fine
# test upper bound
vals, stats = test2(frozen_graph)
print("vals: ", vals)
print("stats:", stats)

# test 3: complement output set, test bounds together
# the SAT values chosen violate the input bounds and the equations. see value assigned to v0, v1, v2, v3, v4
vals, stats = test3(frozen_graph)
print("vals: ", vals)
print("stats:", stats)

# test complement output set
# should be UNSAT, but never finishes preprocessing and I have to kill-9 the process. Seems like a numerical precision issue because it works with eps=1e-4
vals, stats = test4(frozen_graph)
print("vals: ", vals)
print("stats:", stats)

