from AcasUtils import *
from scipy.spatial import ConvexHull
from maraboupy import MarabouUtils
from maraboupy import Marabou

print("Finding points to verify...")
collisionList = getCollisionList()

print("Finding convex hull...")
hull = ConvexHull(collisionList)
hyperplanes = hull.equations

### Load network
filename = "./networks/ACASXU_TF12_run3_DS_Minus15_Online_tau0_pra1_200Epochs.pb"
network = Marabou.read_tf(filename)

# first dimension is batch size
inputVars = network.inputVars[0]
outputVars = network.outputVars[0]

# set bounds for inputs
for i in range(len(inputVars)):
    network.setLowerBound(inputVars[i], getInputLowerBound(i))
    network.setUpperBound(inputVars[i], getInputUpperBound(i))

# set constraints according to convex hull
for hyperplane in hyperplanes:
    MarabouUtils.addInequality(network, inputVars, hyperplane[:-1], -hyperplane[-1])

# property to be UNSAT: output[0] is least
for i in range(1, len(outputVars)):
    MarabouUtils.addInequality(network, [outputVars[0], outputVars[i]], [1, -1],0)

print("Solving...")
vals = network.solve("")
if len(vals)==0:
    print("UNSAT")
else:
    print("SAT")
    print("Input Values:")
    for i in range(inputVars.size):
        print("%d: %.4e" % (i, vals[inputVars[i]]))
              
    print("\nOutput Values:")
    for i in range(outputVars.size):
        print("%d: %.4e" % (i, vals[outputVars[i]]))
