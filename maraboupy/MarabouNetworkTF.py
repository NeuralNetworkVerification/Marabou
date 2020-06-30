''''''
'''
/* *******************                                                        */
/*! \file MarabouNetworkTF.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Kyle Julian, Christopher Lazarus, Shantanu Thakoor, Chelsea Sidrane
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief This file reads tensorflow networks to create Marabou equations and constraints
 **/
'''

import numpy as np
import os
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"

import tensorflow as tf
tf.compat.v1.disable_v2_behavior()
from tensorflow.python.framework import tensor_util
from tensorflow.python.framework import graph_util
from maraboupy import MarabouUtils
from maraboupy import MarabouNetwork

# Try to load helper function available for tensorflow versions >=1.14.0
try:
    from tensorflow.python.framework.convert_to_constants import convert_variables_to_constants_v2
except:
    pass
# Need eager exectution for savedModel_v2 format
try:
    tf.compat.v1.enable_eager_execution()
except:
    pass

class MarabouNetworkTF(MarabouNetwork.MarabouNetwork):
    """Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf or SavedModel

    Args:
        filename (str): Path to tensorflow network
        inputNames (list of str, optional): List of operation names corresponding to inputs
        outputName (str, optional): Name of operation corresponding to output
        modelType (str, optional): Type of model to read. The default is "frozen" for a frozen graph.
                            Can also use "savedModel_v1" or "savedModel_v2" for the SavedModel format
                            created from either tensorflow versions 1.X or 2.X respectively.
        savedModelTags (list of str, optional): If loading a SavedModel, the user must specify tags used, default is []
    """
    def __init__(self, filename, inputNames=None, outputName=None, modelType="frozen", savedModelTags=[]):
        super().__init__()
        self.readTF(filename, inputNames, outputName, modelType, savedModelTags)

    def clear(self):
        """Reset values to represent empty network
        """
        super().clear()
        self.madeGraphEquations = []
        self.varMap = dict()
        self.constantMap = dict()
        self.inputOps = None
        self.outputOp = None
        self.outputShape = None
        self.sess = None

    def readTF(self, filename, inputNames, outputName, modelType, savedModelTags):
        """Read a tensorflow file to create a MarabouNetworkTF object

        Args:
            filename (str): Path to tensorflow network
            inputNames (list of str, optional): List of operation names corresponding to inputs
            outputName (str, optional): Name of operation corresponding to output
            modelType (str, optional): Type of model to read. The default is "frozen" for a frozen graph.
                                Can also use "savedModel_v1" or "savedModel_v2" for the SavedModel format
                                created from either tensorflow versions 1.X or 2.X respectively.
            savedModelTags (list of str, optional): If loading a SavedModel, the user must specify tags used, default is []
        """
        # Read tensorflow model
        if modelType == "frozen":
            # Read frozen graph protobuf file
            with tf.io.gfile.GFile(filename, "rb") as f:
                graph_def = tf.compat.v1.GraphDef()
                graph_def.ParseFromString(f.read())
            with tf.Graph().as_default() as graph:
                tf.import_graph_def(graph_def, name = "")
            self.sess = tf.compat.v1.Session(graph=graph)
            
        elif modelType == "savedModel_v1":
            # Use default tag for savedModel format for tensorflow 1.X
            if not savedModelTags:
                savedModelTags = ["serve"]
                
            # Read SavedModel format created by tensorflow version 1.X
            sess = tf.compat.v1.Session()
            tf.compat.v1.saved_model.loader.load(sess, savedModelTags, filename)

            # Simplify graph using outputName, which must be specified for SavedModel
            simp_graph_def = graph_util.convert_variables_to_constants(sess,sess.graph.as_graph_def(),[outputName])
            with tf.Graph().as_default() as graph:
                tf.import_graph_def(simp_graph_def, name = "")
            self.sess = tf.compat.v1.Session(graph=graph)
            
        elif modelType == "savedModel_v2":
            # Use default tag for savedModel format for tensorflow 2.X
            if not savedModelTags:
                savedModelTags = ["serving_default"]
                
            # Read SavedModel format created by tensorflow version 2.X
            sess = tf.compat.v1.Session()
            loaded = tf.saved_model.load(filename)
            model = loaded.signatures[savedModelTags[0]]
                
            # Create a concrete function from model
            full_model = tf.function(lambda x: model(x))
            full_model = full_model.get_concrete_function(tf.TensorSpec(model.inputs[0].shape, model.inputs[0].dtype))

            # Get frozen ConcreteFunction
            frozen_func = convert_variables_to_constants_v2(full_model)
            self.sess = tf.compat.v1.Session(graph=frozen_func.graph)
       
        else:
            err_msg = "Unknown input to modelType.\n"
            err_msg += "Please use either 'frozen', 'savedModel_v1', or 'savedModel_v2'"
            raise RuntimeError(err_msg)

        # Valid names to use for input or output operations
        valid_names = [op.node_def.name for op in self.sess.graph.get_operations() if self.isVariable(op)]

        # Get output operation if outputName is given
        if outputName:
            if outputName not in valid_names:
                err_msg = "The given output name %s is not an operation in the network.\n" % outputName
                err_msg += "Valid names are\n" + ", ".join(valid_names)
                raise RuntimeError(err_msg)
            outputOp = self.sess.graph.get_operation_by_name(outputName)
        # By default, assume that the last operation is the output if output not specified
        else: 
            outputOp = self.sess.graph.get_operations()[-1]
            
        # Get input operations if inputNames is given
        if inputNames:
            inputOps = []
            for i in inputNames:
                if i not in valid_names:
                    err_msg = "The input name %s is not an operation in the network.\n" % i
                    err_msg += "Valid names are\n" + ", ".join(valid_names)
                    raise RuntimeError(err_msg)
                inputOps.append(self.sess.graph.get_operation_by_name(i))
        # By default, use Placeholders that contribute to output operation as input operations       
        else: 
            inputOps = self.findPlaceholders(outputOp, [])
            
        # Input operation should not equal output operation
        if outputOp in inputOps:
            raise RuntimeError("%s cannot be used as both input and output" % (outputOp.node_def.name))
            
        self.setInputOps(inputOps)
        self.setOutputOp(outputOp)

        # Generate equations corresponding to network
        self.foundnInputFlags = 0
        self.buildEquations(self.outputOp)
        
        # If we did not use all of the input variables, throw an error and tell the user which inputs were used
        if self.foundnInputFlags < len(inputOps):
            good_inputs = []
            for iop in inputOps:
                if iop in self.madeGraphEquations:
                    good_inputs.append(iop.node_def.name)
            err_msg = "All given inputs are part of the graph, but not all inputs contributed to the output %s\n"%outputName
            err_msg += "Please use only these inputs for the inputName list:\n"
            err_msg += "['" + "', '".join(good_inputs) + "']"
            raise RuntimeError(err_msg)
            
        # Set output variables
        self.outputVars = self.varMap[self.outputOp].reshape(self.outputShape)
        
        # This function changes all of the variables assignments so that the output variables
        # come immediately after input variables (Marabou convention).
        # This feature is optional, and can be disabled by commenting this line out
        self.reassignOutputVariables()

    def findPlaceholders(self, op, returnOps):
        """Function that recursively finds the placeholder operations that contribute to a given operation

        Args:
            op: (tf.op) representing operation in network
            returnOps (list of tf.op) Placehoder operartions already found

        Returns:
            returnOps (list of tf.op) Placehoder operartions found so far

        :meta private:
        """
        if op in returnOps:
            return returnOps
        if op.node_def.op == 'Const' or op in self.constantMap:
            return returnOps
        if op.node_def.op == 'Placeholder'or op in self.varMap:
            return returnOps + [op]
        for i in op.inputs:
            returnOps = self.findPlaceholders(i.op, returnOps)
        return returnOps
    
    def setInputOps(self, ops):
        """Function to set input operations

        Args:
            ops: (list of tf.op) list representing input operations

        :meta private:
        """
        self.inputVars = []
        self.inputShapes = []
        self.dummyFeed = dict()
        for op in ops:
            shape = tuple(op.outputs[0].shape.as_list())
            self.inputVars.append(self.makeNewVariables(op))
          
            # Dummy inputs are needed to compute constants for later versions of tensorflow
            self.dummyFeed[op.outputs[0]] = np.zeros(self.inputVars[-1].shape)
        self.inputOps = ops

    def setOutputOp(self, op):
        """Function to set output operation

        Args:
            op: (tf.op) representing output operation

        :meta private:
        """
        shape = op.outputs[0].shape.as_list()
        if shape[0] == None:
            shape[0] = 1
        self.outputShape = shape
        self.outputOp = op

    def makeNewVariables(self, x):
        """Function to find variables corresponding to operation

        Args:
            x: (tf.op) the operation to find variables for
        Returns:
            v: (np array) of variable numbers, in same shape as x

        :meta private:
        """
        # Find number and shape of new variables representing output of given operation
        shape = [a if a is not None else 1 for a in x.outputs[0].get_shape().as_list()]
        size = np.prod(shape)

        # Assign variables
        v = np.array([self.getNewVariable() for _ in range(size)]).reshape(shape)
        self.varMap[x] = v
        assert all([np.equal(np.mod(i, 1), 0) for i in v.reshape(-1)]) # check if integers
        return v

    def getValues(self, op):
        """Function to find underlying constants or variables representing operation

        Args:
            op: (tf.op) operation to find values for

        Returns:
            values: (np array) scalars or variable numbers representing the operation
            isVar: (bool) true if the operation is represented by variables

        :meta private:
        """
        if op in self.varMap:
            return self.varMap[op], True
        return self.constantMap[op], False

    def isVariable(self, op):
        """Function returning whether operation represents variable or constant

        Args:
            op: (tf.op) representing operation in network

        Returns:
            isVariable: (bool) true if variable, false if constant

        :meta private:
        """
        if op.node_def.op == 'Const' or op in self.constantMap:
            return False
        if op.node_def.op == 'Placeholder'or op in self.varMap:
            return True
        return any([self.isVariable(i.op) for i in op.inputs])

    def identity(self, op):
        """Function handling identity operation, performed on variables

        Args:
            op: (tf.op) identity operation

        :meta private:
        """
        self.varMap[op] = self.varMap[op.inputs[0].op]
        
    def reshape(self, op):
        """Function handling reshape operation, performed on variables

        Args:
            op: (tf.op) reshape operation

        :meta private:
        """
        variables = self.varMap[op.inputs[0].op]
        shape = self.constantMap[op.inputs[1].op]
        self.varMap[op] = variables.reshape(shape)
        
    def transpose(self, op):
        """Function handling transpose operation, performed on variables

        Args:
            op: (tf.op) transpose operation

        :meta private:
        """
        variables = self.varMap[op.inputs[0].op]
        perm = self.constantMap[op.inputs[1].op]
        self.varMap[op] = np.transpose(variables, perm)
        
    def concat(self, op):
        """Function handling concat operation, performed on variables

        Args:
            op: (tf.op) concat operation

        :meta private:
        """
        input1 = self.varMap[op.inputs[0].op]
        input2 = self.varMap[op.inputs[1].op]
        axis = self.constantMap[op.inputs[2].op]
        self.varMap[op] = np.concatenate([input1, input2], axis=axis)
        
    def matMulEquations(self, op):
        """Function to generate equations corresponding to matrix multiplication

        Args:
            op: (tf.op) representing matrix multiplication operation

        :meta private:
        """
        # Get variables and constants of inputs
        assert len(op.inputs) == 2
        A, A_isVar = self.getValues(op.inputs[0].op)
        B, B_isVar = self.getValues(op.inputs[1].op)
        
        # For linear equations, both inputs cannot be variables
        assert not A_isVar or not B_isVar
            
        # Handle transpose attributes
        aTranspose = op.node_def.attr['transpose_a'].b
        bTranspose = op.node_def.attr['transpose_b'].b
        if aTranspose:
            A = np.transpose(A)
        if bTranspose:
            B = np.transpose(B)
            
        # Get output variables and scalar values (if matmul is the input to an addition operation)
        outputVars = self.makeNewVariables(op)
        scalars, sgnVar, sgnScalar = self.getScalars(op, outputVars)
        
        # Make sure shapes are valid
        assert (A.shape[0], B.shape[1]) == outputVars.shape
        assert A.shape[1] == B.shape[0]
        m, n = outputVars.shape
        p = A.shape[1]

        # Generate equations
        for i in range(m):
            for j in range(n):
                e = MarabouUtils.Equation()
                for k in range(p):
                    # Make sure addend is added with weight, the variable number
                    if A_isVar:
                        e.addAddend(B[k][j] * sgnVar, A[i][k])
                    else:
                        e.addAddend(A[i][k] * sgnVar, B[k][j])
                e.addAddend(-1, outputVars[i][j])
                e.setScalar(-sgnScalar * scalars[i][j])
                self.addEquation(e)
                
    def getScalars(self, op, outputVars):
        """Get scalar values
        
        This helper function determines if MatMul of Conv2D equations can be combined with a subsequent
        addition or subtraction operation. If so, this function returns a scalar array, as well as
        the sign used for the matmul variables and scalar constants. If combining MatMul/Conv2D with
        addition fails, this returns values so that MatMul or Conv2D equations are unaffected.

        Args:
            op: (tf.op) representing conv2D operation
            outputVars: (np array) output variables from MatMul/Conv2D

        Returns:
            scalars (np array) array of scalar values to be used with MatMul/Conv2D
            sgnVar (int) sign for input variables to MatMul/Conv2D
            sgnScalar (int) sign for scalars to MatMul/Conv2D

        :meta private:
        """
        # If the given operation is the output operation, don't add any scalars
        if op == self.outputOp:
            return np.zeros(outputVars.shape), 1, 1
            
        # Look trhough the graph to find operations where the output of given operation is an input
        outputOp = op.outputs[0]
        nextOps = []
        for nextOp in self.sess.graph.get_operations():
            if op.outputs[0] in nextOp.inputs:
                nextOps.append(nextOp)
                
        # If output of given operation is used in multiple operations, don't combine with addition
        if len(nextOps) != 1:
            return np.zeros(outputVars.shape), 1, 1
        
        # The subsequent operation must be an addition/subtraction operation
        nextOp = nextOps[0]
        if nextOp.node_def.op not in ["Add", "AddV2", "BiasAdd", "Sub"]:
            return np.zeros(outputVars.shape), 1, 1
        
        # The subsequent addition/subtraction must combine the given operation with constants
        input1 = nextOp.inputs[0]
        input2 = nextOp.inputs[1]
        if self.isVariable(input1.op) and self.isVariable(input2.op):
            return np.zeros(outputVars.shape), 1, 1
        
        # Signs corresponding to addition/subtraction
        sgn1 = 1
        sgn2 = 1
        if nextOp.node_def.op in ["Sub"]:
            sgn2 = -1
            
        # Compute scalar values and signs depending on which input to nextOp is the output of given operation
        if input1 == op.outputs[0]:
            sgnVar = sgn1
            sgnScalar = sgn2
            # Extract constants with sess.run since they haven't been added to constantMap yet
            self.constantMap[input2.op] = self.sess.run(input2, feed_dict = self.dummyFeed)
            
            # Special case for BiasAdd with NCHW format. We need to add the bias along channels dimension
            if nextOp.node_def.op == 'BiasAdd':
                if 'data_format' in nextOp.node_def.attr:
                    data_format = nextOp.node_def.attr['data_format'].s.decode().upper()
                    if data_format == 'NCHW':
                        in2 = self.constantMap[input2.op]
                        self.constantMap[input2.op] = in2.reshape((1, len(in2), 1, 1))
            scalars = np.broadcast_to(self.constantMap[input2.op], outputVars.shape)
        else:
            sgnVar = sgn2
            sgnScalar = sgn1
            # Extract constants with sess.run since they haven't been added to constantMap yet
            self.constantMap[input1.op] = self.sess.run(input1, feed_dict = self.dummyFeed)
            scalars = np.broadcast_to(self.constantMap[input1.op], outputVars.shape)
            
        # Save outputVars to correspond to nextOp, so that new equations will not be generated for that operation
        self.varMap[nextOp] = outputVars
        return scalars, sgnVar, sgnScalar
            
    def addEquations(self, op):
        """Function to generate equations corresponding to all types of add/subtraction operations

        Args:
            op: (tf.op) representing addition or subtraction operations

        :meta private:
        """
        # We may have included the add equation with a prior matmul equation
        if op in self.varMap:
            return
        
        # Get inputs and outputs
        assert len(op.inputs) == 2
        input1, input1_isVar = self.getValues(op.inputs[0].op)
        input2, input2_isVar = self.getValues(op.inputs[1].op)
        outputVars = self.makeNewVariables(op).flatten()
        
        # Special case for BiasAdd with NCHW format. We need to add the bias along the channels dimension
        if op.node_def.op == 'BiasAdd':
            data_format = 'NHWC'
            if 'data_format' in op.node_def.attr:
                data_format = op.node_def.attr['data_format'].s.decode().upper()
            if data_format == 'NCHW':
                input2 = input2.reshape((1, len(input2), 1, 1))
        
        # Broadcast and flatten. Assert that lengths are all the same
        input1, input2 = np.broadcast_arrays(input1, input2)
        input1 = input1.flatten()
        input2 = input2.flatten()
        assert len(input1) == len(input2)
        assert len(outputVars) == len(input1)
        
        # Signs for addition/subtraction
        sgn1 = 1
        sgn2 = 1
        if op.node_def.op in ["Sub"]:
            sgn2 = -1
             
        # Create new equations depending on if the inputs are variables or constants
        # At least one input must be a variable, otherwise this operation is a constant,
        # which gets caught in makeGraphEquations.
        assert input1_isVar or input2_isVar
        
        # Always negate the scalar term because it changes sides in equation, from
        # w1*x1+...wk*xk + b = x_out
        # to
        # w1*x1+...wk+xk - x_out = -b
        if input1_isVar and input2_isVar:
            for i in range(len(outputVars)):
                e = MarabouUtils.Equation()
                e.addAddend(sgn1, input1[i])
                e.addAddend(sgn2, input2[i])
                e.addAddend(-1, outputVars[i])
                e.setScalar(0.0)
                self.addEquation(e)
        elif input1_isVar:
            for i in range(len(outputVars)):
                e = MarabouUtils.Equation()
                e.addAddend(sgn1, input1[i])
                e.addAddend(-1, outputVars[i])
                e.setScalar(-sgn2 * input2[i])
                self.addEquation(e)
        else:
            for i in range(len(outputVars)):
                e = MarabouUtils.Equation()
                e.addAddend(sgn2, input2[i])
                e.addAddend(-1, outputVars[i])
                e.setScalar(-sgn1 * input1[i])
                self.addEquation(e)

    def mulEquations(self, op):
        """Function to generate equations corresponding to multiply and divide operations

        Args:
            op: (tf.op) representing an element-wise multiply or divide operation

        :meta private:
        """
        # Get inputs and outputs
        assert len(op.inputs) == 2
        input1, input1_isVar = self.getValues(op.inputs[0].op)
        input2, input2_isVar = self.getValues(op.inputs[1].op)
        
        # For linear equations, both inputs cannot be variables
        assert not input1_isVar or not input2_isVar
        
        # If multiplying by 1, no need for new equations
        if input1_isVar and np.all(input2 == 1.0):
            self.varMap[op] = input1
            return
        if input2_isVar and np.all(input1 == 1.0):
            self.varMap[op] = input2
            return

        # Broadcast and flatten. Assert that lengths are all the same
        input1, input2 = np.broadcast_arrays(input1, input2)
        input1 = input1.flatten()
        input2 = input2.flatten()
        outputVars = self.makeNewVariables(op).flatten()
        assert len(input1) == len(input2)
        assert len(outputVars) == len(input1)
        
        # Handle divide by negating power
        power = 1.0
        if op.node_def.op in ["RealDiv"]:
            power = -1.0
            
        # Equations
        if input1_isVar:
            for i in range(len(outputVars)):
                e = MarabouUtils.Equation()
                e.addAddend(input2[i]**power, input1[i])
                e.addAddend(-1, outputVars[i])
                e.setScalar(0.0)
                self.addEquation(e)
        else:
            if power == -1.0:
                raise RuntimeError("Dividing a constant by a variable is not allowed")
            for i in range(len(outputVars)):
                e = MarabouUtils.Equation()
                e.addAddend(input1[i], input2[i])
                e.addAddend(-1, outputVars[i])
                e.setScalar(0.0)
                self.addEquation(e)
        
    def conv2DEquations(self, op):
        """Function to generate equations corresponding to 2D convolution operation

        Args:
            op: (tf.op) representing conv2D operation

        :meta private:
        """

        # Get input variables and constants
        assert len(op.inputs) == 2
        inputVars = self.varMap[op.inputs[0].op]
        filters = self.constantMap[op.inputs[1].op]
        
        # Make new variables for output
        outputVars = self.makeNewVariables(op)
        
        # Extract attributes
        padding = op.node_def.attr['padding'].s.decode().upper()
        strides = list(op.node_def.attr['strides'].list.i)
        data_format = op.node_def.attr['data_format'].s.decode().upper()
        
        # Handle different data formats
        if data_format == 'NHWC':
            out_num, out_height, out_width, out_channels = outputVars.shape
            in_num, in_height,  in_width,  in_channels  = inputVars.shape
            strides_height = strides[1]
            strides_width = strides[2]
        elif data_format == 'NCHW':
            out_num, out_channels, out_height, out_width = outputVars.shape
            in_num, in_channels, in_height,  in_width  = inputVars.shape
            strides_height = strides[2]
            strides_width = strides[3]
        else:
            raise NotImplementedError("Network uses %s data format. Only 'NHWC' and 'NCHW' are currently supported" % data_format)
            
        # Assert that dimensions match up
        filter_height, filter_width, filter_channels, num_filters = filters.shape
        assert out_num == in_num
        assert filter_channels == in_channels
        assert out_channels == num_filters
        
        # Use padding to determine top and left offsets
        if padding=='SAME':
            pad_top  = max((out_height - 1) * strides_height + filter_height - in_height, 0) // 2
            pad_left = max((out_width - 1) * strides_width + filter_width - in_width, 0) // 2
        elif padding=='VALID':
            pad_top = 0
            pad_left = 0
        else:
            raise NotImplementedError("Network uses %s for conv padding. Only 'SAME' and 'VALID' padding are supported" % padding)
            
        # Try to get scalar values in case this operation is followed by BiasAddition
        scalars, sgnVar, sgnScalar = self.getScalars(op, outputVars)

        # Generate equations
        # There is one equation for every output variable
        for n in range(out_num):
            for i in range(out_height):
                for j in range(out_width):
                    for k in range(out_channels): # Out_channel also corresponds to filter number
                        e = MarabouUtils.Equation()
                        
                        # The equation convolves the filter with the specified input region
                        # Iterate over the filter
                        for di in range(filter_height):
                            for dj in range(filter_width):
                                for dk in range(filter_channels):
                                    
                                    # Get 2D location of filter with respect to input variables
                                    h_ind = strides_height * i + di - pad_top
                                    w_ind = strides_width * j + dj - pad_left
                                    
                                    # Build equation when h_ind and w_ind are valid
                                    if h_ind < in_height and h_ind >= 0 and w_ind < in_width and w_ind >= 0:
                                        if data_format == 'NHWC':
                                            var = inputVars[n][h_ind][w_ind][dk]
                                            c = filters[di][dj][dk][k] * sgnVar
                                            e.addAddend(c, var)
                                        else:
                                            var = inputVars[n][dk][h_ind][w_ind]
                                            c = filters[di][dj][dk][k] * sgnVar
                                            e.addAddend(c, var)
                                            
                        # Add output variable
                        if data_format == 'NHWC':
                            e.addAddend(-1, outputVars[n][i][j][k])
                            e.setScalar(-sgnScalar * scalars[n][i][j][k])
                            self.addEquation(e)
                        else:
                            e.addAddend(-1, outputVars[n][k][i][j])
                            e.setScalar(-sgnScalar * scalars[n][k][i][j])
                            self.addEquation(e)

    def reluEquations(self, op):
        """Function to generate equations corresponding to pointwise Relu

        Args:
            op: (tf.op) representing Relu operation

        :meta private:
        """
        # Get input and output variables
        assert len(op.inputs) == 1
        inputVars = self.varMap[op.inputs[0].op].flatten()
        outputVars = self.makeNewVariables(op).flatten()
        assert len(inputVars) == len(outputVars)

        # Generate Relu constratins
        for inVar, outVar in zip(inputVars, outputVars):
            self.addRelu(inVar, outVar)
            self.setLowerBound(outVar, 0.0)

    def maxpoolEquations(self, op):
        """Function to generate maxpooling equations

        Args:
            op: (tf.op) representing maxpool operation

        :meta private:
        """
        # Get input and output variables
        assert len(op.inputs) == 1
        inputVars = self.varMap[op.inputs[0].op]
        outputVars = self.makeNewVariables(op)
        
        # Get attributes
        padding = op.node_def.attr['padding'].s.decode().upper()
        strides = list(op.node_def.attr['strides'].list.i)
        ksize = list(op.node_def.attr['ksize'].list.i)
        data_format = op.node_def.attr['data_format'].s.decode().upper()
        
        # Handle the different data formats
        if data_format == 'NHWC':
            out_num, out_height, out_width, out_channels = outputVars.shape
            in_num, in_height,  in_width,  in_channels  = inputVars.shape
            strides_height = strides[1]
            strides_width = strides[2]
            ksize_height = ksize[1]
            ksize_width = ksize[2]
        elif data_format == 'NCHW':
            out_num, out_channels, out_height, out_width = outputVars.shape
            in_num, in_channels, in_height,  in_width  = inputVars.shape
            strides_height = strides[2]
            strides_width = strides[3]
            ksize_height = ksize[2]
            ksize_width = ksize[3]
        else:
            raise NotImplementedError("Network uses %s data format. Only 'NHWC' and 'NCHW' are currently supported" % data_format)
            
        # Number of data points and channels should remain the same
        assert out_num == in_num
        assert out_channels == in_channels

        # Determine top and left padding for the padding type
        if padding == "VALID":
            pad_top = 0
            pad_left = 0
        elif padding == "SAME":
            pad_top = max((out_height - 1) * strides_height + ksize_height - in_height, 0) // 2
            pad_left = max((out_width - 1) * strides_width + ksize_width - in_width, 0) // 2
        else:
            raise NotImplementedError("Network uses %s padding for maxpool. Only 'VALID' and 'SAME' padding are currently supported" % padding)
            
        # Create max constraints, one for every output variable
        for n in range(out_num):
            for i in range(out_height):
                for j in range(out_width):
                    for k in range(out_channels):
                        maxVars = set()
                        
                        # Iterate over the input region of the max filter
                        for di in range(ksize_height):
                            for dj in range(ksize_width):

                                # Get 2D location of filter with respect to input variables
                                h_ind = strides_height * i + di - pad_top
                                w_ind = strides_width * j + dj - pad_left
                                
                                # Add input variable to set if it is valid
                                if h_ind < in_height and w_ind < in_width and h_ind>=0 and w_ind>=0:
                                    if data_format == "NHWC":
                                        maxVars.add(inputVars[n][h_ind][w_ind][k])
                                    else:
                                        maxVars.add(inputVars[n][k][h_ind][w_ind])
                                        
                        # Set output variable and add max constraint
                        if data_format == "NHWC":
                            self.addMaxConstraint(maxVars, outputVars[n][i][j][k])
                        else:
                            self.addMaxConstraint(maxVars, outputVars[n][k][i][j])
                    
    def reassignVariable(self, var, numInVars, outVars, newOutVars):
        """ Reassign variable number so output variables follow input variables

        Args:
            var: (int) Original variable number
            numInVars: (int) Number of input variables
            outVars: (array of int) Original output variables
            newOutVars: (array of int) New output variables

        Returns:
            (int) New variable assignment

        :meta private:
        """
        # Do not change input variables
        if var < numInVars:
            return var
        
        # Map output variables to new values
        if var in outVars:
            ind = np.where(var == outVars)[0][0]
            return newOutVars[ind]
        
        # Adjust other variables as needed to make room for output variables
        return var + newOutVars.size - np.sum([outVar < var for outVar in outVars])
    
    def reassignOutputVariables(self):
        """Reassign all variables so that output variables follow input variables

        Other input parsers assign output variables after input variables and before any intermediate variables.
        This function reassigns the numbers for the output variables and shifts all other variables up to make space.

        :meta private:
        """
        outVars = self.outputVars.flatten()
        numInVars = np.sum([inVar.size for inVar in self.inputVars])
        numOutVars = outVars.size
        newOutVars = np.array(range(numInVars,numInVars+numOutVars))
        
        # Build dictionary mapping old variable assignments to their new assignment
        reassignMap = dict()
        for var in range(self.numVars):
            reassignMap[var] = self.reassignVariable(var, numInVars, outVars, newOutVars)
        
        # Adjust equation variables
        for eq in self.equList:
            for i, (c,var) in enumerate(eq.addendList):
                eq.addendList[i] = (c, reassignMap[var])
                
        # Adjust relu list
        for i, variables in enumerate(self.reluList):
            self.reluList[i] = tuple([reassignMap[var] for var in variables])
        
        # Adjust maxpool list
        for i, (elements, outVar) in enumerate(self.maxList):
            newOutVar = reassignMap[outVar]
            newElements = set()
            for var in elements:
                newElements.add(reassignMap[var])
            self.maxList[i] = (newElements, newOutVar)
            
        # Adjust upper/lower bounds
        newLowerBounds = dict()
        newUpperBounds = dict()
        for var in self.lowerBounds:
            newLowerBounds[reassignMap[var]] = self.lowerBounds[var]
        for var in self.upperBounds:
            newUpperBounds[reassignMap[var]] = self.upperBounds[var]
        self.lowerBounds = newLowerBounds
        self.upperBounds = newUpperBounds
        
        # Adjust constraint variables list
        newVarsParticipatingInConstraints = set()
        for var in self.varsParticipatingInConstraints:
            newVarsParticipatingInConstraints.add(reassignMap[var])
        self.varsParticipatingInConstraints = newVarsParticipatingInConstraints
            
        # Assign output variables to the new array
        self.outputVars = newOutVars.reshape(self.outputShape)
        self.varMap[self.outputOp] = self.outputVars 
        
    def makeEquations(self, op):
        """Function to generate equations corresponding to given operation

        Args:
            op: (tf.op) for which to generate equations

        :meta private:
        """
        # These operations do not create new equations, but manipulate the output from previous equations
        if op.node_def.op == "Identity":
            self.identity(op)
        elif op.node_def.op == "Reshape":
            self.reshape(op)
        elif op.node_def.op == "ConcatV2":
            self.concat(op)
        elif op.node_def.op == "Transpose":
            self.transpose(op)
        
        # These operations do require new equations
        elif op.node_def.op == 'MatMul':
            self.matMulEquations(op)
        elif op.node_def.op in ['BiasAdd', 'Add', 'AddV2', 'Sub']:
            self.addEquations(op)
        elif op.node_def.op in ['Mul', 'RealDiv']:
            self.mulEquations(op)
        elif op.node_def.op == 'Conv2D':
            self.conv2DEquations(op)
        elif op.node_def.op == 'Relu':
            self.reluEquations(op)
        elif op.node_def.op == 'MaxPool':
            self.maxpoolEquations(op)
            
        # If we've recursed to find a Placeholder operation, this operation needs to be added the inputName list
        elif op.node_def.op == 'Placeholder':
            raise RuntimeError("The output %s depends on placeholder %s.\nPlease add '%s' to the inputName list." % (self.outputOp.node_def.name, op.node_def.name, op.node_def.name))
        else:
            raise NotImplementedError("Operation %s not implemented" % (op.node_def.op))

    def buildEquations(self, op):
        """Function that searches the graph recursively and builds equations necessary to calculate op

        Args:
            op: (tf.op) representing operation until which we want to generate equations

        :meta private:
        """
        # No need to make more equations if we've already seen this operation
        if op in self.madeGraphEquations:
            return
        self.madeGraphEquations += [op]
        
        # Stop recursing once an input variable is found
        # No equations are needed for the input variables, since they 
        # are not the output of any equation
        if op in self.inputOps:
            self.foundnInputFlags += 1
            return
        
        # If operation is a constant, or the product of equations using
        # constants, don't recurse because equations are not needed
        if not self.isVariable(op):
            self.constantMap[op] = self.sess.run(op.outputs[0], feed_dict = self.dummyFeed)
            return
        
        # Recurse using operation inputs
        in_ops = [x.op for x in op.inputs]
        for x in in_ops:
            self.buildEquations(x)
            
        # Make equations after recursing to build equations of inputs first
        self.makeEquations(op)

    def evaluateWithoutMarabou(self, inputValues):
        """Function to evaluate network at a given point using Tensorflow

        Args:
            inputValues: (list of np arrays) representing inputs to network

        Returns:
            outputValues: (np array) representing output of network
        """
        if len(inputValues) != len(self.inputOps):
            raise RuntimeError("Bad input given. Please provide a list of %d np arrays"%len(self.inputOps))
            
        # Try to reshape inputs to shapes desired by tensorflow
        inReshaped = []
        for i, (op, inputVals) in enumerate(zip(self.inputOps, inputValues)):
            opShape = self.varMap[op].shape
            if np.prod(opShape) != np.prod(inputVals.shape):
                raise RuntimeError("Input %d should have shape "%i + str(opShape) + 
                                   ", or be reshapeable to this shape, but an array with shape " + str(inputVals.shape) + " was given")
            inReshaped.append(np.array(inputVals).reshape(opShape))

        # Evaluate network
        inputNames = [o.outputs[0] for o in self.inputOps]
        feed_dict = dict(zip(inputNames, inReshaped))
        out = self.sess.run(self.outputOp.outputs[0], feed_dict=feed_dict)
        
        # Make sure to return output in correct shape
        return out.reshape(self.varMap[self.outputOp].shape)
