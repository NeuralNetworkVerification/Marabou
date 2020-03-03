

'''
/* *******************                                                        */
/*! \file MarabouNetworkNNeIPQ.py
 ** \verbatim
 ** Top contributors (to current version):
 ** Alex Usvyatsov
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief
 ** This class extends MarabouNetworkNNet class
 ** Adds an Input Query object as an additional attribute
 ** Adds features that allow updating the the MarabouNetworkNNet object from the IPQ
 **
 ** [[ Add lengthier description here ]]
 **/
'''




import MarabouNetworkNNetExtendedParent
import MarabouCore
import numpy as np

class MarabouNetworkNNetIPQ(MarabouNetworkNNetExtendedParent.MarabouNetworkNNetExtendedParent):
    """
    Class that implements a MarabouNetwork from an NNet file.
    """
    def __init__ (self, filename="", property_filename = "", perform_sbt=False, compute_ipq = False):
        """
        Constructs a MarabouNetworkNNetIPQ object from an .nnet file.
        Computes InputQuery, potentially in two ways
        ipq1 is computed from the MarabouNetworkNNet object itself
        ipq2 is computed directly from the nnet file

        Args:
            filename: path to the .nnet file.
            property_filename: path to the property file

        Attributes:
            ipq1             an Input Query object containing the Input Query corresponding to the network
            ipq2             an Input Query object created from the file (and maybe property file)

        Attributes from MarabouNetworkNNet:

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

        Attributes from MarabouNetwork

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
        super(MarabouNetworkNNetIPQ,self).__init__(filename=filename,property_filename=property_filename,perform_sbt=perform_sbt,compute_ipq=compute_ipq)
        if compute_ipq:
            self.ipq1 = self.getMarabouQuery()
        else:
            self.ipq1 = MarabouCore.InputQuery()
        self.ipq2 = self.getMarabouQuery()
        if filename:
            MarabouCore.createInputQuery(self.ipq2,filename,property_filename)


    def computeIPQ(self):
        self.ipq1 = self.getMarabouQuery()

    def getInputQuery(self,networkFilename,propertyFilename):
        MarabouCore.createInputQuery(self.ipq2,networkFilename,propertyFilename)


 #   def readProperty(self,filename):
 #       MarabouCore.PropertyParser().parse(filename,self.ipq)


    # Re-tightens bounds on variables from the Input Query
    def tightenBounds(self):
        self.tightenInputBounds()
        self.tightenOutputBounds()
        self.tighten_fBounds()
        self.tighten_bBounds()

    def tightenInputBounds(self):
        for var in self.inputVars.flatten():
             true_lower_bound = self.ipq2.getLowerBound(var)
             true_upper_bound = self.ipq2.getUpperBound(var)
             if self.lowerBounds[var] < true_lower_bound:
                 self.setLowerBound(var,true_lower_bound)
                 print ('Adjusting lower bound for input variable',var,"to be",true_lower_bound)
             if self.upperBounds[var] > true_upper_bound:
                 self.setUpperBound(var,true_upper_bound)
                 print ("Adjusting upper bound for input variable",var,"to be",true_upper_bound)

    def tightenOutputBounds(self):
        for var in self.outputVars.flatten():
            true_lower_bound = self.ipq2.getLowerBound(var)
            true_upper_bound = self.ipq2.getUpperBound(var)
            if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                self.setLowerBound(var, true_lower_bound)
                print('Adjusting lower bound for output variable', var, "to be", true_lower_bound)
            if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                self.setUpperBound(var, true_upper_bound)
                print("Adjusting upper bound for output variable", var, "to be", true_upper_bound)

    def tighten_bBounds(self):
        for var in self.b_variables:
            true_lower_bound = self.ipq2.getLowerBound(var)
            true_upper_bound = self.ipq2.getUpperBound(var)
            if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                self.setLowerBound(var, true_lower_bound)
                #print('Adjusting lower bound for b variable', var, "to be", true_lower_bound)
            if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                self.setUpperBound(var, true_upper_bound)
                #print("Adjusting upper bound for b variable", var, "to be", true_upper_bound)

    def tighten_fBounds(self):
        for var in self.f_variables:
            true_lower_bound = self.ipq2.getLowerBound(var)
            true_upper_bound = self.ipq2.getUpperBound(var)
            if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                self.setLowerBound(var, true_lower_bound)
                #print('Adjusting lower bound for f variable', var, "to be", true_lower_bound)
            if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                self.setUpperBound(var, true_upper_bound)
                #print("Adjusting upper bound for f variable", var, "to be", true_upper_bound)

    def testInputBounds(self):
        for var in self.inputVars.flatten():
            print(var, ": between ", self.lowerBounds[var], " and ", self.upperBounds[var])

    def testOutputBounds(self):
        for var in self.outputVars.flatten():
            if self.lowerBoundExists(var) and self.upperBoundExists(var):
                print(var, ": between ", self.lowerBounds[var], " and ", self.upperBounds[var])

    def tightenBounds1(self):
        self.tightenInputBounds1()
        self.tightenOutputBounds1()
        self.tighten_fBounds1()
        self.tighten_bBounds1()


    def tightenInputBounds1(self):
        for var in self.inputVars.flatten():
             true_lower_bound = self.ipq1.getLowerBound(var)
             true_upper_bound = self.ipq1.getUpperBound(var)
             if self.lowerBounds[var] < true_lower_bound:
                 self.setLowerBound(var,true_lower_bound)
                 print('Adjusting lower bound for input variable', var, "to be", true_lower_bound)
             if self.upperBounds[var] > true_upper_bound:
                 self.setUpperBound(var,true_upper_bound)
                 print("Adjusting upper bound for input variable", var, "to be", true_upper_bound)

    def tightenOutputBounds1(self):
        for var in self.outputVars.flatten():
            true_lower_bound = self.ipq1.getLowerBound(var)
            true_upper_bound = self.ipq1.getUpperBound(var)
            if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                self.setLowerBound(var, true_lower_bound)
                print('Adjusting lower bound for output variable', var, "to be", true_lower_bound)
            if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                self.setUpperBound(var, true_upper_bound)
                print("Adjusting upper bound for output variable", var, "to be", true_upper_bound)

    def tighten_bBounds1(self):
        for var in self.b_variables:
            true_lower_bound = self.ipq1.getLowerBound(var)
            true_upper_bound = self.ipq1.getUpperBound(var)
            if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                self.setLowerBound(var, true_lower_bound)
                print('Adjusting lower bound for b variable', var, "to be", true_lower_bound)
            if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                self.setUpperBound(var, true_upper_bound)
                print("Adjusting upper bound for b variable", var, "to be", true_upper_bound)

    def tighten_fBounds1(self):
        for var in self.f_variables:
            true_lower_bound = self.ipq1.getLowerBound(var)
            true_upper_bound = self.ipq1.getUpperBound(var)
            if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                self.setLowerBound(var, true_lower_bound)
                print('Adjusting lower bound for f variable', var, "to be", true_lower_bound)
            if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                self.setUpperBound(var, true_upper_bound)
                print("Adjusting upper bound for f variable", var, "to be", true_upper_bound)

   #  """
   #  Evaluate the network directly, without Marabou
   #  Computes the output at a given layer (ouptut layer by default)
   #
   #
   #  Args:
   #      inputs (numpy array of floats): Network inputs to be evaluated
   #
   #  Returns:
   #      (numpy array of floats): Network output
   #
   # """
   #
   #  def evaluateNetworkToLayer(self, inputs, last_layer = 0, normalize_inputs=True, normalize_outputs=True, activate_output_layer=False):
   #      numLayers = self.numLayers
   #      inputSize = self.inputSize
   #      outputSize = self.outputSize
   #      biases = self.biases
   #      weights = self.weights
   #      mins = self.inputMinimums
   #      maxes = self.inputMaximums
   #      means = self.inputMeans
   #      ranges = self.inputRanges
   #
   #      #The default output layer is the last (output) layer
   #      if last_layer == 0:
   #          last_layer = numLayers
   #
   #      # Prepare the inputs to the neural network
   #      if (normalize_inputs):
   #          inputsNorm = np.zeros(inputSize)
   #          for i in range(inputSize):
   #              if inputs[i] < mins[i]:
   #                  inputsNorm[i] = (mins[i] - means[i]) / ranges[i]
   #              elif inputs[i] > maxes[i]:
   #                  inputsNorm[i] = (maxes[i] - means[i]) / ranges[i]
   #              else:
   #                  inputsNorm[i] = (inputs[i] - means[i]) / ranges[i]
   #      else:
   #          inputsNorm = inputs
   #
   #      # Evaluate the neural network
   #      for layer in range(last_layer-1):
   #          inputsNorm = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer], 0)
   #
   #      layer+=1
   #      #print layer
   #      #print last_layer
   #
   #
   #      if (activate_output_layer):
   #          outputs = np.maximum(np.dot(weights[layer], inputsNorm) + biases[layer], 0)
   #      else:
   #          outputs = np.dot(weights[layer], inputsNorm) + biases[layer]
   #
   #      # Undo output normalization
   #      if (normalize_outputs):
   #          for i in range(outputSize):
   #              outputs[i] = outputs[i] * ranges[layer-1] + means[layer-1]
   #
   #      return outputs

