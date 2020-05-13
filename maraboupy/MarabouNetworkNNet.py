'''
/* *******************                                                        */
/*! \file MarabouNetworkNNet.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Andrew Wu, Kyle Julian
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

from .MarabouUtils import *
from maraboupy import MarabouCore
from maraboupy import MarabouNetwork
import numpy as np

class MarabouNetworkNNet(MarabouNetwork.MarabouNetwork):
    """
    Class that implements a MarabouNetwork from an NNet file.
    """
    def __init__ (self, filename, use_nlr=False):
        """
        Constructs a MarabouNetworkNNet object from an .nnet file.

        Args:
            filename: path to the .nnet file.
        Attributes:
            numLayers        (int) The number of layers in the network
            layerSizes       (list of ints) Layer sizes.
            inputSize        (int) Size of the input.
            outputSize       (int) Size of the output.
            maxLayersize     (int) Size of largest layer.
            inputMinimums    (list of floats) Minimum value for each input.
            inputMaximums    (list of floats) Maximum value for each input.
            inputMeans       (list of floats) Mean value for each input.
            inputRanges      (list of floats) Range for each input
            weights          (list of list of lists) Outer index corresponds to layer
                                number.
            biases           (list of lists) Outer index corresponds to layer number.
            nlr              The NetworkLeverReasoner object
        """
        super().__init__()

        # read the file and load values
        self.read_nnet(filename)

        # compute variable ranges
        self.variableRanges()

        # identify variables involved in relu constraints
        relus = self.findRelus()

        # build all equations that govern the network
        equations = self.buildEquations()
        for equation in equations:
            e = Equation()
            for term in equation[:-1]:
                e.addAddend(term[1], term[0])
            e.setScalar(equation[-1])
            self.addEquation(e)


        # add all the relu constraints
        for relu in relus:
            self.addRelu(relu[0], relu[1])

        # Set all the bounds defined in the .nnet file

        # Set input variable bounds
        for i, i_var in enumerate(self.inputVars[0]):
            self.setLowerBound(i_var, self.getInputMinimum(i_var))
            self.setUpperBound(i_var, self.getInputMaximum(i_var))

        # Set bounds for forward facing variables
        for f_var in self.f_variables:
            self.setLowerBound(f_var, 0.0)

        # Set the number of variables
        self.numVars = self.numberOfVariables()
        if use_nlr:
            self.nlr = self.createNLR(filename)
        else:
            self.nlr = None

    def createNLR(self, filename):
        nlr = MarabouCore.NetworkLevelReasoner()
        nlr.setNumberOfLayers(self.numLayers + 1)
        for layer, size in enumerate(self.layerSizes):
            nlr.setLayerSize(layer, size)
        nlr.allocateMemoryByTopology()
        # Biases
        for layer in range(len(self.biases)):
            for node in range(len(self.biases[layer])):
                nlr.setBias(layer + 1, node, self.biases[layer][node])
        # Weights
        for layer in range(len(self.weights)): # starting from the first hidden layer
            for target in range(len(self.weights[layer])):
                for source in range(len(self.weights[layer][target])):
                    nlr.setWeight(layer, source, target, self.weights[layer][target][source])

        # Activation Functions
        RELU = MarabouCore.NetworkLevelReasoner.ReLU;
        for layer in range(len(self.weights) - 1): # only hidden layers
            for neuron in range(len(self.weights[layer])):
                nlr.setNeuronActivationFunction(layer + 1, neuron, RELU );

        # Variable indexing
        for layer in range(len(self.layerSizes))[1:-1]:
            for node in range(self.layerSizes[layer]):
                nlr.setWeightedSumVariable(layer, node, self.nodeTo_b(layer, node))
                nlr.setActivationResultVariable(layer, node, self.nodeTo_f(layer, node))

        for node in range(self.inputSize):
            nlr.setActivationResultVariable(0, node, self.nodeTo_f(0, node))

        for node in range(self.outputSize):
            nlr.setWeightedSumVariable(len(self.layerSizes) - 1, node, self.nodeTo_b(len(self.layerSizes) - 1, node))

        return nlr

    def getMarabouQuery(self):
        ipq = super(MarabouNetworkNNet, self).getMarabouQuery()
        ipq.setNetworkLevelReasoner(self.nlr)
        return ipq

    """
    Read the nnet file, load all the values and assign the class members.

    Args:
        filename: path to the .nnet file.
    """
    def read_nnet(self, file_name):
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

            self.numLayers = numLayers
            self.layerSizes = layerSizes
            self.inputSize = inputSize
            self.outputSize = outputSize
            self.maxLayersize = maxLayersize
            self.inputMinimums = inputMinimums
            self.inputMaximums = inputMaximums
            self.inputMeans = inputMeans
            self.inputRanges = inputRanges
            self.weights = weights
            self.biases = biases

    """
    Compute the variable number ranges for each type (b, f)

    Args:
        None
    """
    def variableRanges(self):
        input_variables = []
        b_variables = []
        f_variables = []
        output_variables = []

        input_variables = [i for i in range(self.layerSizes[0])]

        hidden_layers = self.layerSizes[1:-1]

        for layer, hidden_layer_length in enumerate(hidden_layers):
            for i in range(hidden_layer_length):
                offset = sum([x*2 for x in hidden_layers[:layer]])

                b_variables.append(self.layerSizes[0] + offset + i)
                f_variables.append(self.layerSizes[0] + offset + i+hidden_layer_length)

        #final layer
        for i in range(self.layerSizes[-1]):
            offset = sum([x*2 for x in hidden_layers[:len(hidden_layers) - 1]])
            output_variables.append(self.layerSizes[0] + offset + i + 2*hidden_layers[-1])

        self.inputVars = np.array([input_variables])
        self.b_variables = b_variables
        self.f_variables = f_variables
        self.outputVars = np.array([output_variables])

    """
    Compute the variable number for the b variables in that correspond to the
        layer, node argument.

    Args:
        layer: (int) layer number.
        node: (int) node number.
    Returns:
        variable number: (int) variable number that corresponds to the b variable
        of the node defined by the layer, node indices.
    """
    def nodeTo_b(self, layer, node):
        assert(0 < layer)
        assert(node < self.layerSizes[layer])

        offset = self.layerSizes[0]
        offset += sum([x*2 for x in self.layerSizes[1:layer]])

        return offset + node

    """
    Compute the variable number for the f variables in that correspond to the
        layer, node argument.

    Args:
        layer: (int) layer number.
        node: (int) node number.
    Returns:
        variable number: (int) variable number that corresponds to the f variable
        of the node defined by the layer, node indices.
    """
    def nodeTo_f(self, layer, node):
        assert(layer < len(self.layerSizes))
        assert(node < self.layerSizes[layer])

        if layer == 0:
            return node
        else:
            offset = self.layerSizes[0]
            offset += sum([x*2 for x in self.layerSizes[1:layer]])
            offset += self.layerSizes[layer]

            return offset + node
    """
    Constructs the equation representation from the class members
    Arguments:
        None
    Returns:
        equations_aux   (list of lists) that represents all the equations in
            the network.
    """
    def buildEquations(self):
        equations_aux = []
        equations_count = 0
        marabou_equations = []

        for layer, size in enumerate(self.layerSizes):
            if layer == 0:
                continue

            for node in range(size):
                #add marabou equation

                equations_aux.append([])
                equations_aux[equations_count].append([self.nodeTo_b(layer, node), -1.0])
                for previous_node in range(self.layerSizes[layer-1]):
                    equations_aux[equations_count].append([self.nodeTo_f(layer-1, previous_node), self.weights[layer-1][node][previous_node]])


                equations_aux[equations_count].append(-self.biases[layer-1][node])
                equations_count += 1

        return equations_aux
    """
    Identify all relus and their associated variable numbers.
    Arguments:
        None
    Returns:
        relus   (list of lists) that represents all the relus in
            the network.
    """
    def findRelus(self):
        relus = []
        hidden_layers = self.layerSizes[1:-1]
        for layer, size in enumerate(hidden_layers):
            for node in range(size):
                relus.append([self.nodeTo_b(layer+1, node), self.nodeTo_f(layer+1, node)])

        return relus

    def numberOfVariables(self):
        return self.layerSizes[0] + 2*sum(self.layerSizes[1:-1]) + 1*self.layerSizes[-1]

    def getInputMinimum(self, input):
        return (self.inputMinimums[input] - self.inputMeans[input]) / self.inputRanges[input]

    def getInputMaximum(self, input):
        return (self.inputMaximums[input] - self.inputMeans[input]) / self.inputRanges[input]
