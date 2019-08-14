'''
/* *******************                                                        */
/*! \file multi_input_test.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Chelsea Sidrane
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief [[ Add one-line brief description here ]]
 **
 ** [[ Add lengthier description here ]]
 **/
'''

# a simple keras NN where there are multiple inputs
import os
import numpy as np 
import tensorflow as tf 
from tensorflow.keras.layers import Input, Dense, Concatenate, Activation
from tensorflow.keras.models import Model 
from tensorflow.python.framework import graph_util
from maraboupy import Marabou
import argparse

##### command-line interface
parser = argparse.ArgumentParser("Test Maraboupy support for multiple inputs and the concatenate operator.")
parser.add_argument('log_dir', metavar='log_dir', type=str, nargs=1, help='directory for writing frozen graph and marabou log')
args = parser.parse_args()
log_dir = args.log_dir[0]

###### BUILD THE MODEL
hidden_dim = 4
obs_dim = 3
action_dim = 2
layer_dim = 2

hiddens = Input(shape=(hidden_dim,))
obs = Input(shape=(obs_dim,))

Wh = Dense(layer_dim)(hiddens)
Wo = Dense(layer_dim)(obs)

interm1 = Concatenate(axis=-1)([Wh, Wo])

interm2 = Activation('relu')(interm1)

interm3 = Dense(action_dim)(interm2)

action_weights = Activation('relu', name="output")(interm3)

model = Model(inputs=[hiddens, obs], outputs=action_weights)

############# SAVE THE MODEL
#log_dir = "/home/csidrane/Data/AAHAA/RNNs/run_1_test"
os.mkdir(log_dir)

sess = tf.Session()
with sess.as_default():
	sess.run(tf.global_variables_initializer()) 
	[print(n.name) for n in tf.get_default_graph().as_graph_def().node]
	model.summary()

print("initialized graph")

# prep graph for outputting
output_graph_name = os.path.join(log_dir, "frozen_model.pb")
output_node_names = "output/Relu"
input_graph_def = tf.get_default_graph().as_graph_def()
output_graph_def = graph_util.convert_variables_to_constants(
	sess, # sess used to retrieve weights
	input_graph_def, # graph def used to retrieve nodes
	output_node_names.split(",") # output node names used to select useful nodes
	)

# serialize and dump the output graph to file
with tf.gfile.GFile(output_graph_name, "w") as f:
	f.write(output_graph_def.SerializeToString())
print("%d ops in the final graph." % len(output_graph_def.node))

# record the unique ops used in the graph
op_file = open(os.path.join(log_dir, "ops_in_model.txt"), 'w')
op_set = {(x.op,) for x in output_graph_def.node}
for op in op_set:
	op_file.write(str(op[0])+"\n")
op_file.close()

sess.close()

##### LOAD THE MODEL INTO MARABOU
filename = os.path.join(log_dir, 'frozen_model.pb')

network = Marabou.read_tf(filename, outputName="output/Relu")

inputVars = network.inputVars
outputVars = network.outputVars
print("network.inputVars: ", network.inputVars)

# Set input bounds
for inputV in inputVars:
	print("inputV: ", inputV)
	for i in range(len(inputV[0])):
		print("setting input bounds for: ", inputV[0][i])
		network.setLowerBound(inputV[0][i], 0.0)
		network.setUpperBound(inputV[0][i], 10.0)

# Set output bounds
for i in range(len(outputVars[0])):
	print("setting output bounds for: ", outputVars[0][i])
	network.setLowerBound(outputVars[0][i], -10.0)
	network.setUpperBound(outputVars[0][i], 10.0)

# display equations
print("Equations: ", [e.getParticipatingVariables() for e in network.equList])

##### USE MARABOU TO SOLVE QUERY
# Call to C++ Marabou solver 
vals, stats = network.solve(os.path.join(log_dir,'marabou.log'))
print("vals: ", vals)
print("stats: ", stats)

# run eval code just to make sure it works
hid_test = np.array([vals[0], vals[1], vals[2], vals[3]])
obs_test = np.array([vals[4], vals[5], vals[6]])
outputvals1 = network.evaluate([hid_test, obs_test], useMarabou=False)

outputvals2 = network.evaluate([hid_test, obs_test], useMarabou=True)

print("outputvals1: ", outputvals1)

print("outputvals2: ", outputvals2)

assert all(outputvals1 == outputvals2[0])