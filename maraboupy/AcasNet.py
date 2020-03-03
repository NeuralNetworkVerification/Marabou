'''
/* *******************                                                        */
/*! \file AcasNet.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Haoze Wu
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief Parser class that uses native Marabou's parser
 **
 ** [[ Add lengthier description here ]]
 **/
'''

from maraboupy import MarabouCore
from maraboupy.Marabou import createOptions
import numpy as np

class AcasNet:
    """
    Class representing AcasXU Marabou network
    """
    def __init__(self, network_path, property_path="", lbs=None, ubs=None):
        """
        Constructs a MarabouNetwork object and calls function to initialize
        """
        self.network_path = network_path
        self.getMarabouQuery(property_path)
        if not(lbs is None and ubs is None):
            assert(len(lbs) == len(ubs))
            assert(len(lbs) == self.ipq.getNumInputVariables())
            for i in range(len(lbs)):
                self.setInputLowerBound(i, lbs[i])
                self.setInputUpperBound(i, ubs[i])



    def getMarabouQuery(self, property_path=""):
        self.ipq = MarabouCore.InputQuery()
        MarabouCore.createInputQuery(self.ipq, self.network_path, property_path)

    def setInputLowerBound(self, input_ind, scalar):
        assert(input_ind < self.ipq.getNumInputVariables())
        variable = self.ipq.inputVariableByIndex(input_ind)
        if self.ipq.getLowerBound(variable) < scalar:
            self.ipq.setLowerBound(variable, scalar)

    def setInputUpperBound(self, input_ind, scalar):
        assert(input_ind < self.ipq.getNumInputVariables())
        variable = self.ipq.inputVariableByIndex(input_ind)
        if self.ipq.getUpperBound(variable) > scalar:
            self.ipq.setUpperBound(variable, scalar)

    def getInputRanges(self):
        inputMins = []
        inputMaxs = []
        for input_ind in range(self.ipq.getNumInputVariables()):
            variable = self.ipq.inputVariableByIndex(input_ind)
            inputMins.append(self.ipq.getLowerBound(variable))
            inputMaxs.append(self.ipq.getUpperBound(variable))
        return np.array(inputMins), np.array(inputMaxs)


    def solve(self, filename="", timeout=0):
        """
        Function to solve query represented by this network
        Arguments:
            filename: (string) path to redirect output to
        Returns:
            vals: (dict: int->float) empty if UNSAT, else the
                  satisfying assignment to the input and output variables
            stats: (Statistics) the Statistics object as defined in Marabou
        """
        options = createOptions(timeoutInSeconds=timeout)
        vals, stats = MarabouCore.solve(self.ipq, options, filename)
        assignment = []
        if len(vals) > 0:
            for i in range(self.ipq.getNumInputVariables()):
                assignment.append("input {} = {}".format(i, vals[self.ipq.inputVariableByIndex(i)]))
            for i in range(self.ipq.getNumOutputVariables()):
                assignment.append("Output {} = {}".format(i, vals[self.ipq.outputVariableByIndex(i)]))
        return [assignment, stats]
