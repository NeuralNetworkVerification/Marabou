

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



import warnings

import MarabouNetworkNNetExtended
import MarabouCore

class MarabouNetworkNNetIPQ(MarabouNetworkNNetExtended.MarabouNetworkNNetExtended):
    """
    Class that implements a MarabouNetwork from an NNet file.
    """
    def __init__(self, filename="", property_filename = "", normalize = False, compute_internal_ipq = False,
                 use_nlr = False):
        """
        Constructs a MarabouNetworkNNetIPQ object from an .nnet file.
        Computes InputQuery, potentially in two ways
        internal_ipq is computed from the MarabouNetworkNNet object itself
        marabou_ipq is computed directly from the nnet file

        Args:
            filename: path to the .nnet file.
            property_filename: path to the property file

        Attributes:
            internal_ipq             an Input Query object containing the Input Query corresponding to the network
            marabou_ipq             an Input Query object created from the file (and maybe property file)

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
            outputMean       (float) Mean value of outputs
            outputRange      (float) Range of output values
            weights          (list of list of lists) Outer index corresponds to layer
                                number.
            biases           (list of lists) Outer index corresponds to layer number.

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
        super(MarabouNetworkNNetIPQ,self).__init__(filename=filename, property_filename=property_filename,
                                                   normalize=normalize, compute_internal_ipq=compute_internal_ipq,
                                                   use_nlr=use_nlr)
        if compute_internal_ipq:
            self.computeIPQInternally()
        else:
            self.internal_ipq = None

        if filename:
            self.marabou_ipq = MarabouCore.InputQuery()
            self.computeMarabouIPQ(network_filename=filename, property_filename=property_filename)
        else:
            self.marabou_ipq = None

    def getIPQ(self):
        return self.marabou_ipq if self.marabou_ipq else self.internal_ipq

    def computeIPQInternally(self):
        self.internal_ipq = self.getMarabouQuery()

    def computeMarabouIPQ(self, network_filename, property_filename):
        MarabouCore.createInputQuery(self.marabou_ipq, network_filename, property_filename)

    def tightenBounds(self):
        # Re-tightens bounds on variables from the Input Query computed directly from the file (more accurate)
        if not self.marabou_ipq:
            warnings.warn('Marabou IPQ has not been computed. Failed to adjust bounds.')
            return
        self.tightenInputBounds()
        self.tightenOutputBounds()
        self.tighten_fBounds()
        self.tighten_bBounds()

    def tightenInputBounds(self):
        # print(self.inputVars)
        if self.marabou_ipq:
            for var in self.inputVars.flatten():
                true_lower_bound = self.marabou_ipq.getLowerBound(var)
                true_upper_bound = self.marabou_ipq.getUpperBound(var)
                if self.lowerBounds[var] < true_lower_bound:
                    self.setLowerBound(var,true_lower_bound)
                    print ('Adjusting lower bound for input variable',var,"to be",true_lower_bound)
                if self.upperBounds[var] > true_upper_bound:
                    self.setUpperBound(var,true_upper_bound)
                    print ("Adjusting upper bound for input variable",var,"to be",true_upper_bound)

    def tightenOutputBounds(self):
        if self.marabou_ipq:
            for var in self.outputVars.flatten():
                true_lower_bound = self.marabou_ipq.getLowerBound(var)
                true_upper_bound = self.marabou_ipq.getUpperBound(var)
                if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                    self.setLowerBound(var, true_lower_bound)
                    print('Adjusting lower bound for output variable', var, "to be", true_lower_bound)
                if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                    self.setUpperBound(var, true_upper_bound)
                    print("Adjusting upper bound for output variable", var, "to be", true_upper_bound)

    def tighten_bBounds(self):
        if self.marabou_ipq:
            for var in self.b_variables:
                true_lower_bound = self.marabou_ipq.getLowerBound(var)
                true_upper_bound = self.marabou_ipq.getUpperBound(var)
                if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                    self.setLowerBound(var, true_lower_bound)
                    #print('Adjusting lower bound for b variable', var, "to be", true_lower_bound)
                if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                    self.setUpperBound(var, true_upper_bound)
                    #print("Adjusting upper bound for b variable", var, "to be", true_upper_bound)

    def tighten_fBounds(self):
        if self.marabou_ipq:
            for var in self.f_variables:
                true_lower_bound = self.marabou_ipq.getLowerBound(var)
                true_upper_bound = self.marabou_ipq.getUpperBound(var)
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

    def tightenBoundsFromInternalIPQ(self):
        # Re-tightens bounds on variables from the Input Query computed from the MarabouNetwork object (less accurate)
        if not self.internal_ipq:
            warnings.warn('Internal IPQ has not been computed. Failed to adjust bounds.')
            return
        self.tightenInputBoundsFromInternalIPQ()
        self.tightenOutputBoundsFromInternalIPQ()
        self.tighten_fBoundsFromInternalIPQ()
        self.tighten_bBoundsFromInternalIPQ()


    def tightenInputBoundsFromInternalIPQ(self):
        if self.internal_ipq:
            for var in self.inputVars.flatten():
                 true_lower_bound = self.internal_ipq.getLowerBound(var)
                 true_upper_bound = self.internal_ipq.getUpperBound(var)
                 if self.lowerBounds[var] < true_lower_bound:
                     self.setLowerBound(var,true_lower_bound)
                     print('Adjusting lower bound for input variable', var, "to be", true_lower_bound)
                 if self.upperBounds[var] > true_upper_bound:
                     self.setUpperBound(var,true_upper_bound)
                     print("Adjusting upper bound for input variable", var, "to be", true_upper_bound)

    def tightenOutputBoundsFromInternalIPQ(self):
        if self.internal_ipq:
            for var in self.outputVars.flatten():
                true_lower_bound = self.internal_ipq.getLowerBound(var)
                true_upper_bound = self.internal_ipq.getUpperBound(var)
                if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                    self.setLowerBound(var, true_lower_bound)
                    print('Adjusting lower bound for output variable', var, "to be", true_lower_bound)
                if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                    self.setUpperBound(var, true_upper_bound)
                    print("Adjusting upper bound for output variable", var, "to be", true_upper_bound)

    def tighten_bBoundsFromInternalIPQ(self):
        if self.internal_ipq:
            for var in self.b_variables:
                true_lower_bound = self.internal_ipq.getLowerBound(var)
                true_upper_bound = self.internal_ipq.getUpperBound(var)
                if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                    self.setLowerBound(var, true_lower_bound)
                    print('Adjusting lower bound for b variable', var, "to be", true_lower_bound)
                if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                    self.setUpperBound(var, true_upper_bound)
                    print("Adjusting upper bound for b variable", var, "to be", true_upper_bound)

    def tighten_fBoundsFromInternalIPQ(self):
        if self.internal_ipq:
            for var in self.f_variables:
                true_lower_bound = self.internal_ipq.getLowerBound(var)
                true_upper_bound = self.internal_ipq.getUpperBound(var)
                if (not self.lowerBoundExists(var) or self.lowerBounds[var] < true_lower_bound):
                    self.setLowerBound(var, true_lower_bound)
                    print('Adjusting lower bound for f variable', var, "to be", true_lower_bound)
                if (not self.upperBoundExists(var) or self.upperBounds[var] > true_upper_bound):
                    self.setUpperBound(var, true_upper_bound)
                    print("Adjusting upper bound for f variable", var, "to be", true_upper_bound)

