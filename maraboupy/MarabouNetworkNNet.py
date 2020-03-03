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

from MarabouUtils import *
import MarabouNetwork
import numpy as np




class MarabouNetworkNNet(MarabouNetwork.MarabouNetwork):
    """
    Class that implements a MarabouNetwork from an NNet file.
    """

    def __init__(self, filename = "", perform_sbt=False):
        """
        Constructs a MarabouNetworkNNet object from an .nnet file.

        Args:
            filename: path to the .nnet file.
        Attributes:

        set in constructor:
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
            sbt              The SymbolicBoundTightener object


            inputVars
            b_variables
            f_variables
            outputVars


        Attributes from parent (MarabouNetwork)

            self.numVars
            self.equList = []
            self.reluList = []
            self.maxList = []
            self.varsParticipatingInConstraints = set()
            self.lowerBounds = dict()
            self.upperBounds = dict()
            self.inputVars = []
            self.outputVars = np.array([])


        """
        super(MarabouNetworkNNet,self).__init__()

        if not filename:
            self.numLayers = 0
            self.layerSizes = []
            self.inputSize = 0
            self.outputSize = 0
            self.maxLayersize = 0
            self.inputMinimums = []
            self.inputMaximums = []
            self.inputMeans = []
            self.inputRanges = []
            self.weights = []
            self.biases = []

            self.sbt = None
            return
            

        # read the file and load values
        self.read_nnet(filename)

        self.computeNetworkAttributes(perform_sbt,filename)

    def clearNetwork(self):
        # call clear() from parent
        self.clear()

        self.numLayers = 0
        self.layerSizes = 0
        self.inputSize = 0
        self.outputSize = 0
        self.maxLayersize = 0
        self.inputMinimums = []
        self.inputMaximums = []
        self.inputMeans = []
        self.inputRanges = []
        self.weights = []
        self.biases = []

        self.sbt = None






    def resetNetworkFromParameters(self,inputMinimums,inputMaximums, inputMeans,inputRanges, weights,biases,\
                                   numLayers=-1,layerSizes=[],inputSize=-1,outputSize=-1,maxLayersize=-1,perform_sbt = False):

        #Clearing the attributes that will be computed (clear() method from parent)
        self.clear()

        # Compute network parameters that can be computed from the rest
        _numLayers = len(weights)
        _inputSize = len(inputMinimums)
        _outputSize = len(biases[-1])

        # Find maximum size of any hidden layer
        _maxLayerSize = _inputSize
        for b in biases:
            if len(b) > _maxLayerSize:
                _maxLayerSize = len(b)
        # Create a list of layer Sizes
        _layerSizes = []
        _layerSizes.append(_inputSize)
        for b in biases:
            _layerSizes.append(len(b))

        if numLayers == -1:
            numLayers = _numLayers
        if inputSize == -1:
            inputSize = _inputSize
        if outputSize == -1:
            outputSize = _outputSize
        if layerSizes == []:
            layerSizes = _layerSizes
        if maxLayersize == -1:
            maxLayersize = _maxLayerSize

        # Checking that the parameters provided in the arguments agree with what we have computed

        inputError = False
        if numLayers != _numLayers:
            numLayers = _numLayers
            inputError = True
            errorPlace = 1
        if inputSize != _inputSize:
            inputSize = _inputSize
            inputError = True
            errorPlace = 2
        if outputSize != _outputSize:
            outputSize = _outputSize
            inputError = True
            errorPlace = 3
        if layerSizes != _layerSizes:
            # print(len(biases))
            # print([len(b) for b in biases])
            # print(layerSizes)
            # print(_layerSizes)
            layerSizes = _layerSizes
            inputError = True
            errorPlace = 4
        if maxLayersize != _maxLayerSize:
            maxLayersize = _maxLayerSize
            inputError = True
            errorPlace = 5




        if inputError:
            print("\nSomething went wrong with the arguments, corrected! Error code : ",errorPlace,"\n")


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

        self.computeNetworkAttributes(perform_sbt,filename="")
        # NOTE: here we assume tha the filename is not used in createSBT()
        # To-do (necessary? desireable?): create a new filename for the network



    def computeNetworkAttributes(self,perform_sbt,filename=""):
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
        if perform_sbt:
            self.sbt = self.createSBT(filename)
        else:
            self.sbt = None




    def createSBT(self, filename):

        '''
        Creates a symbolic bound tightener

        CHECK: is the filename attribute used??
        '''


        sbt = MarabouCore.SymbolicBoundTightener()
        sbt.setNumberOfLayers(self.numLayers + 1)
        for layer, size in enumerate(self.layerSizes):
            sbt.setLayerSize(layer, size)
        sbt.allocateWeightAndBiasSpace()
        # Biases
        for layer in range(len(self.biases)):
            for node in range(len(self.biases[layer])):
                sbt.setBias(layer + 1, node, self.biases[layer][node])
        # Weights
        for layer in range(len(self.weights)): # starting from the first hidden layer
            for target in range(len(self.weights[layer])):
                for source in range(len(self.weights[layer][target])):
                    sbt.setWeight(layer, source, target, self.weights[layer][target][source])

        # Initial bounds
        for i in range(self.inputSize):
            sbt.setInputLowerBound( i, self.getInputMinimum(i))
            sbt.setInputUpperBound( i, self.getInputMaximum(i))
        
        # Variable indexing of hidden layers
        for layer in range(len(self.layerSizes))[1:-1]:
            for node in range(self.layerSizes[layer]):
                sbt.setReluBVariable(layer, node, self.nodeTo_b(layer, node))
                sbt.setReluFVariable(layer, node, self.nodeTo_f(layer, node))

        for node in range(self.outputSize):
            sbt.setReluFVariable(len(self.layerSizes) - 1, node, self.nodeTo_b(len(self.layerSizes) - 1, node))

        return sbt

    def getMarabouQuery(self):
        ipq = super(MarabouNetworkNNet, self).getMarabouQuery()
        ipq.setSymbolicBoundTightener(self.sbt)
        return ipq


    def read_nnet(self, file_name):

        """
        Read the nnet file, load all the values and assign the class members.

        Args:
            filename: path to the .nnet file.
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


    def writeNNet(self, fileName: str):
        '''
        Write network data to the .nnet file format

        Args:

            fileName (str): File where the network will be written
        '''

        # Open the file we wish to write
        with open(fileName, 'w') as f2:

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
            #f2.write("// The contents of this file are licensed under the Creative Commons\n")
            #f2.write("// Attribution 4.0 International License: https://creativecommons.org/licenses/by/4.0/\n")
            f2.write("// Neural Network File Format by Kyle Julian, Stanford 2016\n")
            f2.write("// Network written to file by writeNNet() method of class MarabouNetworkNNet\n")

            # Extract the necessary information and write the header information
            # numLayers = len(weights)
            # inputSize = weights[0].shape[1]
            # outputSize = len(biases[-1])
            # maxLayerSize = inputSize

            # Find maximum size of any hidden layer
            # for b in biases:
            #     if len(b) > maxLayerSize:
            #         maxLayerSize = len(b)

            # Write data to header
            f2.write("%d,%d,%d,%d,\n" % (self.numLayers, self.inputSize, self.outputSize, self.maxLayersize))
            # f2.write("%d," % inputSize)
            for s in self.layerSizes:
                f2.write("%d," % s)
            f2.write("\n")
            f2.write("0,\n")  # Unused Flag # CHECK: symmetric??

            # Write Min, Max, Mean, and Range of each of the inputs and outputs for normalization
            f2.write(','.join(str(self.inputMinimums[i]) for i in range(self.inputSize)) + ',\n')  # Minimum Input Values
            f2.write(
                ','.join(str(self.inputMaximums[i]) for i in range(self.inputSize)) + ',\n')  # Maximum Input Values


            f2.write(','.join(str(self.inputMeans[i]) for i in range(self.inputSize + 1)) + ',\n')  # Means for normalizations
            f2.write(','.join(str(self.inputRanges[i]) for i in range(self.inputSize + 1)) + ',\n')  # Ranges for noramlizations

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

        """
        Compute the variable number ranges for each type (input, output, b, f)

        Args:
            None
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

        assert(0 < layer)
        assert(node < self.layerSizes[layer])
        
        offset = self.layerSizes[0]
        offset += sum([x*2 for x in self.layerSizes[1:layer]])
        
        return offset + node


    def nodeTo_f(self, layer, node):

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

        """
        Constructs the equation representation from the class members
        Arguments:
            None
        Returns:
            equations_aux   (list of lists) that represents all the equations in
                the network.
        """
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


    def findRelus(self):

        """
        Identify all relus and their associated variable numbers.
        Arguments:
            None
        Returns:
            relus   (list of lists) that represents all the relus in
                the network.
        """

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






    def evaluateNetwork(self, inputs, normalize_inputs=True, normalize_outputs=True, activate_output_layer=False):

        """
        Evaluate the network directly, without Marabou

        Args:
            inputs (list of floats): Network inputs to be evaluated

        Returns:
            (numpy array of floats): Network output
       """

        numLayers = self.numLayers
        inputSize = self.inputSize
        outputSize = self.outputSize
        biases = self.biases
        weights = self.weights
        mins = self.inputMinimums
        maxes = self.inputMaximums
        means = self.inputMeans
        ranges = self.inputRanges

        # Prepare the inputs to the neural network
        if (normalize_inputs):
            inputsNorm = np.zeros(inputSize)
            for i in range(inputSize):
                if inputs[i] < mins[i]:
                    inputsNorm[i] = (mins[i] - means[i]) / ranges[i]
                elif inputs[i] > maxes[i]:
                    inputsNorm[i] = (maxes[i] - means[i]) / ranges[i]
                else:
                    inputsNorm[i] = (inputs[i] - means[i]) / ranges[i]
        else:
            inputsNorm = inputs

        # Evaluate the neural network
        for layer in range(numLayers - 1):
            inputsNorm = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer], 0)

        if (activate_output_layer):
            outputs = np.maximum(np.dot(weights[-1], inputsNorm) + biases[-1], 0)
        else:
            outputs = np.dot(weights[-1], inputsNorm) + biases[-1]

        # Undo output normalization
        if (normalize_outputs):
            for i in range(outputSize):
                outputs[i] = outputs[i] * ranges[-1] + means[-1]

        return outputs




    def evaluateNetworkMultiple(self, inputs, normalize_inputs=True, normalize_outputs=True,
                                  activate_output_layer=False):

        """
         Evaluate network using multiple sets of inputs

         Args:
             inputs (numpy array of lists of floats): Array of network inputs to be evaluated.

         Returns:
             (numpy array of numpy arrays floats): Network outputs for each set of inputs

        NOT TESTED
         """

        numLayers = self.numLayers
        inputSize = self.inputSize
        outputSize = self.outputSize
        biases = self.biases
        weights = self.weights
        inputs = np.array(inputs).T
        mins = self.inputMinimums
        maxes = self.inputMaximums
        means = self.inputMeans
        ranges = self.inputRanges


        # Prepare the inputs to the neural network
        numInputs = inputs.shape[1]

        if (normalize_inputs):
            inputsNorm = np.zeros(inputSize)
            for i in range(inputSize):
                if inputs[i] < mins[i]:
                    inputsNorm[i] = (mins[i] - means[i]) / ranges[i]
                elif inputs[i] > maxes[i]:
                    inputsNorm[i] = (maxes[i] - means[i]) / ranges[i]
                else:
                    inputsNorm[i] = (inputs[i] - means[i]) / ranges[i]
        else:
            inputsNorm = inputs

        # Evaluate the neural network
        for layer in range(numLayers - 1):
            inputsNorm = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer].reshape((len(biases[layer]), 1)),
                                    0)

        if (activate_output_layer):
            outputs = np.maximum(np.dot(weights[-1], inputsNorm) + biases[-1].reshape((len(biases[-1]), 1)), 0)
        else:
            outputs = np.dot(weights[-1], inputsNorm) + biases[-1].reshape((len(biases[-1]), 1))

        # Undo output normalization
        if (normalize_outputs):
            for i in range(outputSize):
                for j in range(numInputs):
                    outputs[i, j] = outputs[i, j] * ranges[-1] + means[-1]

        return outputs.T



    # def evaluateNetwork(self, inputs, normalize_inputs=True, normalize_outputs=True, activate_output_layer=False):
    #     return self.evaluateNetworkToLayer(inputs,last_layer=0,normalize_inputs=normalize_inputs,normalize_outputs=normalize_outputs,activate_output_layer=activate_output_layer)


    def evaluateNetworkToLayer(self, inputs, last_layer = 0, normalize_inputs=False, normalize_outputs=False, activate_output_layer=False):

        """
        Evaluate the network directly, without Marabou
        Computes the output at a given layer (ouptut layer by default)


        Args:
            inputs (list of floats): Network inputs to be evaluated

        Returns:
            (numpy array of floats): Network output

       """

        numLayers = self.numLayers
        inputSize = self.inputSize
        outputSize = self.outputSize
        biases = self.biases
        weights = self.weights
        mins = self.inputMinimums
        maxes = self.inputMaximums
        means = self.inputMeans
        ranges = self.inputRanges

        #The default output layer is the last (output) layer
        if last_layer == 0:
            last_layer = numLayers

        # Prepare the inputs to the neural network
        if (normalize_inputs):
            inputsNorm = np.zeros(inputSize)
            for i in range(inputSize):
                if inputs[i] < mins[i]:
                    inputsNorm[i] = (mins[i] - means[i]) / ranges[i]
                elif inputs[i] > maxes[i]:
                    inputsNorm[i] = (maxes[i] - means[i]) / ranges[i]
                else:
                    inputsNorm[i] = (inputs[i] - means[i]) / ranges[i]
        else:
            inputsNorm = inputs

        # Evaluate the neural network
        for layer in range(last_layer-1):
            inputsNorm = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer], 0)

        layer+=1
        #print layer
        #print last_layer


        if (activate_output_layer):
            outputs = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer], 0)
        else:
            outputs = np.dot(weights[layer], inputsNorm) + biases[layer]

        # Undo output normalization
        if (normalize_outputs):
            for i in range(outputSize):
                outputs[i] = outputs[i] * ranges[layer-1] + means[layer-1]

        return outputs





    def evaluateNetworkFromLayer(self, inputs, first_layer = 0, normalize_inputs=False, normalize_outputs=False, activate_output_layer=False):

        """
        Evaluate the network directly, without Marabou, starting after a specific layer (input layer by default)

        Args:
            inputs (list of floats): inputs to the given layer for the network to be evaluated on

        Returns:
            (numpy array of floats): Network output
       """

        numLayers = self.numLayers
        outputSize = self.outputSize
        biases = self.biases
        weights = self.weights
        mins = self.inputMinimums
        maxes = self.inputMaximums
        means = self.inputMeans
        ranges = self.inputRanges

        # Prepare the inputs to the neural network
        # if (normalize_inputs):
        #     inputsNorm = np.zeros(inputSize)
        #     for i in range(inputSize):
        #         if inputs[i] < mins[i]:
        #             inputsNorm[i] = (mins[i] - means[i]) / ranges[i]
        #         elif inputs[i] > maxes[i]:
        #             inputsNorm[i] = (maxes[i] - means[i]) / ranges[i]
        #         else:
        #             inputsNorm[i] = (inputs[i] - means[i]) / ranges[i]
        # else:
        #     inputsNorm = inputs


        # We do not allow normalizations of the inputs here for now.
        inputsNorm  = inputs

        # Evaluate the neural network
        for layer in range(first_layer,numLayers-1):
            inputsNorm = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer], 0)

        if (activate_output_layer):
            outputs = np.maximum(np.dot(weights[-1], inputsNorm) + biases[-1], 0)
        else:
            outputs = np.dot(weights[-1], inputsNorm) + biases[-1]

        # Undo output normalization
        if (normalize_outputs):
            for i in range(outputSize):
                outputs[i] = outputs[i] * ranges[-1] + means[-1]

        return outputs


    """
     Evaluate network directly, up to a certain layer, using multiple sets of inputs

     Args:
         inputs (list of floats): Array of network inputs to be evaluated.

     Returns:
         (numpy array of floats): Network outputs for each set of inputs

    HAS NOT BEEN TESTED
     """

    def evaluateNetworkMultipleToLayer(self, inputs, last_layer=0, normalize_inputs=True, normalize_outputs=True,
                                       activate_output_layer=False):

        numLayers = self.numLayers
        inputSize = self.inputSize
        outputSize = self.outputSize
        biases = self.biases
        weights = self.weights
        inputs = np.array(inputs).T
        mins = self.inputMinimums
        maxes = self.inputMaximums
        means = self.inputMeans
        ranges = self.inputRanges

        # Prepare the inputs to the neural network
        numInputs = inputs.shape[1]

        if last_layer == 0:
            last_layer = numLayers

        if (normalize_inputs):
            inputsNorm = np.zeros((inputSize, numInputs))
            for i in range(inputSize):
                for j in range(numInputs):
                    if inputs[i, j] < mins[i]:
                        inputsNorm[i, j] = (mins[i] - means[i]) / ranges[i]
                    elif inputs[i, j] > maxes[i]:
                        inputsNorm[i, j] = (maxes[i] - means[i]) / ranges[i]
                    else:
                        inputsNorm[i, j] = (inputs[i, j] - means[i]) / ranges[i]
        else:
            inputsNorm = inputs

        # Evaluate the neural network
        for layer in range(last_layer - 1):
            inputsNorm = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer].reshape((len(biases[layer]), 1)),
                                    0)

        layer += 1

        if (activate_output_layer):
            outputs = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer].reshape((len(biases[layer]), 1)), 0)
        else:
            outputs = np.dot(weights[-1], inputsNorm) + biases[-1].reshape((len(biases[-1]), 1))

        # Undo output normalization
        if (normalize_outputs):
            for i in range(outputSize):
                for j in range(numInputs):
                    outputs[i, j] = outputs[i, j] * self.ranges[layer - 1] + self.means[layer - 1]

        return outputs.T


