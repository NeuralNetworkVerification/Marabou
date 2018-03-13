import numpy as np
from maraboupy import MarabouCore

def read_nnet(file_name):
    with open(file_name) as f:
        line = f.readline()
        cnt = 1
        while line[0:2] == "//":
            line=f.readline() 
            cnt+= 1
        #numLayers does't include the input layer!
        numLayers, inputSize, outputSize, maxLayersize = [int(x) for x in line.strip().split(",")[:-1]]
        line=f.readline()

        #input layer size, layer1size, layer2size...
        layerSizes = [int(x) for x in line.strip().split(",")[:-1]]

        line=f.readline()
        symmetric = int(line.strip().split(",")[0])

        line = f.readline()
        inputMinimums = [float(x) for x in line.strip().split(",")[:-1]]

        line = f.readline()
        inputMaximums = [float(x) for x in line.strip().split(",")[:-1]]

        line = f.readline()
        inputMeans = [float(x) for x in line.strip().split(",")[:-1]]

        line = f.readline()
        inputRanges = [float(x) for x in line.strip().split(",")[:-1]]

        weights=[]
        biases = []
        for layernum in range(numLayers):

            previousLayerSize = layerSizes[layernum]
            currentLayerSize = layerSizes[layernum+1]
            # weights
            weights.append([])
            biases.append([])
            #weights
            for i in range(currentLayerSize):
                line=f.readline()
                aux = [float(x) for x in line.strip().split(",")[:-1]]
                weights[layernum].append([])
                for j in range(previousLayerSize):
                    weights[layernum][i].append(aux[j])
            #biases
            for i in range(currentLayerSize):
                line=f.readline()
                x = float(line.strip().split(",")[0])
                biases[layernum].append(x)
        return numLayers, layerSizes, inputSize, outputSize, maxLayersize, inputMinimums, inputMaximums, inputMeans, inputRanges, weights, biases

def variableRanges(layerSizes):
    input_variables = []
    b_variables = []
    f_variables = []
    aux_variables = []
    output_variables = []
    
    input_variables = [i for i in range(layerSizes[0])]
    
    hidden_layers = layerSizes[1:-1]
    
    for layer, hidden_layer_length in enumerate(hidden_layers):
        for i in range(hidden_layer_length):
            offset = sum([x*3 for x in hidden_layers[:layer]])
            
            b_variables.append(layerSizes[0] + offset + i)
            aux_variables.append(layerSizes[0] + offset + i+hidden_layer_length)
            f_variables.append(layerSizes[0] + offset + i+2*hidden_layer_length)
    
    #final layer
    for i in range(layerSizes[-1]):
        offset = sum([x*3 for x in hidden_layers[:len(hidden_layers) - 1]])
        output_variables.append(layerSizes[0] + offset + i + 3*hidden_layers[-1])
        aux_variables.append(layerSizes[0] + offset + i + 3*hidden_layers[-1] + layerSizes[-1])
    return input_variables, b_variables, f_variables, aux_variables, output_variables


def nodeTo_b(layer, node):
    assert(0 < layer)
    assert(node < layerSizes[layer])
    
    offset = layerSizes[0]
    offset += sum([x*3 for x in layerSizes[1:layer]])
    
    return offset + node

def nodeTo_aux(layer, node):
    assert(0 < layer)
    assert(node < layerSizes[layer])
    
    offset = layerSizes[0]
    offset += sum([x*3 for x in layerSizes[1:layer]])
    offset += layerSizes[layer]
    
    return offset + node

def nodeTo_f(layer, node):
    assert(layer < len(layerSizes))
    assert(node < layerSizes[layer])
    
    if layer == 0:
        return node
    else:
        offset = layerSizes[0]
        offset += sum([x*3 for x in layerSizes[1:layer]])
        offset += 2*layerSizes[layer]

        return offset + node

def buildEquations(layerSizes):
    equations_aux = []
    equations_count = 0
    marabou_equations = []

    for layer, size in enumerate(layerSizes):
        if layer == 0:
            continue

        for node in range(size):
            #add marabou equation

            equation = MarabouCore.Equation()
            equations_aux.append([])

            #equations_aux[]
            for previous_node in range(layerSizes[layer-1]):
                equation.addAddend(weights[layer-1][node][previous_node], nodeTo_f(layer-1, previous_node))
                equations_aux[equations_count].append([nodeTo_f(layer-1, previous_node), weights[layer-1][node][previous_node]])
            
            equation.addAddend(1.0, nodeTo_aux(layer, node))
            equation.markAuxiliaryVariable(nodeTo_aux(layer, node))
            equations_aux[equations_count].append([nodeTo_aux(layer, node), 1.0])
            
            equation.addAddend(-1.0, nodeTo_b(layer, node))
            equations_aux[equations_count].append([nodeTo_b(layer, node), -1.0])
            
            equation.setScalar(-biases[layer-1][node])
            equations_aux[equations_count].append(-biases[layer-1][node])
            equations_count += 1
            
            marabou_equations.append(equation)
            
    return marabou_equations

def findRelus(layerSizes):
    relus = []
    hidden_layers = layerSizes[1:-1]
    for layer, size in enumerate(hidden_layers):
        for node in range(size):
            relus.append([nodeTo_b(layer+1, node), nodeTo_f(layer+1, node)])
            
    return relus

def numberOfVariables(layerSizes):
    return layerSizes[0] + 3*sum(layerSizes[1:-1]) + 2*layerSizes[-1]

def getInputMinimum(input):
    return (inputMinimums[input] - inputMeans[input]) / inputRanges[input]

def getInputMaximum(input):
    return (inputMaximums[input] - inputMeans[input]) / inputRanges[input]

# 5-5-5
#file_name = ../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny.nnet"

#5-50-5
file_name = "../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"

#5-50-50-5
#file_name = "../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_3.nnet"


#5-50-50-50-5, 150 Relus
#file_name = "../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_4.nnet"

numLayers, layerSizes, inputSize,\
 outputSize, maxLayersize, inputMinimums,\
  inputMaximums, inputMeans, inputRanges,\
   weights, biases =read_nnet(file_name)

print(layerSizes)

equation_objects = buildEquations(layerSizes) 

input_variables, b_variables, f_variables,\
 aux_variables, output_variables = variableRanges(layerSizes)

relus = findRelus(layerSizes)

ipq = MarabouCore.InputQuery()

#add the equations

#print("EQUATION: {}".format(len(equation_objects)))


for eq in equation_objects:
    ipq.addEquation(eq)

#mark aux vars


#add the relus
for relu in relus:
    ipq.addReluConstraint(relu[0], relu[1])

#add the network bounds

#input minimums and maximums
print("INPUT VARS: {}".format(input_variables))

for i, i_var in enumerate(input_variables):
    ipq.setLowerBound(i_var, getInputMinimum(i_var))
    print("INPUT MINIMUMS: {}".format(getInputMinimum(i_var)))
    ipq.setUpperBound(i_var, getInputMaximum(i_var))
    print("INPUT MAXIMUMS: {}".format(getInputMaximum(i_var)))

#aux variables
for aux_var in aux_variables:
    ipq.setLowerBound(aux_var, 0.0)
    ipq.setUpperBound(aux_var, 0.0)


for f_var in f_variables:
    ipq.setLowerBound(f_var, 0.0)


#add the property boudns

#ipq.setLowerBound( output_variables[0], 0.5 );
#print("OUTPUT VARIABLE FOR BOUND: {}".format(output_variables[0]))

#CHECK THE EVAL

input0 = -0.328422874212265
input1 = 0.40932923555374146
input2 = -0.017379289492964745
input3 = -0.2747684121131897
input4 = -0.30628132820129395

output0 = 0.5
output1 = -0.18876336514949799
output2 = 0.8081545233726501
output3 = -2.764213800430298
output4 = -0.12992984056472778

ipq.setLowerBound(input_variables[0], input0)
ipq.setUpperBound(input_variables[0], input0)

ipq.setLowerBound(input_variables[1], input1)
ipq.setUpperBound(input_variables[1], input1)

ipq.setLowerBound(input_variables[2], input2)
ipq.setUpperBound(input_variables[2], input2)

ipq.setLowerBound(input_variables[3], input3)
ipq.setUpperBound(input_variables[3], input3)

ipq.setLowerBound(input_variables[4], input4)
ipq.setUpperBound(input_variables[4], input4)


#Input[0] = -0.328422877151060
#Input[1] = 0.413358209042742
#Input[2] = -0.013607140388665
#Input[3] = -0.304152412046139
#Input[4] = -0.300504199489204


input0 = -0.328422874212265
input1 = 0.40932923555374146
input2 = -0.017379289492964745
input3 = -0.2747684121131897
input4 = -0.30628132820129395

output0 = 0.5
output1 = -0.18876336514949799
output2 = 0.8081545233726501
output3 = -2.764213800430298
output4 = -0.12992984056472778

numvars = numberOfVariables(layerSizes)

ipq.setNumberOfVariables(numvars)

vals = MarabouCore.solve(ipq, "dump")
if len(vals)==0:
    print("UNSAT")
else:
    print("SAT")
    print(vals)

for i, var in enumerate(input_variables):
    print("input" + str(i) + " = {}".format(vals[var]) )

for i, var in enumerate(output_variables):
    print("output" + str(i) + " = {}".format(vals[var]) )

print("DIFF0 = {}".format(output0 - vals[output_variables[0]]))
print("DIFF1 = {}".format(output1 - vals[output_variables[1]]))
print("DIFF2 = {}".format(output2 - vals[output_variables[2]]))
print("DIFF3 = {}".format(output3 - vals[output_variables[3]]))
print("DIFF4 = {}".format(output4 - vals[output_variables[4]]))

error = np.array([ output0 - vals[output_variables[0]],output1 - vals[output_variables[1]], output2 - vals[output_variables[2]], output3 - vals[output_variables[3]], output4 - vals[output_variables[4]],  ])

print("NORM OF ERROR: {}".format(np.linalg.norm(error)))
