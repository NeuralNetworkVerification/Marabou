'''
Top contributors (to current version):
    - Christopher Lazarus
    - Andrew Wu
    - Kyle Julian
    
This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

MarabouNetworkNNet represents neural networks with piecewise linear constraints derived from the NNet format
'''

from maraboupy.MarabouUtils import *
from maraboupy import MarabouCore
from maraboupy import MarabouNetwork

import warnings
import numpy as np
import sys


class MarabouNetworkNNet(MarabouNetwork.MarabouNetwork):
    """ Abstract class representing a Marabou network from an NNet file
        Inherits from MarabouNetwork class

        Attributes:

        initialized in __init__:
            numLayers (int): The number of layers in the network
            layerSizes (list of int): Layer sizes
            inputSize (int): Size of the input
            outputSize (int): Size of the output
            maxLayersize (int): Size of largest layer
            inputMinimums (list of float): Minimum value for each input
            inputMaximums (list of float): Maximum value for each input
            inputMeans (list of floats): Mean value for each input
            inputRanges (list of floats): Range for each input
            outputMean (float): Mean value of outputs
            outputRange (float): Range of output values
            weights (list): Network weight matrices, where the outer index corresponds to layer number
            biases (list): Network bias vectors, where the outer index corresponds to layer number

        initialized elsewhere:
            b_variables (list): List of b variables
            f_variables (list): list of f variables

        attributes from parent MarabouNetwork:
            numVars (int): Total number of variables to represent network
            equList (list of :class:`~maraboupy.MarabouUtils.Equation`): Network equations
            reluList (list of tuples): List of relu constraint tuples
            maxList (list of tuples): List of max constraint tuples
            varsParticipatingInConstraints (set of int): Variables involved in some constraint
            lowerBounds (Dict[int, float]): Lower bounds of variables
            upperBounds (Dict[int, float]): Upper bounds of variables
            inputVars (list of numpy arrays): Input variables
            outputVars (numpy array): Output variables

    """


    def __init__(self, filename='', normalize=False):
        """Constructs a MarabouNetworkNNet object from an .nnet file

        Args:
            filename: path to the .nnet file
            normalize: (bool) True if network parameters should be adjusted to incorporate
                          network input/output normalization. Otherwise, properties must be written
                          with the normalization already incorporated.
         """
        super().__init__()

        self.normalize = normalize

        if not filename:
            self.clearNetwork()
            return

        # Read the file and load values
        self.read_nnet(filename)
        self.computeNetworkAttributes()

    def clearNetwork(self):
        """Clears the network

         """

        # Call clear() from parent
        self.clear()

        self.numLayers = 0
        self.layerSizes = []
        self.inputSize = 0
        self.outputSize = 0
        self.maxLayersize = 0
        self.inputMinimums = []
        self.inputMaximums = []
        self.inputMeans = []
        self.inputRanges = []
        self.outputMean = 0
        self.outputRange = 0
        self.weights = []
        self.biases = []

    def resetNetworkFromParameters(self, weights: list, biases: list, normalize = False,
                                   inputMinimums=[], inputMaximums=[], inputMeans=[], inputRanges=[],
                                   outputMean=0, outputRange=1):
        """Recompute the attributes of the network from the basic argumemts

        Args:
            weights (list of list of lists): Outer index corresponds to layer number
            biases (list of lists): Outer index corresponds to layer number

            inputMinimums (list of floats) Observed minimal values for the network
            inputMaximums (list of floats) Observed maximal values for the network
            inputMeans  (list of floats): Mean value for each input
            inputRanges (list of floats): Range for each input
            outputMean  (float): Mean value of outputs
            outputRange (float): Range of output values

        """

        # Clearing the attributes that will be computed (clear() method from parent)
        self.clear()

        self.normalize = normalize

        # Assert that the essential arguments are non-empty
        assert weights
        assert weights[0]
        assert weights[0][0]
        assert biases
        assert biases[0]

        # Compute network parameters that can be computed from the rest
        numLayers = len(weights)
        inputSize = len(weights[0][0])
        outputSize = len(biases[-1])

        # Initiate non-mandatory parameters
        if not inputMinimums:
            inputMinimums = [-sys.float_info.max]*inputSize
        else:
            inputMinimums = inputMinimums[:]
        if not inputMaximums:
            inputMaximums = [sys.float_info.max]*inputSize
        else:
            inputMaximums = inputMaximums[:]
        if not inputMeans:
            inputMeans = [0]*inputSize
        else:
            inputMeans = inputMeans[:]
        if not inputRanges:
            inputRanges = [1]*inputSize
        else:
            inputRanges = inputRanges[:]

        # Find maximum size of any hidden layer
        maxLayersize = inputSize
        for b in biases:
            if len(b) > maxLayersize:
                maxLayersize = len(b)

        # Create a list of layer Sizes
        layerSizes = list()
        layerSizes.append(inputSize)
        for b in biases:
            layerSizes.append(len(b))

        self.numLayers = numLayers
        self.layerSizes = layerSizes
        self.inputSize = inputSize
        self.outputSize = outputSize
        self.maxLayersize = maxLayersize
        self.inputMinimums = inputMinimums
        self.inputMaximums = inputMaximums
        self.inputMeans = inputMeans
        self.inputRanges = inputRanges
        self.outputMean = outputMean
        self.outputRange = outputRange
        self.weights = weights
        self.biases = biases

        self.computeNetworkAttributes()
        # TODO (necessary? desirable?): create a new filename for the network

    def computeNetworkAttributes(self):

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

    # def getMarabouQuery(self):
    #     ipq = super(MarabouNetworkNNet, self).getMarabouQuery()
    #     return ipq

    def read_nnet(self, file_name):
        """Read the nnet file, load all the values and assign the class members

        Args:
            filename: path to the .nnet file
            
        :meta private:
        """
        with open(file_name) as f:
            line = f.readline()
            cnt = 1
            while line[0:2] == "//":
                line = f.readline()
                cnt += 1
            # numLayers does't include the input layer!
            numLayers, inputSize, outputSize, maxLayersize = [int(x) for x in line.strip().split(",")[:-1]]
            line = f.readline()

            # input layer size, layer1size, layer2size...
            layerSizes = [int(x) for x in line.strip().split(",")[:-1]]

            line = f.readline()
            symmetric = int(line.strip().split(",")[0])

            line = f.readline()
            inputMinimums = [float(x) for x in line.strip().split(",")[:-1]]

            line = f.readline()
            inputMaximums = [float(x) for x in line.strip().split(",")[:-1]]

            line = f.readline()
            means = [float(x) for x in line.strip().split(",")[:-1]]

            line = f.readline()
            ranges = [float(x) for x in line.strip().split(",")[:-1]]

            weights = []
            biases = []
            for layernum in range(numLayers):

                previousLayerSize = layerSizes[layernum]
                currentLayerSize = layerSizes[layernum + 1]
                # weights
                weights.append([])
                biases.append([])
                # weights
                for i in range(currentLayerSize):
                    line = f.readline()
                    aux = [float(x) for x in line.strip().split(",")[:-1]]
                    weights[layernum].append([])
                    for j in range(previousLayerSize):
                        weights[layernum][i].append(aux[j])
                # biases
                for i in range(currentLayerSize):
                    line = f.readline()
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

            if not self.normalize:
                for i in range(self.inputSize):
                    self.inputMinimums[i] = (self.inputMinimums[i] - self.inputMeans[i]) / self.inputRanges[i]
                    self.inputMaximums[i] = (self.inputMaximums[i] - self.inputMeans[i]) / self.inputRanges[i]



    def writeNNet(self, file_name: str):
        """Write network data into the .nnet file format

        Args:
            file_name (str): File where the network will be written

        :meta private:
        """

        # Open the file we wish to write
        with open(file_name, 'w') as f2:

            #####################
            # First, we write the header lines:
            # The first line written is just a line of text
            # The second line gives the four values:
            #     Number of fully connected layers in the network
            #     Number of inputs to the network
            #     Number of outputs from the network
            #     Maximum size of any hidden layer
            # The third line gives the sizes of each layer, including the input and output layers
            # The fourth line gives an outdated flag, so this can be ignored
            # The fifth line specifies the minimum values each input can take
            # The sixth line specifies the maximum values each input can take
            #     Inputs passed to the network are truncated to be between this range
            # The seventh line gives the mean value of each input and of all outputs
            # The eighth line gives the range of each input and of all outputs
            #     These two lines are used to map raw inputs to the 0 mean, unit range of the inputs and outputs
            #     used during training
            # The ninth line begins the network weights and biases
            ####################
            # f2.write("// The contents of this file are licensed under the Creative Commons\n")
            # f2.write("// Attribution 4.0 International License: https://creativecommons.org/licenses/by/4.0/\n")
            f2.write("// Neural Network File Format by Kyle Julian, Stanford 2016\n")
            f2.write("// Network written to file by writeNNet() method of class MarabouNetworkNNet\n")

            # Write data to header
            f2.write("%d,%d,%d,%d,\n" % (self.numLayers, self.inputSize, self.outputSize, self.maxLayersize))
            # f2.write("%d," % inputSize)
            for s in self.layerSizes:
                f2.write("%d," % s)
            f2.write("\n")
            f2.write("0,\n")  # Unused Flag # CHECK: symmetric??

            # Write Min, Max, Mean, and Range of each of the inputs and outputs for normalization
            # If normalization was not incorporated when the network was originally created, which means that
            #   normalization was performed when creating the MarabouNNet object, we undo it here.

            inputMinimums = self.inputMinimums[:]
            inputMaximums = self.inputMaximums[:]

            if not self.normalize:
                for i in range(self.inputSize):
                    inputMinimums[i] = self.inputMinimums[i] * self.inputRanges[i] + self.inputMeans[i]
                    inputMaximums[i] = self.inputMaximums[i] * self.inputRanges[i] + self.inputMeans[i]

            # Minimum Input Values
            f2.write(','.join(str(inputMinimums[i]) for i in range(self.inputSize)) + ',\n')

            # Maximum Input Values
            f2.write(','.join(str(inputMaximums[i]) for i in range(self.inputSize)) + ',\n')

            # Means for normalizations
            f2.write(','.join(str(self.inputMeans[i]) for i in range(self.inputSize)) +
                     ',' + str(self.outputMean) + ',\n')

            # Ranges for normalizations
            f2.write(','.join(str(self.inputRanges[i]) for i in range(self.inputSize)) +
                     ',' + str(self.outputRange) + ',\n')

            ##################
            # Write weights and biases of neural network
            # First, the weights from the input layer to the first hidden layer are written
            # Then, the biases of the first hidden layer are written
            # The pattern is repeated by next writing the weights from the first hidden layer to the second hidden layer,
            # followed by the biases of the second hidden layer.
            ##################
            for w, b in zip(self.weights, self.biases):
                w = np.array(w)
                for i in range(w.shape[0]):
                    for j in range(w.shape[1]):
                        f2.write("%.5e," % w[i][
                            j])  # Five digits written. More can be used, but that requires more space.
                    f2.write("\n")

                for i in range(len(b)):
                    f2.write(
                        "%.5e,\n" % b[i])  # Five digits written. More can be used, but that requires more more space.


    def variableRanges(self):
        """Compute the variable number ranges for each type (b, f)
        
        :meta private:
        """
        b_variables = []
        f_variables = []
        output_variables = []

        input_variables = [i for i in range(self.layerSizes[0])]

        hidden_layers = self.layerSizes[1:-1]

        for layer, hidden_layer_length in enumerate(hidden_layers):
            for i in range(hidden_layer_length):
                offset = sum([x * 2 for x in hidden_layers[:layer]])

                b_variables.append(self.layerSizes[0] + offset + i)
                f_variables.append(self.layerSizes[0] + offset + i + hidden_layer_length)

        # final layer
        for i in range(self.layerSizes[-1]):
            offset = sum([x * 2 for x in hidden_layers[:len(hidden_layers) - 1]])
            output_variables.append(self.layerSizes[0] + offset + i + 2 * hidden_layers[-1])

        self.inputVars = np.array([input_variables])
        self.b_variables = b_variables
        self.f_variables = f_variables
        self.outputVars = np.array([output_variables])

    def nodeTo_b(self, layer, node):
        """Compute variable number for the backward variable of a node in a layer

        Args:
            layer (int): Layer number
            node (int): Node number

        Returns:
            (int)
        
        :meta private:
        """
        assert(0 < layer)
        assert(node < self.layerSizes[layer])

        offset = self.layerSizes[0]
        offset += sum([x * 2 for x in self.layerSizes[1:layer]])

        return offset + node

    def nodeTo_f(self, layer, node):
        """Compute variable number for the forward variable of a node in a layer

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
            offset += sum([x * 2 for x in self.layerSizes[1:layer]])
            offset += self.layerSizes[layer]

            return offset + node

    def getVariable(self, layer, node, f=True):
        """ Returns variable number corresponding to layer, node
            If f=True, f variables
            Otherwise, b variables
        Args:
            layer (int)
            node (int)
            f (bool)
        Return:
            (int)
        """
        return self.nodeTo_f(layer, node) if f else self.nodeTo_b(layer, node)

    def getUpperBound(self, layer, node, f=True):
        """ Returns upper bound for the variable corresponding to layer, node
            If f=True, f variable
            Otherwise, b variable
        Args:
            layer (int)
            node (int)
            f (bool)
        Return:
            (float)
        """
        var = self.getVariable(layer, node, f)
        if self.upperBoundExists(var):
            return self.upperBounds[var]
        return None

    def getLowerBound(self, layer, node, f=True):
        """ Returns lower bound for the variable corresponding to layer, node
            If f=True, f variable
            Otherwise, b variable
        Args:
            layer (int)
            node (int)
            f (bool)
        Return:
            (float)
        """
        var = self.getVariable(layer, node, f)
        if self.lowerBoundExists(var):
            return self.lowerBounds[var]
        return None

    def getUpperBoundsForLayer(self, layer, f=True):
        """ Returns a list of upper bounds for the given layer
            If f=True, f variable
            Otherwise, b variable
        Args:
            layer (int)
            f (bool)
        Return:
            (list of floats)
        """
        bound_list = [self.getUpperBound(layer, node, f) for node in range(self.layerSizes[layer])]
        return bound_list

    def getLowerBoundsForLayer(self, layer, f=True):
        """ Returns a list of lower bounds for the given layer
            If f=True, f variable
            Otherwise, b variable
        Args:
            layer (int)
            f (bool)
        Return:
            (list of floats)
        """
        bound_list = [self.getLowerBound(layer, node, f) for node in range(self.layerSizes[layer])]
        return bound_list

    def getBoundsForLayer(self, layer, f=True):
        """ Returns a tuple of two lists, the lower and upper bounds for the variables corresponding to the given layer
            If f=True, f variable
            Otherwise, b variable
        Args:
            layer (int)
            f (bool)
        Returns:
            tuple of two list of floats (1st is lower bounds)
        """
        return self.getLowerBoundsForLayer(layer, f), self.getUpperBoundsForLayer(layer, f)


    def numberOfVariables(self):
        return self.layerSizes[0] + 2 * sum(self.layerSizes[1:-1]) + 1 * self.layerSizes[-1]

    def getInputMinimum(self, inputVar):
        return self.inputMinimums[inputVar]

    def getInputMaximum(self, inputVar):
        return self.inputMaximums[inputVar]


    def evaluateWithoutMarabou(self, inputValues):
        """ Evaluate network directly (without Marabou) at a given point
        Args:
            inputValues (list of np arrays): input to network
        Returns:
            (np array): output of the network
        """

        return self.evaluateNNet(inputValues.flatten().tolist(), normalize_inputs=self.normalize,
                                 normalize_outputs=self.normalize)


    def evaluateNNet(self, inputs, first_layer = 0, last_layer=-1, normalize_inputs=False,
                     normalize_outputs=False, activate_output_layer=False):
        """ Evaluate nnet directly (without Marabou) at a given point
                between layer first_layer (input layer by default)
                and layer last_layer (output layer by default)

        Args:
            inputs: (list of floats):         Network inputs to be evaluated
            first_layer:  (int):              the initial layer of the evaluation
            last_layer: (int):                the last layer of the evaluation
            normalize_inputs: (bool):         if True and first_layer==0, normalization of inputs is performed
            normalize_outputs: (bool):        if True, normalization of output is undone
            activate_output_layer: (bool):    if True, the last layer is activated, otherwise it is not.

        Returns:
            (list of floats): the result of the evaluation
        """
        num_layers = self.numLayers
        input_size = self.inputSize
        output_size = self.outputSize
        biases = self.biases
        weights = self.weights
        mins = self.inputMinimums
        maxes = self.inputMaximums
        means = self.inputMeans
        ranges = self.inputRanges

        # The default output layer is the last (output) layer
        if last_layer == -1:
            last_layer = num_layers

        inputs_norm = inputs[:]

        # Prepare the inputs to the neural network
        if normalize_inputs:
            if (first_layer == 0):


                for i in range(input_size):
                    if inputs[i] < mins[i]:
                        inputs_norm[i] = (mins[i] - means[i]) / ranges[i]
                    elif inputs[i] > maxes[i]:
                        inputs_norm[i] = (maxes[i] - means[i]) / ranges[i]
                    else:
                        inputs_norm[i] = (inputs[i] - means[i]) / ranges[i]
            else:
                warnings.warn('Normalization of inputs is supported for the input layer only. Request for '
                              'normalization of inputs ignored.')

        # Evaluate the neural network
        for layer in range(first_layer, last_layer - 1):
            inputs_norm = np.maximum(np.dot(weights[layer], inputs_norm) + biases[layer], 0)

        layer = last_layer - 1

        if (activate_output_layer):
            outputs = np.maximum(np.dot(weights[layer], inputs_norm) + biases[layer], 0)
        else:
            outputs = np.dot(weights[layer], inputs_norm) + biases[layer]

        # Undo output normalization
        if (normalize_outputs):
            if last_layer == num_layers:
                output_mean = self.outputMean
                output_range = self.outputRange
            else:
                output_mean = means[layer - 1]
                output_range = ranges[layer - 1]

            for i in range(output_size):
                outputs[i] = outputs[i] * output_range + output_mean

        return outputs


    def createRandomInputsForNetwork(self):

        inputs = []
        for input_var in self.inputVars.flatten():
            assert self.upperBoundExists(input_var)
            assert self.lowerBoundExists(input_var)
            random_value = np.random.uniform(low=self.lowerBounds[input_var],
                                             high=self.upperBounds[input_var])
            inputs.append(random_value)
        return inputs


    def buildEquations(self):
        """Construct the Marabou equations
        
        :meta private:
        """
        for layer, size in enumerate(self.layerSizes):
            if layer == 0:
                continue

            for node in range(size):
                bias = self.biases[layer - 1][node]

                # Add marabou equation and add addend for output variable
                e = Equation()
                e.addAddend(-1.0, self.nodeTo_b(layer, node))

                # Add addends for weighted input variables
                for previous_node in range(self.layerSizes[layer - 1]):
                    weight = self.weights[layer - 1][node][previous_node]

                    # Adjust weights and bias of first layer to incorporate input normalization
                    if self.normalize and layer == 1:
                        weight /= self.inputRanges[previous_node]
                        bias -= weight * self.inputMeans[previous_node]

                    # Adjust weights of output layer to incorporate output normalization
                    elif self.normalize and layer == len(self.layerSizes) - 1:
                        weight *= self.outputRange
                    e.addAddend(weight, self.nodeTo_f(layer - 1, previous_node))

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
                self.addRelu(self.nodeTo_b(layer + 1, node), self.nodeTo_f(layer + 1, node))

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

