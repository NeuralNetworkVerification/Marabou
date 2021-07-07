'''
Top contributors (to current version):
    - Kyle Julian
    
This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

MarabouNetworkONNX represents neural networks with piecewise linear constraints derived from the ONNX format
'''

import numpy as np
import onnx
import onnxruntime
from onnx import numpy_helper
from onnx.helper import get_attribute_value
from maraboupy import MarabouUtils
from maraboupy import MarabouNetwork
from onnx import TensorProto
import itertools

class MarabouNetworkONNX(MarabouNetwork.MarabouNetwork):
    """Constructs a MarabouNetworkONNX object from an ONNX file

    Args:
        filename (str): Path to the ONNX file
        inputNames: (list of str, optional): List of node names corresponding to inputs
        outputName: (string, optional): Name of node corresponding to output

    Returns:
        :class:`~maraboupy.Marabou.marabouNetworkONNX.marabouNetworkONNX`
    """
    def __init__(self, filename, inputNames=None, outputName=None):
        super().__init__()
        self.readONNX(filename, inputNames, outputName)

    def clear(self):
        """Reset values to represent empty network
        """
        super().clear()
        self.madeGraphEquations = []
        self.varMap = dict()
        self.constantMap = dict()
        self.shapeMap = dict()
        self.inputNames = None
        self.outputName = None
        self.graph = None
        
    def readONNX(self, filename, inputNames, outputName):
        """Read an ONNX file and create a MarabouNetworkONNX object

        Args:
            filename: (str): Path to the ONNX file
            inputNames: (list of str): List of names corresponding to inputs
            outputName: (str): Name of node corresponding to output
            
        :meta private:
        """
        self.filename = filename
        self.graph = onnx.load(filename).graph
        
        # Get default inputs/output if no names are provided
        if not inputNames:
            assert len(self.graph.input) >= 1
            initNames =  [node.name for node in self.graph.initializer]
            inputNames = [inp.name for inp in self.graph.input if inp.name not in initNames]
        if not outputName:
            if len(self.graph.output) > 1:
                err_msg = "Your model has multiple outputs defined\n"
                err_msg += "Please specify the name of the output you want to consider using the 'outputName' argument\n"
                err_msg += "Possible options: " + ", ".join([out.name for out in self.graph.output])
                raise RuntimeError(err_msg)
                
            outputName = self.graph.output[0].name
            
        # Check that input/outputs are in the graph
        for name in inputNames:
            if not len([nde for nde in self.graph.node if name in nde.input]):
                raise RuntimeError("Input %s not found in graph!" % name)
        if not len([nde for nde in self.graph.node if outputName in nde.output]):
            raise RuntimeError("Output %s not found in graph!" % outputName)
            
        self.inputNames = inputNames
        self.outputName = outputName
        
        # Process the shapes and values of the graph while making Marabou equations and constraints 
        self.foundnInputFlags = 0
        self.processGraph()
        
        # If the given inputNames/outputName specify only a portion of the network, then we will have
        # shape information saved not relevant to the portion of the network. Remove extra shapes.
        self.cleanShapes()
        
        # Other Marabou input parsers assign output variables immediately after input variables and before any
        # intermediate variables. This function reassigns variable numbering to match other parsers.
        # If this is skipped, the output variables will be the last variables defined.
        self.reassignOutputVariables()
        
    def processGraph(self):
        """Processes the ONNX graph to produce Marabou equations
        
        :meta private:
        """
        # Add shapes for the graph's inputs
        for node in self.graph.input:
            self.shapeMap[node.name] = list([dim.dim_value if dim.dim_value > 0 else 1 for dim in node.type.tensor_type.shape.dim])
            
            # If we find one of the specified inputs, create new variables
            if node.name in self.inputNames:
                self.madeGraphEquations += [node.name]
                self.foundnInputFlags += 1
                self.makeNewVariables(node.name)
                self.inputVars += [np.array(self.varMap[node.name])] 
                
        # Add shapes for constants
        for node in self.graph.initializer:
            self.shapeMap[node.name] = list(node.dims)
            self.madeGraphEquations += [node.name]
            
        # Recursively create remaining shapes and equations as needed
        self.makeGraphEquations(self.outputName, True)

    def makeGraphEquations(self, nodeName, makeEquations):
        """Recursively populates self.shapeMap, self.varMap, and self.constantMap while adding equations and constraints
        
        Args:
            nodeName (str): Name of node for making the shape
            makeEquations (bool): Create Marabou equations for this node if True
            
        :meta private:
        """
        if nodeName in self.madeGraphEquations:
            return
        
        if nodeName in self.inputNames:
            self.foundnInputFlags += 1   
            # If an inputName is an intermediate layer of the network, we don't need to create Marabou
            # equations for its inputs. However, we still need to call makeMarabouEquations in order to
            # compute shapes. We just need to set the makeEquations flag to false
            makeEquations = False
        self.madeGraphEquations += [nodeName]
        
        # Recursively call makeGraphEquations, then call makeMarabouEquations
        # This ensures that shapes and values of a node's inputs have been computed first 
        for inNodeName in self.getInputNodes(nodeName):
            self.makeGraphEquations(inNodeName, makeEquations)
            
        # By this point, all input variables need to have been found
        if self.foundnInputFlags != len(self.inputNames):
            err_msg = "These input variables could not be found: %s"%(", ".join([inVar for inVar in self.inputNames if inVar not in self.varMap]))
            raise RuntimeError(err_msg)
            
        # Compute node's shape and create Marabou equations as needed
        self.makeMarabouEquations(nodeName, makeEquations)
        
        # Create new variables when we find one of the inputs
        if nodeName in self.inputNames:
            self.makeNewVariables(nodeName)
            self.inputVars += [np.array(self.varMap[nodeName])]  
            
    def makeMarabouEquations(self, nodeName, makeEquations):
        """Compute the shape and values of a node assuming the input shapes and values have been computed already.

        Args:
            nodeName (str): Name of node for which we want to compute the output shape
            makeEquations (bool): Create Marabou equations for this node if True
            
        :meta private:
        """
        node = self.getNode(nodeName)
        
        if node.op_type == 'Constant':
            self.constant(node)
        elif node.op_type == 'Identity': 
            self.identity(node)
        elif node.op_type == 'Cast':
            self.cast(node)
        elif node.op_type == 'Reshape':
            self.reshape(node)
        elif node.op_type == 'Flatten':
            self.flatten(node)
        elif node.op_type == "Transpose":
            self.transpose(node)
        elif node.op_type == "MaxPool":
            self.maxpoolEquations(node, makeEquations)
        elif node.op_type == "Conv":
            self.convEquations(node, makeEquations)
        elif node.op_type == 'Gemm':
            self.gemmEquations(node, makeEquations)
        elif node.op_type == 'MatMul':
            self.matMulEquations(node, makeEquations)
        elif node.op_type == 'Add':
            self.addEquations(node, makeEquations)
        elif node.op_type == 'Relu': 
            self.reluEquations(node, makeEquations)
        else:
            raise NotImplementedError("Operation %s not implemented" % (node.op_type))      
    
    def getNode(self, nodeName):
        """Find the node in the graph corresponding to the given name

        Args:
            nodeName (str): Name of node to find in graph
            
        Returns:
            (str): ONNX node named nodeName

        :meta private:
        """
        nodes = [node for node in self.graph.node if nodeName in node.output]
        assert len(nodes) == 1
        return nodes[0]
    
    def makeNewVariables(self, nodeName):
        """Assuming the node's shape is known, return a set of new variables in the same shape

        Args:
            nodeName (str): Name of node

        Returns:
            (numpy array): Array of variable numbers 

        :meta private:
        """
        assert nodeName not in self.varMap
        shape = self.shapeMap[nodeName]
        size = np.prod(shape)
        v = np.array([self.getNewVariable() for _ in range(size)]).reshape(shape)
        self.varMap[nodeName] = v
        assert all([np.equal(np.mod(i, 1), 0) for i in v.reshape(-1)]) # check if integers
        return v
        
    def getInputNodes(self, nodeName):
        """Get names of nodes that are inputs to the given node

        Args:
            nodeName (str): Name of node
            saveConstant (bool): If true, save constant variables to self.constantMap

        Returns:
            (list of str): Names of nodes that are inputs to the given node

        :meta private:
        """
        node = self.getNode(nodeName)
        inNodes = []
        for inp in node.input:
            if len([nde for nde in self.graph.node if inp in nde.output]):
                inNodes += [inp]
            elif len([nde for nde in self.graph.initializer if nde.name == inp]):
                self.constantMap[inp] = [numpy_helper.to_array(init) for init in self.graph.initializer if init.name == inp][0]
        return inNodes
        
    def constant(self, node):
        """Function representing a constant tensor

        Args:
            node (node): ONNX node representing constant operation

        :meta private:
        """
        nodeName = node.output[0]
        for attr in node.attribute:
            if attr.name == "value":
                self.constantMap[nodeName] = numpy_helper.to_array(get_attribute_value(attr))
                return
        raise RuntimeError("Could not find value of tensor constant")
        
    def identity(self, node):
        """Function representing identity

        Args:
            node (node): ONNX node representing identity operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        self.shapeMap[nodeName] = self.shapeMap[inputName]
        if inputName in self.varMap:
            self.varMap[nodeName] = self.varMap[inputName]
        elif inputName in self.constantMap:
            self.constantMap[nodeName] = self.constantMap[inputName]
            
    def cast(self, node):
        """Function representing cast

        Args:
            node (node): ONNX node representing cast operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        self.shapeMap[nodeName] = self.shapeMap[inputName]
        
        # Try to find type to cast to. If not found, raise error
        to = None
        for attr in node.attribute:
            if attr.name == "to":
                to = get_attribute_value(attr)
        if to is None:
            raise RuntimeError("Casting type not specified with attribute 'to'")
            
        # Cast input array to correct type, and throw error if type is unknown
        if inputName in self.constantMap:
            if to == TensorProto.FLOAT16:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('float16')
            elif to == TensorProto.FLOAT:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('float32')
            elif to == TensorProto.DOUBLE:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('double')
            elif to == TensorProto.UINT8:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('uint8')
            elif to == TensorProto.UINT16:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('uint16')
            elif to == TensorProto.UINT32:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('uint32')
            elif to == TensorProto.UINT64:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('uint64')
            elif to == TensorProto.INT8:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('int8')
            elif to == TensorProto.INT16:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('int16')
            elif to == TensorProto.INT32:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('int32')
            elif to == TensorProto.INT64:
                self.constantMap[nodeName] = self.constantMap[inputName].astype('int64')
            else:
                err_msg = "Unknown type for casting: %d\n" % to
                err_msg += "Check here for ONNX TensorProto: https://github.com/onnx/onnx/blob/master/onnx/onnx.proto"
                raise NotImplementedError(err_msg)
                
        # We shouldn't be casting variables to different types, since Marabou assumes variables have double precision
        elif inputName in self.varMap:
            raise NotImplementedError("Casting variables not allowed with Marabou")

    def reshape(self, node):
        """Function representing reshape

        Args:
            node (node): ONNX node representing reshape operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName1, inputName2 = node.input
        
        # Assume first input is array to be reshaped, second input is the new shape array
        reshapeVals = self.constantMap[inputName2]
        self.shapeMap[nodeName] = list(np.zeros(self.shapeMap[inputName1]).reshape(reshapeVals).shape)
        if inputName1 in self.varMap:
            self.varMap[nodeName] = self.varMap[inputName1].reshape(reshapeVals)
        elif inputName1 in self.constantMap:
            self.constantMap[nodeName] = self.constantMap[inputName1].reshape(reshapeVals)

    def flatten(self, node):
        """Function representing flatten

        Unlike numpy.flatten(), ONNX's Flatten operation reshapes
        a (d_0, d_1, ..., d_n) tensor into a 2D tensor with shape
        (d_0 * d_1 * ... * d_(axis-1), d_axis * d_(axis+1) * ... * d_n).

        Args:
            node (node): ONNX node representing flatten operation

        :meta private:
        """
        nodeName = node.output[0]

        # Assume first input is array to be flattened
        inputName = node.input[0]
        axis = 1
        for attr in node.attribute:
            if attr.name == "axis":
                axis = get_attribute_value(attr)

        dimension1 = int(np.prod(self.shapeMap[inputName][:axis]))
        dimension2 = int(np.prod(self.shapeMap[inputName][axis:]))
        newShape = [dimension1, dimension2]
        self.shapeMap[nodeName] = newShape

        if inputName in self.varMap:
            self.varMap[nodeName] = self.varMap[inputName].reshape(newShape)
        elif inputName in self.constantMap:
            self.constantMap[nodeName] = self.constantMap[inputName].reshape(newShape)
            
    def transpose(self, node):
        """Function representing transpose

        Args:
            node (node): ONNX node representing transpose operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        
        # Get attributes
        perm = None
        for attr in node.attribute:
            if attr.name == "perm":
                perm = get_attribute_value(attr)
        if perm is None:
            raise RuntimeError("Permutation indices not specified by attibute 'perm'")
        self.shapeMap[nodeName] = [self.shapeMap[inputName][p] for p in perm]
        if inputName in self.varMap:
            self.varMap[nodeName] = np.transpose(self.varMap[node.input[0]], perm)
        elif inputName in self.constantMap:
            self.constantMap[nodeName] = np.transpose(self.constantMap[inputName], perm)
    
    def maxpoolEquations(self, node, makeEquations):
        """Function to generate maxpooling equations

        Args:
            node (node): ONNX node representing maxpool operation
            makeEquations (bool): True if we need to create new variables and maxpool constraints

        :meta private:
        """
        nodeName = node.output[0]
        
        # Extract attributes and define shape
        inputShape = self.shapeMap[node.input[0]]
        kernel_shape = [1, 1]
        strides = [1, 1]
        for attr in node.attribute:
            if attr.name == 'kernel_shape':
                kernel_shape = get_attribute_value(attr)
            elif attr.name == 'strides':
                strides = get_attribute_value(attr)
                
        outputShape = [dim for dim in inputShape]
        outputShape[2] = int(np.ceil((inputShape[2] - ((kernel_shape[0] - 1) + 1) + 1) / strides[0]))
        outputShape[3] = int(np.ceil((inputShape[3] - ((kernel_shape[1] - 1) + 1) + 1) / strides[1]))
        self.shapeMap[nodeName] = outputShape
        
        if not makeEquations:
            return
        
        inVars = self.varMap[node.input[0]]
        outVars = self.makeNewVariables(nodeName)
        for i in range(outputShape[2]):
            for j in range(outputShape[3]):
                for k in range(outputShape[1]):
                    maxVars = set()
                    for di in range(strides[0]*i, strides[0]*i + kernel_shape[0]):
                        for dj in range(strides[1]*j, strides[1]*j + kernel_shape[1]):
                            if di < inputShape[2] and dj < inputShape[3]:
                                maxVars.add(inVars[0][k][di][dj])
                    self.addMaxConstraint(maxVars, outVars[0][k][i][j])
        
    def convEquations(self, node, makeEquations):
        """Function to generate equations for a 2D convolution

        Args:
            node (node): ONNX node representing the 2D Convolution operation
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """
        nodeName = node.output[0]
        
        # Extract information about convolution
        strides = [1, 1]
        pads = [0, 0, 0, 0]
        for attr in node.attribute:
            if attr.name == 'strides':
                strides = get_attribute_value(attr)
            elif attr.name == 'pads':
                pads = get_attribute_value(attr)
        pad_left, pad_bottom, pad_right, pad_top = pads
                
        # Get input shape information
        # First input should be variable tensor, the second a weight matrix defining filters
        shape0 = self.shapeMap[node.input[0]]
        shape1 = self.shapeMap[node.input[1]]
        input_channels = shape0[1]
        input_width = shape0[2]
        input_height = shape0[3]
        num_filters = shape1[0]
        filter_channels = shape1[1]
        filter_width = shape1[2]
        filter_height = shape1[3]
        
        # The third input is optional and specifies a bias for each filter
        # Bias is 0 if third input is not given
        biases = np.zeros(num_filters)
        if len(node.input) == 3:
            biases = self.constantMap[node.input[2]]

        # The number of channels should match between input variable and filters
        assert input_channels == filter_channels

        # Compute output shape
        out_width = (input_width - filter_width + pad_left + pad_right) // strides[0] + 1
        out_height = (input_height - filter_height + pad_bottom + pad_top) // strides[1] + 1
        out_channels = num_filters
        self.shapeMap[nodeName] = [shape0[0], out_channels, out_width, out_height]
        
        if not makeEquations:
            return
        
        inVars = self.varMap[node.input[0]]
        weights = self.constantMap[node.input[1]]
        outVars = self.makeNewVariables(nodeName)

        ### Generate actual equations ###
        # There is one equation for every output variable
        for i in range(out_width):
            for j in range(out_height):
                for k in range(out_channels): # Out_channel corresponds to filter number
                    e = MarabouUtils.Equation()

                    # The equation convolves the filter with the specified input region
                    # Iterate over the filter
                    for di in range(filter_width):
                        for dj in range(filter_height):
                            for dk in range(filter_channels):
                                w_ind = int(strides[0]*i+di - pad_left)
                                h_ind = int(strides[1]*j+dj - pad_top)
                                if h_ind < input_height and h_ind >= 0 and w_ind < input_width and w_ind >= 0:
                                    var = inVars[0][dk][w_ind][h_ind]
                                    c = weights[k][dk][di][dj]
                                    e.addAddend(c, var)

                    # Add output variable
                    e.addAddend(-1, outVars[0][k][i][j])
                    e.setScalar(-biases[k])
                    self.addEquation(e)
        
    def gemmEquations(self, node, makeEquations):  
        """Function to generate equations corresponding to Gemm (general matrix multiplication)

        Args:
            node (node): ONNX node representing the Gemm operation
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """  
        nodeName = node.output[0]
        
        # Get inputs
        inputName1, inputName2, inputName3 = node.input
        shape1 = self.shapeMap[inputName1]
        shape2 = self.shapeMap[inputName2]
        
        # Transpose first two inputs if needed,
        # and save scaling parameters alpha and beta if set
        alpha = 1.0
        beta = 1.0
        transA = 0
        transB = 0
        for attr in node.attribute:
            if attr.name == 'transA':
                transA = get_attribute_value(attr)
            elif attr.name == 'transB':
                transB = get_attribute_value(attr)
            elif attr.name == 'alpha':
                alpha = get_attribute_value(attr)
            elif attr.name == 'beta':
                beta = get_attribute_value(attr)
        
        if transA:
            shape1 = shape1[::-1]
        if transB:
            shape2 = shape2[::-1]
        outShape = [shape1[0], shape2[1]]
        self.shapeMap[nodeName] = outShape
        if not makeEquations:
            return
        
        # Assume that first input is variables, second is Matrix for MatMul, and third is bias addition
        input1 = self.varMap[inputName1]
        input2 = self.constantMap[inputName2]
        input3 = self.constantMap[inputName3]
        
        # Transpose inputs
        if transA:
            input1 = np.transpose(input1)
        if transB:
            input2 = np.transpose(input2)
        input3 = np.broadcast_to(input3, outShape)
        
        assert shape1[-1] == shape2[0]
        assert shape1[0] == outShape[0]
        assert shape2[1] == outShape[1]

        # Create new variables
        outputVariables = self.makeNewVariables(nodeName)
        # Generate equations
        for i in range(shape1[0]):
            for j in range(shape2[1]):
                e = MarabouUtils.Equation()
                for k in range(shape1[1]):
                    e.addAddend(input2[k][j]*alpha, input1[i][k])

                # Put output variable as the last addend last
                e.addAddend(-1, outputVariables[i][j])
                e.setScalar(-input3[i][j]*beta)
                self.addEquation(e)
    
    def matMulEquations(self, node, makeEquations):
        """Function to generate equations corresponding to matrix multiplication

        Args:
            node (node): ONNX node representing the MatMul operation
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """
        nodeName = node.output[0]
        
        # Get inputs and determine which inputs are constants and which are variables
        inputName1, inputName2 = node.input
        shape1 = self.shapeMap[inputName1]
        shape2 = self.shapeMap[inputName2]
        assert shape1[-1] == shape2[0]
        self.shapeMap[nodeName] = shape1[:-1] + shape2[1:]
        if not makeEquations:
            return
            
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
            
        # Broadcast first input to make sure the first input is a matrix
        if len(shape1) == 1:
            shape1 = [1] + shape1
            input1 = input1.reshape(shape1)

        # Assume that at least one input is a constant (We cannot represent variable products with linear equations)
        assert firstInputConstant or secondInputConstant

        # If both inputs are constant, than the output is constant as well, and we don't need new variables or equations
        if firstInputConstant and secondInputConstant:
            self.constantMap[nodeName] = np.matmul(input1,input2)
            return

        # Create new variables
        outputVariables = self.makeNewVariables(nodeName)

        # Pad the output if needed (matrix-matrix multiplication)
        if len(outputVariables.shape) == 1 and len(shape2) > 1:
            outputVariables = outputVariables.reshape([1, outputVariables.shape[0]])

        # Generate equations
        for i in range(shape1[0]):
            # Differentiate between matrix-vector multiplication and matrix-matrix multiplication
            if len(shape2)>1:
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
            else:
                e = MarabouUtils.Equation()
                for k in range(shape1[1]):
                    if firstInputConstant:
                        e.addAddend(input1[i][k], input2[k])
                    else:
                        e.addAddend(input2[k], input1[i][k])

                # Put output variable as the last addend last
                e.addAddend(-1, outputVariables[i])
                e.setScalar(0.0)
                self.addEquation(e)
    
    def addEquations(self, node, makeEquations):
        """Function to generate equations corresponding to addition

        Args:
            node (node): ONNX node representing the Add operation
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """
        nodeName = node.output[0]
        
        # Get the inputs
        inputName1, inputName2 = node.input
        shape1 = self.shapeMap[inputName1]
        shape2 = self.shapeMap[inputName2]
        
        # Get the broadcasted shape
        outShape = getBroadcastShape(shape1, shape2)
        self.shapeMap[nodeName] = outShape
        if not makeEquations:
            return
        
        # Decide which inputs are variables and which are constants
        firstInputConstant = False; secondInputConstant = False
        if inputName1 in self.constantMap:
            firstInputConstant = True
            input1 = self.constantMap[inputName1]
        else:
            input1 = self.varMap[inputName1]
            
        if inputName2 in self.constantMap:
            secondInputConstant = True
            input2 = self.constantMap[inputName2]
        else:
            input2 = self.varMap[inputName2]
            
        # Broadcast inputs to ensure the shapes match
        input1 = np.broadcast_to(input1, outShape)
        input2 = np.broadcast_to(input2, outShape)
        
        # The shape after broadcasting must match
        assert input1.shape == input2.shape
            
        # If both inputs to add are constant, then the output is constant too
        # No new variables are needed, we just need to store the output in constantMap
        if firstInputConstant and secondInputConstant:
            self.constantMap[nodeName] = input1 + input2
            return
        
        # If both inputs are variables, then we need a new variable to represent
        # the sum of the two variables
        elif not firstInputConstant and not secondInputConstant:
            outputVariables = self.makeNewVariables(nodeName)
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
            return
        
        # Otherwise, we are adding constants to variables.
        # We don't need new equations or new variables if the input variable is the output of a linear equation.
        # Instead, we can just edit the scalar term of the existing linear equation.
        # However, if the input variables are not outputs of linear equations (input variables or outputs of 
        # activation functions) then we will need new equations.
        if firstInputConstant:
            constInput = input1
            varInput = input2
        else:
            constInput = input2
            varInput = input1
        constInput = constInput.reshape(-1)
        varInput = varInput.reshape(-1)

        # Adjust equations to incorporate the constant addition
        numEquationsChanged = 0
        for equ in self.equList:
            (c,var) = equ.addendList[-1]
            assert c == -1
            if var in varInput:
                ind = np.where(var == varInput)[0][0]

                # Adjust the equation
                equ.setScalar(equ.scalar-constInput[ind])
                numEquationsChanged += 1

        # If we changed one equation for every input variable, then
        # we don't need any new equations
        if numEquationsChanged == len(varInput):
            self.varMap[nodeName] = varInput
        else:
            # Otherwise, assert no equations were changed, and we need to create new equations
            assert numEquationsChanged == 0
            outputVariables = self.makeNewVariables(nodeName).reshape(-1)
            for i in range(len(outputVariables)):
                e = MarabouUtils.Equation()
                e.addAddend(1, varInput[i])
                e.addAddend(-1, outputVariables[i])
                e.setScalar(-constInput[i])
                self.addEquation(e)
    
    def reluEquations(self, node, makeEquations):
        """Function to generate equations corresponding to pointwise Relu

        Args:
            node (node): ONNX node representing the Relu operation
            makeEquations (bool): True if we need to create new variables and add new Relus

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        self.shapeMap[nodeName] = self.shapeMap[inputName] 
        if not makeEquations:
            return
        
        # Get variables
        inputVars = self.varMap[inputName].reshape(-1)
        outputVars = self.makeNewVariables(nodeName).reshape(-1)
        assert len(inputVars) == len(outputVars)

        # Generate equations
        for i in range(len(inputVars)):
            self.addRelu(inputVars[i], outputVars[i])
        for f in outputVars:
            self.setLowerBound(f, 0.0)
                     
    def cleanShapes(self):
        """Remove unused shapes
        
        After constructing equations, remove shapes from self.shapeMap that are part of the graph but not
        relevant for this input query. This is only cosmetic and does not impact Marabou.

        :meta private:
        """
        for nodeName in [name for name in self.shapeMap]:
            if nodeName not in self.varMap and nodeName not in self.constantMap:
                self.shapeMap.pop(nodeName)
                
    def reassignVariable(self, var, numInVars, outVars, newOutVars):
        """Reassign variable so that output variables follow input variables

        This function computes what the given variable should be when the output variables are 
        moved to come after the input variables.

        Args:
            var (int): Original variable number
            numInVars (int): Number of input variables
            outVars (numpy array of int): Original output variables
            newOutVars (numpy array of int): New output variables

        Returns:
            (int): New variable assignment

        :meta private:
        """
        if var < numInVars:
            return var
        if var in outVars:
            ind = np.where(var == outVars)[0][0]
            return newOutVars[ind]
        return var + len(outVars)
    
    def reassignOutputVariables(self):
        """Reassign output variables so output variable numbers follow input variable numbers
        
        Other input parsers assign output variables after input variables and before any intermediate variables.
        This function reassigns the numbers for the output variables and shifts all other variables up to make space.

        :meta private:
        """
        if self.outputName in self.constantMap:
            raise RuntimeError("Output variable %s is a constant, not the output of equations!"%self.outputName)
        outVars = self.varMap[self.outputName].reshape(-1)
        numInVars = np.sum([np.prod(self.shapeMap[inputName]) for inputName in self.inputNames])
        numOutVars = len(outVars)
        newOutVars = np.array(range(numInVars,numInVars+numOutVars))
        
        # Adjust equation variables
        for eq in self.equList:
            for i, (c,var) in enumerate(eq.addendList):
                eq.addendList[i] = (c, self.reassignVariable(var, numInVars, outVars, newOutVars))
                
        # Adjust relu list
        for i, variables in enumerate(self.reluList):
            self.reluList[i] = tuple([self.reassignVariable(var, numInVars, outVars, newOutVars) for var in variables])
        
        # Adjust max pool list
        for i, (elements, outVar) in enumerate(self.maxList):
            newOutVar = self.reassignVariable(outVar, numInVars, outVars, newOutVars)
            newElements = set()
            for var in elements:
                newElements.add(self.reassignVariable(var, numInVars, outVars, newOutVars))
            self.maxList[i] = (newElements, newOutVar)
            
        # Adjust upper/lower bounds
        newLowerBounds = dict()
        newUpperBounds = dict()
        for var in self.lowerBounds:
            newLowerBounds[self.reassignVariable(var, numInVars, outVars, newOutVars)] = self.lowerBounds[var]
        for var in self.upperBounds:
            newUpperBounds[self.reassignVariable(var, numInVars, outVars, newOutVars)] = self.upperBounds[var]
        self.lowerBounds = newLowerBounds
        self.upperBounds = newUpperBounds

        # Assign output variables to the new array
        for nodeName, variables in self.varMap.items():
            self.varMap[nodeName] = np.vectorize(self.reassignVariable, excluded=[1,2,3])(variables, numInVars, outVars, newOutVars)
        self.outputVars = self.varMap[self.outputName] 
    
    def evaluateWithoutMarabou(self, inputValues):
        """Try to evaluate the network with the given inputs using ONNX

        Args:
            inputValues (list of numpy array): Input values representing inputs to network

        Returns:
            (numpy array): Output values of neural network
        """
        # Check that all input variables are designated as inputs in the graph
        # Unlike Tensorflow, ONNX only allows assignment of values to input/output nodes
        onnxInputNames = [node.name for node in self.graph.input]
        for inName in self.inputNames:
            if inName not in onnxInputNames:
                raise NotImplementedError("ONNX does not allow intermediate layers to be set as inputs!")
        
        # Check that the output variable is designated as an output in the graph
        # Unlike Tensorflow, ONNX only allows assignment of values to input/output nodes
        onnxOutputNames = [node.name for node in self.graph.output]
        if self.outputName not in onnxOutputNames:
            raise NotImplementedError("ONNX does not allow intermediate layers to be set as the output!")
        
        initNames =  [node.name for node in self.graph.initializer]
        graphInputs = [inp.name for inp in self.graph.input if inp.name not in initNames]
        if len(inputValues) != len(graphInputs):
            raise RuntimeError("There are %d inputs to network, but only %d input arrays were given."%(len(graphInputs), len(inputValues)))
            
        # Use onnxruntime session to evaluate the point
        sess = onnxruntime.InferenceSession(self.filename)
        input_dict = dict()
        for i, inputName in enumerate(self.inputNames):
            
            # Try to cast input to correct type
            onnxType = sess.get_inputs()[i].type
            if 'float' in onnxType:
                inputType = 'float32'
            else:
                raise NotImplementedError("Inputs to network expected to be of type 'float', not %s" % onnxType)
            input_dict[inputName] = inputValues[i].reshape(self.inputVars[i].shape).astype(inputType)
        return sess.run([self.outputName],input_dict)[0]

def getBroadcastShape(shape1, shape2):
    """Helper function to get the shape that results from broadcasting these shapes together

    Args:
        shape1 (list of int): First shape
        shape2 (list of int): Second shape

    Returns:
        (list of int): Broadcast shape

    :meta private:
    """
    return [l1 if l1 == l2 else max(l1, l2) for l1, l2 in itertools.zip_longest(shape1[::-1], shape2[::-1], fillvalue=1)][::-1]
