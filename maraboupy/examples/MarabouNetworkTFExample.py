from maraboupy import Marabou
import numpy as np


# Network corresponds to inputs x0, x1
# Outputs: y0 = |x0| + |x1|, y1 = x0^2 + x1^2
filename = './networks/graph_test_medium.pb'
network = Marabou.read_tf(filename)

## Or, you can specify the operation names of the input and output operations
## By default chooses the only placeholder as input, last op as output
#inputName = 'Placeholder'
#outputName = 'y_out'
#network = MarabouNetworkTF(filename=filename, inputName=inputName, outputName = outputName)

# Get the input and output variable numbers; [0] since first dimension is batch size
inputVars = network.inputVars[0]
outputVars = network.outputVars[0]

# Set input bounds
network.setLowerBound(inputVars[0],-10.0)
network.setUpperBound(inputVars[0], 10.0)
network.setLowerBound(inputVars[1],-10.0)
network.setUpperBound(inputVars[1], 10.0)

# Set output bounds
network.setLowerBound(outputVars[1], 194.0)
network.setUpperBound(outputVars[1], 210.0)

# Call to C++ Marabou solver
vals, stats = network.solve("marabou.log")