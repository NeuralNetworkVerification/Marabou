import numpy as np
from tensorflow.python.framework import tensor_util
from tensorflow.python.framework import graph_util
import os
os.environ['TF_CPP_MIN_LOG_LEVEL']='2'
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"

import tensorflow as tf
from . import MarabouUtils
from . import MarabouNetwork

class MarabouNetworkTF(MarabouNetwork.MarabouNetwork):
    def __init__(self, filename, inputName=None, outputName=None, savedModel=False, savedModelTags=[]):
        """
        Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf or SavedModel

        Args:
            filename: (string) If savedModel is false, path to the frozen graph .pb file.
                               If savedModel is true, path to SavedModel folder, which
                               contains .pb file and variables subdirectory.
            inputName: (string) optional, name of operation corresponding to input.
            outputName: (string) optional, name of operation corresponding to output.
            savedModel: (bool) If false, load frozen graph. If true, load SavedModel object.
            savedModelTags: (list of strings) If loading a SavedModel, the user must specify tags used.
        """
        super().__init__()
        self.biasAddRelations = list()
        self.readFromPb(filename, inputName, outputName, savedModel, savedModelTags)
        self.processBiasAddRelations()
    
    def clear(self):
        """
        Reset values to represent empty network
        """
        super().clear()
        self.varMap = dict()
        self.shapeMap = dict()
        self.inputOp = None
        self.outputOp = None
        self.sess = None
        self.biasAddRelations = list()

    def readFromPb(self, filename, inputName, outputName, savedModel, savedModelTags):
        """
        Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf or SavedModel

        Args:
            filename: (string) If savedModel is false, path to the frozen graph .pb file.
                               If savedModel is true, path to SavedModel folder, which
                               contains .pb file and variables subdirectory.
            inputName: (string) optional, name of operation corresponding to input.
            outputName: (string) optional, name of operation corresponding to output.
            savedModel: (bool) If false, load frozen graph. If true, load SavedModel object.
            savedModelTags: (list of strings) If loading a SavedModel, the user must specify tags used.
        """
        
        if savedModel:
            ### Read SavedModel ###
            sess = tf.Session()
            tf.saved_model.loader.load(sess, savedModelTags, filename)
            
            ### Simplify graph using outputName, which must be specified for SavedModel ###
            simp_graph_def = graph_util.convert_variables_to_constants(sess,sess.graph.as_graph_def(),[outputName])  
            with tf.Graph().as_default() as graph:
                tf.import_graph_def(simp_graph_def, name="")
            self.sess = tf.Session(graph=graph)
            ### End reading SavedModel
            
        else:
            ### Read protobuf file and begin session ###
            with tf.gfile.GFile(filename, "rb") as f:
                graph_def = tf.GraphDef()
                graph_def.ParseFromString(f.read())
            with tf.Graph().as_default() as graph:
                tf.import_graph_def(graph_def, name="")
            self.sess = tf.Session(graph=graph)
            ### END reading protobuf ###

        ### Find operations corresponding to input and output ###
        if inputName:
            inputOp = self.sess.graph.get_operation_by_name(inputName)
        else: # If there is just one placeholder, use it as input
            ops = self.sess.graph.get_operations()  
            placeholders = [x for x in ops if x.node_def.op == 'Placeholder']
            assert len(placeholders)==1
            inputOp = placeholders[0]
        if outputName:
            outputOp = self.sess.graph.get_operation_by_name(outputName)
        else: # Assume that the last operation is the output
            outputOp = self.sess.graph.get_operations()[-1]
        self.setInputOp(inputOp)
        self.setOutputOp(outputOp)
        ### END finding input/output operations ###

        ### Generate equations corresponding to network ###
        self.foundInputFlag = False
        self.makeGraphEquations(self.outputOp)
        assert self.foundInputFlag
        ### END generating equations ###

    def setInputOp(self, op):
        """
        Function to set input operation
        Arguments:
            op: (tf.op) Representing input
        """
        try:
            shape = tuple(op.outputs[0].shape.as_list())
            self.shapeMap[op] = shape
        except:
            self.shapeMap[op] = [None]
        self.inputOp = op
        self.inputVars = self.opToVarArray(self.inputOp)

    def setOutputOp(self, op):
        """
        Function to set output operation
        Arguments:
            op: (tf.op) Representing output
        """
        try:
            shape = tuple(op.outputs[0].shape.as_list())
            self.shapeMap[op] = shape
        except:
            self.shapeMap[op] = [None]
        self.outputOp = op
        self.outputVars = self.opToVarArray(self.outputOp)    

    def opToVarArray(self, x):
        """
        Function to find variables corresponding to operation
        Arguments:
            x: (tf.op) the operation to find variables for
        Returns:
            v: (np array) of variable numbers, in same shape as x
        """
        if x in self.varMap:
            return self.varMap[x]

        ### Find number of new variables needed ###
        if x in self.shapeMap:
            shape = self.shapeMap[x]
            shape = [a if a is not None else 1 for a in shape]
        else:
            shape = [a if a is not None else 1 for a in x.outputs[0].get_shape().as_list()]
        size = 1
        for a in shape:
            size*=a
        ### END finding number of new variables ###

        v = np.array([self.getNewVariable() for _ in range(size)]).reshape(shape)
        self.varMap[x] = v
        return v
        
    def getValues(self, op):
        """
        Function to find underlying constants/variables representing operation
        Arguments:
            op: (tf.op) to get values of
        Returns:
            values: (np array) of scalars or variable numbers depending on op
        """
        input_ops = [i.op for i in op.inputs]

        ### Operations not requiring new variables ###
        if op.node_def.op == 'Identity':
            return self.getValues(input_ops[0])
        if op.node_def.op in ['Reshape', 'Pack']:
            prevValues = [tf.constant(self.getValues[i]) for i in input_ops]
            names = [x.op.name for x in prevValues]
            op.node_def.inputs = names
            return self.sess.run(op)
        if op.node_def.op == 'Const':
            tproto = op.node_def.attr['value'].tensor
            return tensor_util.MakeNdarray(tproto)
        ### END operations not requiring new variables ###

        if op.node_def.op in ['MatMul', 'BiasAdd', 'Add', 'Relu', 'MaxPool', 'Conv2D', 'Placeholder']:
            # need to create variables for these
            return self.opToVarArray(op)

        raise NotImplementedError

    def matMulEquations(self, op):
        """
        Function to generate equations corresponding to matrix multiplication
        Arguments:
            op: (tf.op) representing matrix multiplication operation
        """

        ### Get variables and constants of inputs ###
        input_ops = [i.op for i in op.inputs]
        prevValues = [self.getValues(i) for i in input_ops]
        curValues = self.getValues(op)
        aTranspose = op.node_def.attr['transpose_a'].b
        bTranspose = op.node_def.attr['transpose_b'].b
        A = prevValues[0]
        B = prevValues[1]
        if aTranspose:
            A = np.transpose(A)
        if bTranspose:
            B = np.transpose(B)
        assert (A.shape[0], B.shape[1]) == curValues.shape
        assert A.shape[1] == B.shape[0]
        m, n = curValues.shape
        p = A.shape[1]
        ### END getting inputs ###

        ### Generate actual equations ###
        for i in range(m):
            for j in range(n):
                e = []
                e = MarabouUtils.Equation()
                for k in range(p):
                    e.addAddend(B[k][j], A[i][k])
                e.addAddend(-1, curValues[i][j])
                e.setScalar(0.0)
                aux = self.getNewVariable()
                self.setLowerBound(aux, 0.0)
                self.setUpperBound(aux, 0.0)
                e.markAuxiliaryVariable(aux)
                self.addEquation(e)

    def biasAddEquations(self, op):
        """
        Function to generate equations corresponding to bias addition
        Arguments:
            op: (tf.op) representing bias add operation
        """

        ### Get variables and constants of inputs ###
        input_ops = [i.op for i in op.inputs]
        prevValues = [self.getValues(i) for i in input_ops]
        curValues = self.getValues(op)
        prevVars = prevValues[0].reshape(-1)
        prevConsts = prevValues[1].reshape(-1)
        # broadcasting
        prevConsts = np.tile(prevConsts, len(prevVars)//len(prevConsts))
        curVars = curValues.reshape(-1)
        assert len(prevVars)==len(curVars) and len(curVars)==len(prevConsts)
        ### END getting inputs ###

        ### Do not generate equations, as these can be eliminated ###
        for i in range(len(prevVars)):
            # prevVars = curVars - prevConst
            self.biasAddRelations += [(prevVars[i], curVars[i], -prevConsts[i])]

    def processBiasAddRelations(self):
        """
        Either add an equation representing a bias add,
        Or eliminate one of the two variables in every other relation
        """
        biasAddUpdates = dict()
        for (x, xprime, c) in self.biasAddRelations:
            # x = xprime + c
            # replace x only if it does not occur anywhere else in the system
            if self.lowerBoundExists(x) or self.upperBoundExists(x) or self.participatesInPLConstraint(x):
                e = MarabouUtils.Equation()
                e.addAddend(1, x)
                e.addAddend(-1, xprime)
                e.setScalar(-c)
                aux = self.getNewVariable()
                self.setLowerBound(aux, 0.0)
                self.setUpperBound(aux, 0.0)
                e.markAuxiliaryVariable(aux)
                self.addEquation(e)
            else:
                biasAddUpdates[x] = (xprime, c)
                self.setLowerBound(x, 0)
                self.setUpperBound(x, 0)

        for equ in self.equList:
            participating = equ.getParticipatingVariables()
            for x in participating:
                if x in biasAddUpdates: # if a variable to remove is part of this equation
                    xprime, c = biasAddUpdates[x]
                    equ.replaceVariable(x, xprime, c)

    def conv2DEquations(self, op):
        """
        Function to generate equations corresponding to 2D convolution operation
        Arguments:
            op: (tf.op) representing conv2D operation
        """

        ### Get variables and constants of inputs ###
        input_ops = [i.op for i in op.inputs]
        prevValues = [self.getValues(i) for i in input_ops]
        curValues = self.getValues(op)
        padding = op.node_def.attr['padding'].s.decode()
        strides = list(op.node_def.attr['strides'].list.i)
        prevValues, prevConsts = prevValues[0], prevValues[1]
        _, out_height, out_width, out_channels = curValues.shape
        _, in_height,  in_width,  in_channels  = prevValues.shape
        filter_height, filter_width, filter_channels, num_filters = prevConsts.shape
        assert filter_channels == in_channels
        assert out_channels == num_filters
        # Use padding to determine top and left offsets
        # See https://github.com/tensorflow/tensorflow/blob/master/tensorflow/core/kernels/quantized_conv_ops.cc#L51
        if padding=='SAME':
            pad_top  = ((out_height - 1) * strides[1] + filter_height - in_height) // 2
            pad_left = ((out_width - 1) * strides[2] + filter_width - in_width) // 2
        elif padding=='VALID':
            pad_top  = ((out_height - 1) * strides[1] + filter_height - in_height + 1) // 2
            pad_left = ((out_width - 1) * strides[2] + filter_width - in_width + 1) // 2
        else:
            raise NotImplementedError 
        ### END getting inputs ###
        
        ### Generate actual equations ###
        # There is one equation for every output variable
        for i in range(out_height):
            for j in range(out_width):
                for k in range(out_channels): # Out_channel corresponds to filter number
                    e = MarabouUtils.Equation()
                    # The equation convolves the filter with the specified input region
                    # Iterate over the filter
                    for di in range(filter_height):
                        for dj in range(filter_width):
                            for dk in range(filter_channels):
                                
                                h_ind = int(strides[1]*i+di - pad_top)
                                w_ind = int(strides[2]*j+dj - pad_left)
                                if h_ind < in_height and h_ind>=0 and w_ind < in_width and w_ind >=0:
                                    var = prevValues[0][h_ind][w_ind][dk]
                                    c = prevConsts[di][dj][dk][k]
                                    e.addAddend(c, var)
                                
                    # Add output variable
                    e.addAddend(-1, curValues[0][i][j][k])
                    e.setScalar(0.0)
                    aux = self.getNewVariable()
                    self.setLowerBound(aux, 0.0)
                    self.setUpperBound(aux, 0.0)
                    e.markAuxiliaryVariable(aux)
                    self.addEquation(e)

    def reluEquations(self, op):
        """     
        Function to generate equations corresponding to pointwise Relu
        Arguments:
            op: (tf.op) representing Relu operation
        """

        ### Get variables and constants of inputs ###
        input_ops = [i.op for i in op.inputs]
        prevValues = [self.getValues(i) for i in input_ops]
        curValues = self.getValues(op)
        prev = prevValues[0].reshape(-1)
        cur = curValues.reshape(-1)
        assert len(prev) == len(cur)
        ### END getting inputs ###
        
        ### Generate actual equations ###
        for i in range(len(prev)):
            self.addRelu(prev[i], cur[i])
        #     e = MarabouUtils.Equation()
        #     e.addAddend(1.0, prev[i])
        #     e.addAddend(-1.0, cur[i])
        #     e.setScalar(0.0)
        #     aux = self.getNewVariable()
        #     e.addAddend(1.0, aux)
        #     e.markAuxiliaryVariable(aux)
        #     self.setLowerBound(aux, 0.0)
        #     self.addEquation(e)
        for f in cur:
            self.setLowerBound(f, 0.0)

    def maxpoolEquations(self, op):
        """         
        Function to generate maxpooling equations
        Arguments:
            op: (tf.op) representing maxpool operation
        """
        ### Get variables and constants of inputs ###
        input_ops = [i.op for i in op.inputs]
        prevValues = [self.getValues(i) for i in input_ops]
        curValues = self.getValues(op)
        validPadding = op.node_def.attr['padding'].s == b'VALID'
        if not validPadding:
            raise NotImplementedError
        prevValues = prevValues[0]
        strides = list(op.node_def.attr['strides'].list.i)
        ksize = list(op.node_def.attr['ksize'].list.i)
        for i in range(curValues.shape[1]):
            for j in range(curValues.shape[2]):
                for k in range(curValues.shape[3]):
                    maxVars = set()
                    for di in range(strides[1]*i, strides[1]*i + ksize[1]):
                        for dj in range(strides[2]*j, strides[2]*j + ksize[2]):
                            if di < prevValues.shape[1] and dj < prevValues.shape[2]:
                                maxVars.insert([prevValues[0][di][dj][k]])
                    self.addMaxConstraint(maxVars, curValues[0][i][j][k])

    def makeNeuronEquations(self, op): 
        """
        Function to generate equations corresponding to given operation
        Arguments:
            op: (tf.op) for which to generate equations
        """
        if op.node_def.op in ['Identity', 'Reshape', 'Pack', 'Placeholder', 'Const']:
            return
        if op.node_def.op == 'MatMul':
            self.matMulEquations(op)
        elif op.node_def.op in ['BiasAdd', 'Add']:
            self.biasAddEquations(op)
        elif op.node_def.op == 'Conv2D':
            self.conv2DEquations(op)
        elif op.node_def.op == 'Relu':
            self.reluEquations(op)
        elif op.node_def.op == 'MaxPool':
            self.maxpoolEquations(op)
        else:
            raise NotImplementedError

    def makeGraphEquations(self, op):
        """
        Function to generate equations for network necessary to calculate op
        Arguments:
            op: (tf.op) representing operation until which we want to generate network equations
        """
        in_ops = [x.op for x in op.inputs]    
        for x in in_ops:
            if not x.name == self.inputOp.name:
                self.makeGraphEquations(x)
            else:
                self.foundInputFlag = True
        self.makeNeuronEquations(op)

    def evaluateWithoutMarabou(self, inputValues):
        """
        Function to evaluate network at a given point using Tensorflow
        Arguments:
            inputValues: (np array) representing input to network
        Returns:
            outputValues: (np array) representing output of network
        """
        inputShape = self.shapeMap[self.inputOp]
        inputShape = [i if i is not None else 1 for i in inputShape]        
        # Try to reshape given input to correct shape
        inputValues = inputValues.reshape(inputShape)
        
        inputName = self.inputOp.name
        outputName = self.outputOp.name
        out = self.sess.run(outputName + ":0", feed_dict={inputName + ":0":inputValues})
        return out[0]
