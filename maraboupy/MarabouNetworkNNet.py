''''''
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
    """Constructs a MarabouNetworkNNet object from an .nnet file.

        Args:
            filename: path to the .nnet file.
            normalize: (bool) True if network parameters should be adjusted to incorporate
                          network input/output normalization. Otherwise, properties must be written
                          with the normalization already incorporated.
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
            outputMean       (float) Mean value of outputs
            outputRange      (float) Range of output values
            weights          (list of list of lists) Outer index corresponds to layer
                                number.
            biases           (list of lists) Outer index corresponds to layer number.
        """
    def __init__ (self, filename, normalize=False):
        """Read nnet file and extract network equations and constraints
        """
        super().__init__()

        self.normalize = normalize
        # Read the file and load values
        self.read_nnet(filename)

        # Add equations that govern the network
        self.buildEquations()

        # Add variables involved in relu constraints
        self.addRelus()

        # Compute variable ranges
        self.variableRanges()

        # Set input variable bounds
        for i, i_var in enumerate(self.inputVars[0]):
            self.setLowerBound(i_var, self.getInputMinimum(i_var))
            self.setUpperBound(i_var, self.getInputMaximum(i_var))

        # Set bounds for forward facing variables
        for f_var in self.f_variables:
            self.setLowerBound(f_var, 0.0)

        # Set the number of variables
        self.numVars = self.numberOfVariables()

    def read_nnet(self, file_name):
        """
        Read the nnet file, load all the values and assign the class members.

        Args:
            filename: path to the .nnet file.
            
        :meta private:
        """
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
            means = [float(x) for x in line.strip().split(",")[:-1]]

            line = f.readline()
            ranges = [float(x) for x in line.strip().split(",")[:-1]]

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
            self.inputMeans = means[:-1]
            self.inputRanges = ranges[:-1]
            self.outputMean = means[-1]
            self.outputRange = ranges[-1]
            self.weights = weights
            self.biases = biases

            # Convert input bounds from their original values to normalized values if normalization is not being incorporated
            if not self.normalize:
                for i in range(self.inputSize):
                    self.inputMinimums[i] = (self.inputMinimums[i] - self.inputMeans[i]) / self.inputRanges[i]
                    self.inputMaximums[i] = (self.inputMaximums[i] - self.inputMeans[i]) / self.inputRanges[i]

    def variableRanges(self):
        """Compute the variable number ranges for each type (b, f)
        
        :meta private:
        """
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

    def nodeTo_b(self, layer, node):
        """Compute variable number for the b variable of a node in a layer

        Args:
            layer (int): Layer number
            node (int): Node number.

        Returns:
            (int)
        
        :meta private:
        """
        assert(0 < layer)
        assert(node < self.layerSizes[layer])

        offset = self.layerSizes[0]
        offset += sum([x*2 for x in self.layerSizes[1:layer]])

        return offset + node

        
    def nodeTo_f(self, layer, node):
        """Compute variable number for the f variable of a node in a layer

        Args:
            layer (int): Layer number
            node (int): Node number
            
        Returns:
            (int)
        
        :meta private:
        """
        assert(layer < len(self.layerSizes))
        assert(node < self.layerSizes[layer])

        if layer == 0:
            return node
        else:
            offset = self.layerSizes[0]
            offset += sum([x*2 for x in self.layerSizes[1:layer]])
            offset += self.layerSizes[layer]

            return offset + node

    def buildEquations(self):
        """Construct the Marabou equations
        
        :meta private:
        """
        for layer, size in enumerate(self.layerSizes):
            if layer == 0:
                continue

            for node in range(size):
                bias = self.biases[layer-1][node]

                # Add marabou equation and add addend for output variable
                e = Equation()
                e.addAddend(-1.0, self.nodeTo_b(layer, node))

                # Add addends for weighted input variables
                for previous_node in range(self.layerSizes[layer-1]):
                    weight = self.weights[layer-1][node][previous_node]

                    # Adjust weights and bias of first layer to incorporate input normalization
                    if self.normalize and layer == 1:
                        weight /= self.inputRanges[previous_node]
                        bias -= weight * self.inputMeans[previous_node]

                    # Adjust weights of output layer to incorporate output normalization
                    elif self.normalize and layer == len(self.layerSizes) - 1:
                        weight *= self.outputRange
                    e.addAddend(weight, self.nodeTo_f(layer-1, previous_node))

                # Adjust bias of output layer to incorporate output normalization
                if self.normalize and layer == len(self.layerSizes) - 1:
                    bias = bias * self.outputRange + self.outputMean
                e.setScalar(-bias)
                self.addEquation(e)

    def addRelus(self):
        """Identify all relus and their associated variable numbers and add them to Marabou network
        
        :meta private:
        """
        relus = []
        hidden_layers = self.layerSizes[1:-1]
        for layer, size in enumerate(hidden_layers):
            for node in range(size):
                self.addRelu(self.nodeTo_b(layer+1, node), self.nodeTo_f(layer+1, node))

    def numberOfVariables(self):
        """Get number of variables in network
        
        Returns:
            (int)
        """
        return self.layerSizes[0] + 2*sum(self.layerSizes[1:-1]) + 1*self.layerSizes[-1]

    def getInputMinimum(self, inputVar):
        """Get minimum value for given input variable

        Args:
            inputVar (int): Input variable
        Returns:
            (float)
        """
        return self.inputMinimums[inputVar]

    def getInputMaximum(self, inputVar):
        """Get maximum value for given input variable

        Args:
            inputVar (int): Input variable
        Returns:
            (float)
        """
        return self.inputMaximums[inputVar]
