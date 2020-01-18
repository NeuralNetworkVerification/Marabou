'''
/* *******************                                                        */
/*! \file MarabouNetworkONNX.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian
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

import numpy as np
import onnx
import onnxruntime
from onnx import numpy_helper
from maraboupy import MarabouUtils
from maraboupy import MarabouNetwork

class MarabouNetworkONNX(MarabouNetwork.MarabouNetwork):
    def __init__(self, filename, inputNames=None, outputName=None):
        """
        Constructs a MarabouNetworkONNX object from an ONNX file

        Args:
            filename: (string) Path to the ONNX file
            inputNames: (list of strings) optional, list of node names corresponding to inputs.
            outputName: (string) optional, name of node corresponding to output.
        Returns:
            marabouNetworkONNX: (MarabouNetworkONNX) representing network
        """
        super().__init__()
        self.readONNX(filename, inputNames, outputName)

    def clear(self):
        """
        Reset values to represent empty network
        """
        super().clear()
        self.madeGraphEquations = []
        self.madeShapes = []
        self.varMap = dict()
        self.constantMap = dict()
        self.shapeMap = dict()
        self.inputNames = None
        self.outputName = None
        self.graph = None
        
    def readONNX(self, filename, inputNames, outputName):
        """
        Constructs a MarabouNetworkONNX object from an ONNX file

        Args:
            filename: (string) Path to the ONNX file
            inputNames: (list of strings) optional, list of names corresponding to inputs.
            outputName: (string) optional, name of node corresponding to output.
        Returns:
            marabouNetworkONNX: (MarabouNetworkONNX) representing network
        """
        self.filename = filename
        self.graph = onnx.load(filename).graph
        
        # Get default inputs/output if no names are provided
        if not inputNames:
            assert len(self.graph.input) >= 1
            inputNames = [inp.name for inp in self.graph.input]
        if not outputName:
            assert len(self.graph.output) == 1
            outputName = self.graph.output[0].name
            
        # Check that input/outputs are in the graph
        for name in inputNames:
            if not len([nde for nde in self.graph.node if name in nde.input]):
                print("Input %s not found in graph!" % name)
                raise RuntimeError
        if not len([nde for nde in self.graph.node if outputName in nde.output]):
            print("Output %s not found in graph!" % outputName)
            raise RuntimeError
        self.inputNames = inputNames
        self.outputName = outputName
        
        # First process the shapes of all nodes in the network.
        # The graph only specifies shapes for inputs, output, and constants,
        # and the rest must be inferred from node operations.
        self.foundnInputFlags = 0
        self.processShapes()
        assert self.foundnInputFlags == len(self.inputNames)
        
        # To be consistent with other network parsers, input and output variables are assigned first
        self.processInputAndOutput()
        
        # Assign the remaining variables and compile lists of equations and relus
        self.makeGraphEquations(self.outputName)
        
        # If the given inputNames/outputName specify only a portion of the network, then we will have
        # shape information saved not relevant to the portion of the network. Remove extra shapes.
        self.cleanShapes()
        
    def processShapes(self):
        """
        Determine shape of all nodes and save to self.shapeMap
        """
        # Add shapes for the graph's inputs
        for node in self.graph.input:
            self.shapeMap[node.name] = list([dim.dim_value for dim in node.type.tensor_type.shape.dim])
            self.madeShapes += [node.name]
            if node.name in self.inputNames:
                self.foundnInputFlags += 1
                
        # Add shapes for constants
        for node in self.graph.initializer:
            self.shapeMap[node.name] = list(node.dims)
            self.madeShapes += [node.name]
            
        # Pad shape with extra dimension if there is only one dimension. Helpful for MatMul 
        for nodeName in self.madeShapes:
            if len(self.shapeMap[nodeName]) == 1:
                self.shapeMap[nodeName] += [1]
                
        # Recursively add the remaining shapes
        self.makeShapes(self.outputName)
        
        # Remove any extra shape padding for the input/output nodes to be consistent with other Marabou parsers
        for nodeName in self.inputNames + [self.outputName]:
            if len(self.shapeMap[nodeName]) > 1 and self.shapeMap[nodeName][-1] == 1:
                self.shapeMap[nodeName] = self.shapeMap[nodeName][:-1]
        
    def makeShapes(self, nodeName):
        """
        Recursively add shapes to self.shapeMap of nodes that are the result of an operation
        Arguments:
            nodeName: (str) name of node for making the shape
        """
        if nodeName in self.madeShapes:
            return
        
        if nodeName in self.inputNames:
            self.foundnInputFlags += 1      
        self.madeShapes += [nodeName]
        
        # Recursively call makeShapes, then call computeShape
        # This ensures that shapes of a node's inputs have been computed first 
        for inNodeName in self.getInputNodes(nodeName):
            self.makeShapes(inNodeName)
        self.computeShape(nodeName)
            
    def computeShape(self, nodeName):
        """
        Compute the shape of a node assuming the input shapes have been computed already.
        Arguments:
            nodeName: (str) name of node for which we want to compute the output shape
        """
        node = self.getNode(nodeName)
        
        if node.op_type == 'MatMul':
            shape0 = self.shapeMap[node.input[0]]
            shape1 = self.shapeMap[node.input[1]]
            assert shape0[-1] == shape1[0]
            
            # Pad the output of MatMul with an extra 1 if needed
            if len(shape1)==1:
                self.shapeMap[nodeName] =  shape0[:-1] + [1]
            else:
                self.shapeMap[nodeName] =  shape0[:-1] + shape1[1:]
        elif node.op_type == 'Add':
            shape0 = self.shapeMap[node.input[0]]
            shape1 = self.shapeMap[node.input[1]]
            assert shape0 == shape1
            self.shapeMap[nodeName] = shape0   
        elif node.op_type== 'Relu':
            self.shapeMap[nodeName] = self.shapeMap[node.input[0]]  
        else:
            print("Operation %s not implemented" % (node.op_type))
            raise NotImplementedError
         
    def cleanShapes(self):
        """
        After constructing equations, remove shapes from self.shapeMap that are part of the graph but not
        relevant for this input query. This is only cosmetic and does not impact Marabou
        """
        for nodeName in [name for name in self.shapeMap]:
            if nodeName not in self.varMap and nodeName not in self.constantMap:
                self.shapeMap.pop(nodeName)
        
    def makeGraphEquations(self, nodeName):
        """
        Function to generate equations for network necessary to calculate the node
        Arguments:
            nodeName: (str) representing the name of the node for which we want to generate network equations
        """
        if nodeName in self.madeGraphEquations:
            return
        
        self.madeGraphEquations += [nodeName]
        
        # Recursively call makeGraphEquations before makeNeuronEquations to ensure that each
        # node's input variables are defined before making equations for the output
        for inNodeName in self.getInputNodes(nodeName, saveConstant=True):
            self.makeGraphEquations(inNodeName)
        self.makeNeuronEquations(nodeName)
        
    def makeNeuronEquations(self, nodeName):
        """
        Function to generate equations corresponding to given node
        Arguments:
            nodeName: (str) for which to generate equations
        """
        # Variables that are inputs are not generated from equations
        if nodeName in self.inputNames:
            return
        
        node = self.getNode(nodeName)

        if node.op_type == 'MatMul':
            self.matMulEquations(node)
        elif node.op_type == 'Add':
            self.addEquations(node)
        elif node.op_type == 'Relu':
            self.reluEquations(node)
        else:
            print("Operation %s not implemented" % (node.op_type))
            raise NotImplementedError
        
    def getNode(self, nodeName):
        """
        Find the node in the graph corresponding to the given name
        Arguments:
            nodeName: (str) name of node to find in graph
        Returns:
            ONNX node named nodeName
        """
        node = [node for node in self.graph.node if nodeName in node.output]
        if len(node) > 0:
            return node[0]
        return None
    
    def makeNewVariables(self, nodeName):
        """
        Assuming the node's shape is known, return a set of new variables in the same shape
        Arguments:
            nodeName: (str) name of node
        Returns:
            v: (np.array) array of variable numbers 
        """
        assert nodeName not in self.varMap
        shape = self.shapeMap[nodeName]
        size = np.prod(shape)
        v = np.array([self.getNewVariable() for _ in range(size)]).reshape(shape)
        self.varMap[nodeName] = v
        assert all([np.equal(np.mod(i, 1), 0) for i in v.reshape(-1)]) # check if integers
        return v

    def processInputAndOutput(self):
        """
        Make variables for input/output operations and save them in inputVars/outputVars
        """
        for nodeName in self.inputNames:
            self.makeNewVariables(nodeName)
        self.inputVars = [np.array(self.varMap[name]) for name in self.inputNames]
        self.madeGraphEquations += self.inputNames
        
        self.makeNewVariables(self.outputName)
        self.outputVars = self.varMap[self.outputName]
        
    def getInputNodes(self, nodeName, saveConstant = False):
        """
        Get names of nodes that are inputs to the given node
        Arguments:
            nodeName: (str) name of node
            saveConstant: (bool) if true, save constant variables to self.constantMap
        Returns:
            inNodes: (list of str) names of nodes that are inputs to the given node
        """
        node = self.getNode(nodeName)
        inNodes = []
        for inp in node.input:
            if len([nde for nde in self.graph.node if inp in nde.output]):
                inNodes += [inp]
            elif saveConstant and len([nde for nde in self.graph.initializer if nde.name == inp]):
                self.constantMap[inp] = [numpy_helper.to_array(init) for init in self.graph.initializer if init.name == inp][0]
        return inNodes
        
    def matMulEquations(self, node):
        """
        Function to generate equations corresponding to matrix multiplication
        Arguments:
            node: (node) representing the MatMul operation
        """
        # Get inputs and determine which inputs are constants and which are variables
        inputName1, inputName2 = node.input
        shape1 = self.shapeMap[inputName1]
        shape2 = self.shapeMap[inputName2]
        firstInputConstant = False; secondInputConstant = False
        if inputName1 in self.constantMap:
            input1 = self.constantMap[inputName1]
            firstInputConstant = True
        else:
            input1 = self.varMap[inputName1]
            
        if inputName2 in self.constantMap:
            input2 = self.constantMap[inputName2]
            secondInputConstant = True
        else:
            input2 = self.varMap[inputName2]
            
        # Assume that exactly one input is variable and one input is a constant
        assert (firstInputConstant or secondInputConstant) and not (firstInputConstant and secondInputConstant)
        
        # Pad shape if needed
        if len(shape2) == 1:
            shape2 += [1]
            input2 = input2.reshape(shape2)
            
        # Create new variables
        outputVariables = self.makeNewVariables(node.output[0])

        # Generate equations
        for i in range(shape1[0]):
            for j in range(shape2[1]):
                e = MarabouUtils.Equation()
                for k in range(shape1[1]):
                    if firstInputConstant:
                        e.addAddend(input1[i][k], input2[k][j])
                    else:
                        e.addAddend(input2[k][j], input1[i][k])
                
                # Put output variable as the last addend last
                e.addAddend(-1, outputVariables[i][j])
                e.setScalar(0.0)
                self.addEquation(e)
    
    def addEquations(self, node):
        """
        Function to generate equations corresponding to addition
        Arguments:
            node: (node) representing the Add operation
        """
        # Get the inputs
        inputName1, inputName2 = node.input
        outputName = node.output[0]
        shape1 = self.shapeMap[inputName1]
        shape2 = self.shapeMap[inputName2]
        assert shape1 == shape2
        
        # Decide which inputs are variables and which are constants
        firstInputConstant = False; secondInputConstant = False
        if inputName1 in self.constantMap:
            input1 = self.constantMap[inputName1]
            firstInputConstant = True
        else:
            input1 = self.varMap[inputName1]
            
        if inputName2 in self.constantMap:
            input2 = self.constantMap[inputName2]
            secondInputConstant = True
        else:
            input2 = self.varMap[inputName2]
            
        # If both inputs to add are constant, then the output is constant too
        # No new variables are needed, we just need to store the output in constantMap
        if firstInputConstant and secondInputConstant:
            self.constantMap[outputName] = input1 + input2
        
        # If both inputs are variables, then we need a new variable to represent
        # the sum of the two variables
        elif not firstInputConstant and not secondInputConstant:
            outputVariables = self.makeNewVariables(outputName)
            input1 = input1.reshape(-1)
            input2 = input2.reshape(-1)
            outputVariables = outputVariables.reshape(-1)
            for i in range(len(input1)):
                e = MarabouUtils.Equation()
                e.addAddend(1, input1[i])
                e.addAddend(1, input2[i])
                e.addAddend(-1, outputVariables[i])
                e.setScalar(0.0)
                self.addEquation(e)
        
        # Otherwise, we are adding constants to variables.
        # We don't need new equations or new variables. We can just edit the scalar term 
        # of existing equations in order to represent the bias addition.
        else:
            if firstInputConstant:
                constInput = input1
                varInput = input2
            else:
                constInput = input2
                varInput = input1
                
            # If this node is the final output, then we created too many variables
            # Delete the extra variables if so
            if outputName != self.outputName:
                self.varMap[outputName] = varInput
            else:
                outVars = self.varMap[outputName].reshape(-1)
                self.numVars -= len(outVars)
             
            constInput = constInput.reshape(-1)
            varInput = varInput.reshape(-1)
        
            # Adjust equations to incorporate the constant addition
            for equ in self.equList:
                (c,var) = equ.addendList[-1]
                assert c == -1
                if var in varInput:
                    ind = np.where(var == varInput)[0][0]
                    
                    # Adjust the equation
                    equ.setScalar(equ.scalar-constInput[ind])
                    
                    # Replace the output variable of the equation with the overall output variable if necessary
                    if outputName == self.outputName:
                        equ.addendList[-1] = (c, outVars[ind])
    
    def reluEquations(self, node):
        """
        Function to generate equations corresponding to pointwise Relu
        Arguments:
            node: (node) representing the Relu operation
        """
        # Get variables
        inputName = node.input[0]
        outputName = node.output[0]
        inputVars = self.varMap[inputName].reshape(-1)
        outputVars = self.makeNewVariables(outputName).reshape(-1)
        assert len(inputVars) == len(outputVars)

        # Generate equations
        for i in range(len(inputVars)):
            self.addRelu(inputVars[i], outputVars[i])
        for f in outputVars:
            self.setLowerBound(f, 0.0)
    
    def evaluateWithoutMarabou(self, inputValues):
        """
        Try to evaluate the network with the given inputs
        Arguments:
            inputValues: (list of np.arrays) input values representing input to network
        Returns:
            Output values of neural network
        """
        # Check that all input variables are designated as inputs in the graph
        # Unlike Tensorflow, ONNX only allows assignment of values to input/output nodes
        onnxInputNames = [node.name for node in self.graph.input]
        for inName in self.inputNames:
            if inName not in onnxInputNames:
                print("ONNX does not allow intermediate layers to be set as inputs!")
                raise NotImplementedError
        
        # Check that the output variable is designated as an output in the graph
        # Unlike Tensorflow, ONNX only allows assignment of values to input/output nodes
        onnxOutputNames = [node.name for node in self.graph.output]
        if self.outputName not in onnxOutputNames:
            print("ONNX does not allow intermediate layers to be set as the output!")
            raise NotImplementedError
        
        # Use onnxruntime session to evaluate the point
        sess = onnxruntime.InferenceSession(self.filename)
        input_dict = dict()
        for i, inputName in enumerate(self.inputNames):
            
            # Try to cast input to correct type
            onnxType = sess.get_inputs()[i].type
            if 'float' in onnxType:
                inputType = 'float32'
            elif 'int' in onnxType:
                inputType = 'int64'
            else:
                printf("Not sure how to cast input to graph input of type %s" % onnxType)
                raise NotImplementedError
            input_dict[inputName] = inputValues[i].reshape(self.inputVars[i].shape).astype(inputType)
        return sess.run(self.outputName,input_dict)[0]