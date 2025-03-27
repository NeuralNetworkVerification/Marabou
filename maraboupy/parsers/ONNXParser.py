'''
Top contributors (to current version):
    - Kyle Julian
    - Haoze Wu
    - Teruhiro Tagomori
    - Tobey Shim
    - Idan Refaeli

This file is part of the Marabou project.
Copyright (c) 2017-2024 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

MarabouNetworkONNX represents neural networks with piecewise linear constraints derived from the ONNX format
'''
from typing import List
import numpy as np
from onnx import numpy_helper
from onnx.helper import get_attribute_value
from maraboupy import MarabouUtils
from maraboupy.parsers.InputQueryBuilder import InputQueryBuilder
from onnx import TensorProto
import itertools
from copy import copy
from onnx.reference.ops._op_list import Split_18, Unsqueeze_1, Slice_10

class ONNXParser:
    """
    Class for parsing ONNX files.
    Should eventually be implemented by a renamed `ONNXParser.cpp` on the C++ side.
    """

    @staticmethod
    def parse(query:InputQueryBuilder, graph, inputNames:List[str], outputNames:List[str]):
        """
        Parses the provided ONNX graph into constraints which are stored in the query argument.

        Args:
            query: the query to which the constraints are added.
            graph: the graph of the ONNX file to parse.
            inputNames: list of node names corresponding to inputs
            outputNames: list of node names corresponding to outputs

        Returns:
            :class:`~maraboupy.Marabou.marabouNetworkONNX.marabouNetworkONNX`
        """
        parser = ONNXParser(query, graph, inputNames, outputNames)
        parser.parseGraph()


    def __init__(self, query:InputQueryBuilder, graph, inputNames, outputNames):
        """
        Should not be called directly. Use `ONNXParser.parse` instead.

        :meta private:
        """
        super().__init__()
        self.query = query
        self.graph = graph
        self.inputNames = inputNames
        self.outputNames = outputNames

        self.madeGraphEquations = []
        self.varMap = dict()
        self.constantMap = dict()
        self.shapeMap = dict()


    def parseGraph(self):
        """Read an ONNX file and create a MarabouNetworkONNX object

        :meta private:
        """

        # Process the shapes and values of the graph while making Marabou equations and constraints
        self.foundnInputFlags = 0
        self.processGraph()

        # If the given inputNames/outputNames specify only a portion of the network, then we will have
        # shape information saved not relevant to the portion of the network. Remove extra shapes.
        self.cleanShapes()

        for outputName in self.outputNames:
            if outputName in self.constantMap:
                raise RuntimeError("Output variable %s is a constant, not the output of equations!" % outputName)
        self.query.outputVars.extend([self.varMap[outputName] for outputName in self.outputNames])

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
                self.query.inputVars += [np.array(self.varMap[node.name])]

        # Add shapes for constants
        for node in self.graph.initializer:
            self.shapeMap[node.name] = list(node.dims)
            self.madeGraphEquations += [node.name]

        # Recursively create remaining shapes and equations as needed
        for outputName in self.outputNames:
            self.makeGraphEquations(outputName, True)

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
            self.query.inputVars += [np.array(self.varMap[nodeName])]

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
        elif node.op_type == 'Dropout':
            self.dropout(node)
        elif node.op_type == 'Cast':
            self.cast(node)
        elif node.op_type == 'ConstantOfShape':
            self.constantOfShape(node)
        elif node.op_type == 'Reshape':
            self.reshape(node)
        elif node.op_type == 'Flatten':
            self.flatten(node)
        elif node.op_type == "Transpose":
            self.transpose(node)
        elif node.op_type == 'Unsqueeze':
            self.unsqueeze(node)
        elif node.op_type == 'Squeeze':
            self.squeeze(node)
        elif node.op_type == "Slice":
            self.slice(node)
        elif node.op_type == "BatchNormalization":
            self.batchNorm(node, makeEquations)
        elif node.op_type == 'Concat':
            self.concatEquations(node)
        elif node.op_type == "MaxPool":
            self.maxpoolEquations(node, makeEquations)
        elif node.op_type == "Conv":
            self.convEquations(node, makeEquations)
        elif node.op_type == 'Gemm':
            self.gemmEquations(node, makeEquations)
        elif node.op_type == 'MatMul':
            self.matMulEquations(node, makeEquations)
        elif node.op_type == 'Mul':
            self.mulEquations(node, makeEquations)
        elif node.op_type == 'Add':
            self.addEquations(node, makeEquations)
        elif node.op_type == 'Relu':
            self.reluEquations(node, makeEquations)
        elif node.op_type == 'Sigmoid':
            self.sigmoidEquations(node, makeEquations)
        elif node.op_type == 'Split':
            self.splitEquations(node, nodeName, makeEquations)
        elif node.op_type == 'Resize':
            self.resizeEquations(node, makeEquations)
        elif node.op_type == 'Tanh':
            self.tanhEquations(node, makeEquations)
        elif node.op_type == 'LeakyRelu':
            self.leakyReluEquations(node, makeEquations)
        elif node.op_type == 'Softmax':
            self.softmaxEquations(node, makeEquations)
        elif node.op_type == 'Sub':
            self.subEquations(node, makeEquations)
        else:
            raise NotImplementedError("Operation {} not implemented".format(node.op_type))

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
        v = np.array([self.query.getNewVariable() for _ in range(size)]).reshape(shape)
        self.varMap[nodeName] = v
        assert all([np.equal(np.mod(i, 1), 0) for i in v.reshape(-1)]) # check if integers
        return v

    def getInputNodes(self, nodeName):
        """Get names of nodes that are inputs to the given node

        Args:
            nodeName (str): Name of node

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
                self.shapeMap[nodeName] = self.constantMap[nodeName].shape
                return
        raise RuntimeError("Could not find value of tensor constant")

    def constantOfShape(self, node):
        """Function representing a constant tensor of shape

        Args:
            node (node): ONNX node representing constantOfShape operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        for attr in node.attribute:
            if attr.name == "value":
                value = numpy_helper.to_array(get_attribute_value(attr))
        assert inputName in self.constantMap
        shape = self.constantMap[inputName]
        self.constantMap[nodeName] = np.broadcast_to(value, shape)
        self.shapeMap[nodeName] = shape

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

    def dropout(self, node):
        """Function representing dropout

        Args:
            node (node): ONNX node representing dropout operation

        :meta private:
        """
        if len(node.input) == 3 and self.constantMap[node.input[2]]:
            raise RuntimeError("Marabou only supports training_mode=False in dropout")
        else:
            self.identity(node)

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
            self.varMap[nodeName] = copy(self.varMap[inputName1]).reshape(reshapeVals)
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
            self.varMap[nodeName] = \
            np.transpose(self.varMap[node.input[0]].reshape(self.shapeMap[node.input[0]]),
                         perm)
        elif inputName in self.constantMap:
            self.constantMap[nodeName] = np.transpose(self.constantMap[inputName], perm)

    def slice(self, node):
        nodeName = node.output[0]
        inputName = node.input[0]
        starts = self.constantMap[node.input[1]]
        ends = self.constantMap[node.input[2]]
        axes = self.constantMap[node.input[3]]
        steps = self.constantMap[node.input[4]]

        if inputName in self.varMap:
            output_data = Slice_10.eval(self.varMap[inputName], starts=starts, ends=ends, axes=axes, steps=steps)
            self.shapeMap[nodeName] = output_data.shape
            self.varMap[nodeName] = output_data
        else:
            output_data = Slice_10.eval(self.constantMap[inputName], starts=starts, ends=ends, axes=axes, steps=steps)
            self.shapeMap[nodeName] = output_data.shape
            self.constantMap[nodeName] = output_data

    def unsqueeze(self, node):
        """Function representing unsqueeze

        Args:
            node (node): ONNX node representing unsqueeze operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        axes = self.constantMap[node.input[1]]

        if inputName in self.varMap:
            output_data = Unsqueeze_1.eval(self.varMap[inputName], axes=axes)
            self.shapeMap[nodeName] = output_data.shape
            self.varMap[nodeName] = output_data
        else:
            output_data = Unsqueeze_1.eval(self.constantMap[inputName], axes=axes)
            self.shapeMap[nodeName] = output_data.shape
            self.constantMap[nodeName] = output_data


    def squeeze(self, node):
        """Function representing squeeze

        Args:
            node (node): ONNX node representing squeeze operation

        :meta private:
        """
        nodeName = node.output[0]
        inputName, axisName = node.input

        axis = self.constantMap[axisName]

        axis_ = copy(axis)
        if inputName in self.varMap:
            vars = copy(self.varMap[inputName])
            for i in range(len(axis)):
                vars = np.squeeze(vars, axis_[i])
                for j in range(len(axis))[i+1:]:
                    axis_[j] -= 1
            self.varMap[nodeName] = vars
            self.shapeMap[nodeName] = vars.shape
        return

    def batchNorm(self, node, makeEquations):
        """Function to generate equations for a BatchNormalization

        Args:
            node (node): ONNX node representing the BatchNormalization operation

        :meta private
        """

        nodeName = node.output[0]
        inputName = node.input[0]
        self.shapeMap[nodeName] = self.shapeMap[inputName]

        # Get attributes
        epsilon = None
        for attr in node.attribute:
            if attr.name == "epsilon":
                epsilon = get_attribute_value(attr)

        # Get inputs
        scales = self.constantMap[node.input[1]].reshape(-1)
        biases = self.constantMap[node.input[2]].reshape(-1)
        input_means = self.constantMap[node.input[3]].reshape(-1)
        input_variances = self.constantMap[node.input[4]].reshape(-1)

        if not makeEquations:
            return

        numChannels = len(scales)

        # Get variables
        inputVars = self.varMap[inputName].reshape(numChannels, -1)
        outputVars = self.makeNewVariables(nodeName).reshape(numChannels, -1)
        assert(inputVars.shape == outputVars.shape)

        numInputs = inputVars.shape[1]

        for i in range(numChannels):
            for j in range(numInputs):
                # Add equation
                # To know this computation,
                # refer to https://github.com/onnx/onnx/blob/master/docs/Operators.md#batchnormalization.
                e = MarabouUtils.Equation()
                e.addAddend(1 / np.sqrt(input_variances[i] + epsilon) * scales[i], inputVars[i][j])
                e.addAddend(-1, outputVars[i][j])
                e.setScalar(input_means[i] / np.sqrt(input_variances[i] + epsilon) * scales[i] - biases[i])
                self.query.addEquation(e)

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
                    self.query.addMaxConstraint(maxVars, outVars[0][k][i][j])

    def softmaxEquations(self, node, makeEquations):
        """Function to generate constraints for softmax

        Args:
        node (node): ONNX node representing the softmax operation
        makeEquations (bool): True if we need to create new variables and maxpool constraints

        :meta private:
        """
        nodeName = node.output[0]

        # Extract attributes and define shape
        inputShape = self.shapeMap[node.input[0]]
        for attr in node.attribute:
            if attr.name == 'axis':
                axis = get_attribute_value(attr)

        self.shapeMap[nodeName] = inputShape

        if not makeEquations:
            return

        inVars = self.varMap[node.input[0]]
        outVars = self.makeNewVariables(nodeName)

        if len(inputShape) == 2 and inputShape[0] == 1:
            self.query.addSoftmaxConstraint(list(np.array(inVars).flatten()), list(np.array(outVars).flatten()))
        else:
            axis = ( len(inputShape) + axis ) % len(inputShape)
            perm = []
            for i, s in enumerate(inputShape):
                if i == axis:
                    continue
                perm.append(i)
            perm.append(axis)

            inVarsReshaped = np.transpose(inVars, perm).reshape(-1, inputShape[axis])
            outVarsReshaped = np.transpose(outVars, perm).reshape(-1, inputShape[axis])
            for i in range(inVarsReshaped.shape[0]):
                self.query.addSoftmaxConstraint(inVarsReshaped[i], outVarsReshaped[i])

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
                                h_ind = int(strides[1]*j+dj - pad_bottom)
                                if h_ind < input_height and h_ind >= 0 and w_ind < input_width and w_ind >= 0:
                                    var = inVars[0][dk][w_ind][h_ind]
                                    c = weights[k][dk][di][dj]
                                    e.addAddend(c, var)

                    # Add output variable
                    e.addAddend(-1, outVars[0][k][i][j])
                    e.setScalar(-biases[k])
                    self.query.addEquation(e)

    def gemmEquations(self, node, makeEquations):
        """Function to generate equations corresponding to Gemm (general matrix multiplication)

        Args:
            node (node): ONNX node representing the Gemm operation
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """
        nodeName = node.output[0]

        # Get inputs
        if len(node.input) == 3:
            inputName1, inputName2, inputName3 = node.input
        else:
            inputName1, inputName2 = node.input
            inputName3 = None
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
        if inputName3:
            input3 = self.constantMap[inputName3]

        # Transpose inputs
        if transA:
            input1 = np.transpose(input1)
        if transB:
            input2 = np.transpose(input2)
        if inputName3:
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
                if inputName3:
                    e.setScalar(-input3[i][j]*beta)
                else:
                    e.setScalar(0)
                self.query.addEquation(e)


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
        if len(shape1) > 2 and shape1[0] == 1:
            shape1 = shape1[1:]
        elif len(shape1) == 1:
            # Broadcast first input to make sure the first input is a matrix
            shape1 = [1] + shape1

        if len(shape2) > 2 and shape2[0] == 1:
            shape2 = shape2[1:]
        a = np.zeros(shape1)
        b = np.zeros(shape2)
        c = np.matmul(a, b)
        outshape = c.shape
        self.shapeMap[nodeName] = outshape
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

        input1 = input1.reshape(shape1)
        input2 = input2.reshape(shape2)

        # If both inputs are constant, than the output is constant as well, and we don't need new variables or equations
        if firstInputConstant and secondInputConstant:
            self.constantMap[nodeName] = np.matmul(input1,input2)
            return

        # Create new variables
        outputVariables = self.makeNewVariables(nodeName)

        if not firstInputConstant and not secondInputConstant:
            # bi-linear constraints
            self.addBilinearConstraints(shape1, shape2, input1, input2, outputVariables)
        else:
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
                        self.query.addEquation(e)
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
                    self.query.addEquation(e)

    def addBilinearConstraints(self, shape1, shape2, input1, input2, outputVariables):
        # Generate equations
        if len(shape2) == 2:
            for i in range(shape1[0]):
                for j in range(shape2[1]):
                    e = MarabouUtils.Equation()
                    for k in range(shape1[1]):
                        v = self.query.getNewVariable()
                        self.query.addBilinear(input1[i][k], input2[k][j], v)
                        e.addAddend(1, v)

                    # Put output variable as the last addend last
                    e.addAddend(-1, outputVariables[i][j])
                    e.setScalar(0.0)
                    self.query.addEquation(e)
        elif len(shape2) == 3:
            assert(shape1[0] == shape2[0])
            for l in range(shape1[0]):
                for i in range(shape1[1]):
                    for j in range(shape2[2]):
                        e = MarabouUtils.Equation()
                        for k in range(shape1[2]):
                            v = self.query.getNewVariable()
                            self.query.addBilinear(input1[l][i][k], input2[l][k][j], v)
                            e.addAddend(1, v)

                        # Put output variable as the last addend last
                        e.addAddend(-1, outputVariables[l][i][j])
                        e.setScalar(0.0)
                        self.query.addEquation(e)
        else:
            raise RuntimeError(f"Unsupported shape in matMul tensor: {shape2}")

    def concatEquations(self, node):
        """Function to generate equations corresponding to concat

        Args:
            node (node): ONNX node representing the Concat operation

        :meta private:
        """
        nodeName = node.output[0]

        # Get attributes
        axis = None
        for attr in node.attribute:
            if attr.name == "axis":
                axis = get_attribute_value(attr)

        # Set maps of shape and var
        inputVars = list([self.varMap[input] for input in node.input])
        outputVars = np.concatenate(inputVars, axis)
        self.shapeMap[nodeName] = outputVars.shape
        self.varMap[nodeName] = outputVars

    def splitEquations(self, node, nodeName, makeEquations):
        """Function to generate equations corresponding to split

        Args:
            node (node): ONNX node representing the Split operation
            nodeName (str): Name of target node
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """
        # Get attributes
        axis = None
        split = None
        for attr in node.attribute:
            if attr.name == "axis":
                axis = get_attribute_value(attr)
            if attr.name == "split":
                split = get_attribute_value(attr)

        inputName = node.input[0]
        inputVars = Split_18.eval(self.varMap[inputName], split=split, axis=axis)

        assert len(inputVars) == len(node.output)

        # Set a shape of target output
        for i in range(len(node.output)):
            if node.output[i] == nodeName:
                self.shapeMap[node.output[i]] = inputVars[i].shape
                break

        if not makeEquations:
            return

        # Get variables and add quations
        for i in range(len(node.output)):
            if node.output[i] == nodeName:
                reshapedInputVars = inputVars[i].reshape(-1)
                outputVars = self.makeNewVariables(node.output[i]).reshape(-1)
                for j in range(len(reshapedInputVars)):
                    # Add equation
                    e = MarabouUtils.Equation()
                    e.addAddend(1, reshapedInputVars[j])
                    e.addAddend(-1, outputVars[j])
                    e.setScalar(0)
                    self.query.addEquation(e)
                break

    def resizeEquations(self, node, makeEquations):
        """Function to generate equations corresponding to resize

        Args:
            node (node): ONNX node representing the Resize operation
            makeEquations (bool): True if we need to create new variables and write Marabou equations

        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]

        # Check number of dimension of input
        inputVars = self.varMap[inputName]
        inputShape = inputVars.shape
        if inputVars.ndim != 4:
            raise NotImplementedError("Marabou only supports resize operator for very specific upsample case used in YOLO now.")

        # Get and check attributes
        coordinate_transformation_mode = None
        cubic_coeff_a = None
        mode = None
        nearest_mode = None

        for attr in node.attribute:
            value = get_attribute_value(attr)
            if attr.name == "coordinate_transformation_mode" and value.decode() == "asymmetric":
                coordinate_transformation_mode = value
            elif attr.name == "cubic_coeff_a" and value == -0.75:
                cubic_coeff_a = value
            elif attr.name == "mode" and value.decode() == "nearest":
                mode = value
            elif attr.name == "nearest_mode" and value.decode() == "floor":
                nearest_mode = value
            else:
                # Marabou supports Resize only very specific case below.
                #  coordinate_transformation_mode: asymmetric
                #  cubic_coeff_a: -0.75
                #  mode: nearest
                #  nearest_mode: floor
                # There are many cases other than the above case according to https://github.com/onnx/onnx/blob/main/docs/Operators.md#resize
                # Please note that we should carefully expand this operation beyond this case.
                raise NotImplementedError("Marabou only supports resize operator for very specific upsample case used in YOLO now.")

        # Get scales
        scales = None
        if len(node.input) == 3  and np.all(self.constantMap[node.input[2]] == [1., 1., 2., 2.]):
            scales = [1, 1, 2, 2]
        else:
             raise NotImplementedError("Marabou only supports resize operator for very specific upsample case used in YOLO now.")

        # Set output shape
        outputShape = (inputShape[0], inputShape[1], inputShape[2] * scales[2], inputShape[3] * scales[3])
        self.shapeMap[nodeName] = outputShape

        if not makeEquations:
            return

        # Get variables
        outputVars = self.makeNewVariables(nodeName)

        assert scales[2] * scales[3] * inputVars.size == outputVars.size

        for i in range(outputShape[1]):
            for j in range(outputShape[2]):
                for k in range(outputShape[3]):
                    # Add equation
                    e = MarabouUtils.Equation()
                    e.addAddend(1, inputVars[0][i][int(j / 2)][int(k / 2)])
                    e.addAddend(-1, outputVars[0][i][j][k])
                    e.setScalar(0)
                    self.query.addEquation(e)

    def mulEquations(self, node, makeEquations):
        nodeName = node.output[0]

        # Get the inputs
        inputName1, inputName2 = node.input
        shape1 = self.shapeMap[inputName1]
        # shape2 = self.shapeMap[inputName2] # comment out since this is never used.


        # Get the broadcasted shape
        outShape = shape1
        self.shapeMap[nodeName] = outShape
        if not makeEquations:
            return

        multiple = self.constantMap[inputName2]
        if inputName1 in self.constantMap:
            input1 = self.constantMap[inputName1]
            self.constantMap[nodeName] = input1 * multiple
        else:
            input1 = self.varMap[inputName1]
            outputVariables = self.makeNewVariables(nodeName)
            input1 = input1.reshape(-1)
            outputVariables = outputVariables.reshape(-1)

            for i in range(len(input1)):
                e = MarabouUtils.Equation()
                e.addAddend(multiple, input1[i])
                e.addAddend(-1, outputVariables[i])
                e.setScalar(0.0)
                self.query.addEquation(e)
        return

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
                self.query.addEquation(e)
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
        for equ in self.query.equList:
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
            self.varMap[nodeName] = copy(varInput).reshape(outShape)
        else:
            # Otherwise, assert no equations were changed, and we need to create new equations
            assert numEquationsChanged == 0
            outputVariables = self.makeNewVariables(nodeName).reshape(-1)
            for i in range(len(outputVariables)):
                e = MarabouUtils.Equation()
                e.addAddend(1, varInput[i])
                e.addAddend(-1, outputVariables[i])
                e.setScalar(-constInput[i])
                self.query.addEquation(e)

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
            self.query.addRelu(inputVars[i], outputVars[i])
        for f in outputVars:
            self.query.setLowerBound(f, 0.0)

    def leakyReluEquations(self, node, makeEquations):
        """Function to generate equations corresponding to pointwise LeakyRelu
        Args:
            node (node): ONNX node representing the LeakyRelu operation
            makeEquations (bool): True if we need to create new variables and add new LeakyRelus
        :meta private:
        """
        nodeName = node.output[0]
        inputName = node.input[0]
        self.shapeMap[nodeName] = self.shapeMap[inputName]
        if not makeEquations:
            return

        alpha = 0.01
        for attr in node.attribute:
            if attr.name == 'alpha':
                alpha = get_attribute_value(attr)

        # Get variables
        inputVars = self.varMap[inputName].reshape(-1)
        outputVars = self.makeNewVariables(nodeName).reshape(-1)
        assert len(inputVars) == len(outputVars)

        # Generate equations
        for i in range(len(inputVars)):
            self.query.addLeakyRelu(inputVars[i], outputVars[i], alpha)

    def subEquations(self, node, makeEquations):
        """Function to generate equations corresponding to subtraction

        Args:
            node (node): ONNX node representing the Sub operation
            makeEquations (bool): True if we need to create new variables and add new Relus

        :meta private:
        """
        nodeName = node.output[0]
        inputName1, inputName2 = node.input[0], node.input[1]
        assert inputName1 in self.shapeMap and inputName2 in self.shapeMap
        assert list(self.shapeMap[inputName1]) == list(self.shapeMap[inputName2])
        self.shapeMap[nodeName] = self.shapeMap[inputName1]

        if not makeEquations:
            return

        assert inputName1 in self.varMap and (inputName2 in self.constantMap or inputName2 in self.varMap)

        # the difference between the two variables
        if inputName1 in self.varMap and inputName2 in self.varMap:
            outputVariables = self.makeNewVariables(nodeName)
            input1 = self.varMap[inputName1].reshape(-1)
            input2 = self.varMap[inputName2].reshape(-1)
            outputVariables = outputVariables.reshape(-1)
            for i in range(len(input1)):
                e = MarabouUtils.Equation()
                e.addAddend(1, input1[i])
                e.addAddend(-1, input2[i])
                e.addAddend(-1, outputVariables[i])
                e.setScalar(0.0)
                self.query.addEquation(e)
            return

        # Get variables
        inputVars = self.varMap[inputName1].reshape(-1)
        outputVars = self.makeNewVariables(nodeName).reshape(-1)
        constants = self.constantMap[inputName2].reshape(-1)
        assert len(inputVars) == len(outputVars) == len(constants)

        # Generate equations
        for i in range(len(inputVars)):
            e = MarabouUtils.Equation()
            e.addAddend(1, inputVars[i])
            e.addAddend(-1, outputVars[i])
            e.setScalar(constants[i])
            self.query.addEquation(e)

    def sigmoidEquations(self, node, makeEquations):
        """Function to generate equations corresponding to Sigmoid

        Args:
            node (node): ONNX node representing the Sigmoid operation
            makeEquations (bool): True if we need to create new variables and add new Sigmoids

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
            self.query.addSigmoid(inputVars[i], outputVars[i])
        for f in outputVars:
            self.query.setLowerBound(f, 0.0)
            self.query.setUpperBound(f, 1.0)

    def tanhEquations(self, node, makeEquations):
        """Function to generate equations corresponding to Tanh

        Args:
            node (node): ONNX node representing the Tanh operation
            makeEquations (bool): True if we need to create new variables and add new Tanhs

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
        firstAffine = np.array([self.query.getNewVariable() for i in range(outputVars.size)])
        sigmoidOutput = np.array([self.query.getNewVariable() for i in range(outputVars.size)])

        # Generate equations
        for i in range(len(inputVars)):  # tanh(x) = 2 * \sigmoid(2x) - 1
            self.query.addEquality([inputVars[i], firstAffine[i]], [2.0, -1.0], 0.0, isProperty=False)
            self.query.addSigmoid(firstAffine[i], sigmoidOutput[i])
            self.query.addEquality([sigmoidOutput[i], outputVars[i]], [2.0, -1.0], 1.0, isProperty=False)
        for f in outputVars:
            self.query.setLowerBound(f, -1.0)
            self.query.setUpperBound(f, 1.0)

    def cleanShapes(self):
        """Remove unused shapes

        After constructing equations, remove shapes from self.shapeMap that are part of the graph but not
        relevant for this input query. This is only cosmetic and does not impact Marabou.

        :meta private:
        """
        for nodeName in [name for name in self.shapeMap]:
            if nodeName not in self.varMap and nodeName not in self.constantMap:
                self.shapeMap.pop(nodeName)

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
