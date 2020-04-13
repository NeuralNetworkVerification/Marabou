'''
/* *******************                                                        */
/*! \file MarabouNetwork.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Shantanu Thakoor, Andrew Wu
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

from maraboupy import MarabouCore
import numpy as np

class MarabouNetwork:
    """
    Abstract class representing general Marabou network
    Defines functions common to MarabouNetworkNnet and MarabouNetworkTF
    """
    def __init__(self):
        """
        Constructs a MarabouNetwork object and calls function to initialize
        """
        self.clear()

    def clear(self):
        """
        Reset values to represent empty network
        """
        self.numVars = 0
        self.equList = []
        self.reluList = []
        self.maxList = []
        self.varsParticipatingInConstraints = set()
        self.lowerBounds = dict()
        self.upperBounds = dict()
        self.inputVars = []
        self.outputVars = np.array([])

    def getNewVariable(self):
        """
        Function to request allocation of new variable

        Returns:
            varnum: (int) representing new variable
        """
        self.numVars += 1
        return self.numVars - 1

    def addEquation(self, x):
        """
        Function to add new equation to the network
        Arguments:
            x: (MarabouUtils.Equation) representing new equation
        """
        self.equList += [x]

    def setLowerBound(self, x, v):
        """
        Function to set lower bound for variable
        Arguments:
            x: (int) variable number to set
            v: (float) value representing lower bound
        """
        self.lowerBounds[x]=v

    def setUpperBound(self, x, v):
        """
        Function to set upper bound for variable
        Arguments:
            x: (int) variable number to set
            v: (float) value representing upper bound
        """
        self.upperBounds[x]=v

    def addRelu(self, v1, v2):
        """
        Function to add a new Relu constraint
        Arguments:
            v1: (int) variable representing input of Relu
            v2: (int) variable representing output of Relu
        """
        self.reluList += [(v1, v2)]
        self.varsParticipatingInConstraints.add(v1)
        self.varsParticipatingInConstraints.add(v2)

    def addMaxConstraint(self, elements, v):
        """
        Function to add a new Max constraint
        Arguments:
            elements: (set of int) variable representing input to max constraint
            v: (int) variable representing output of max constraint
        """
        self.maxList += [(elements, v)]
        self.varsParticipatingInConstraints.add(v)
        for i in elements:
            self.varsParticipatingInConstraints.add(i)

    def lowerBoundExists(self, x):
        """
        Function to check whether lower bound for a variable is known
        Arguments:
            x: (int) variable to check
        """
        return x in self.lowerBounds

    def upperBoundExists(self, x):
        """
        Function to check whether upper bound for a variable is known
        Arguments:
            x: (int) variable to check
        """
        return x in self.upperBounds

    def participatesInPLConstraint(self, x):
        """
        Function to check whether variable participates in any piecewise linear constraint in this network
        Arguments:
            x: (int) variable to check
        """
        # ReLUs
        return x in self.varsParticipatingInConstraints

    def getMarabouQuery(self):
        """
        Function to convert network into Marabou Query
        Returns:
            ipq: (MarabouCore.InputQuery) representing query
        """
        ipq = MarabouCore.InputQuery()
        ipq.setNumberOfVariables(self.numVars)

        i = 0
        for inputVarArray in self.inputVars:
            for inputVar in inputVarArray.flatten():
                ipq.markInputVariable(inputVar, i)
                i+=1

        i = 0
        for outputVar in self.outputVars.flatten():
            ipq.markOutputVariable(outputVar, i)
            i+=1

        for e in self.equList:
            eq = MarabouCore.Equation(e.EquationType)
            for (c, v) in e.addendList:
                assert v < self.numVars
                eq.addAddend(c, v)
            eq.setScalar(e.scalar)
            ipq.addEquation(eq)

        id_ = 0
        for r in self.reluList:
            assert r[1] < self.numVars and r[0] < self.numVars
            MarabouCore.addReluConstraint(ipq, r[0], r[1], id_)
            id_ += 1

        for m in self.maxList:
            assert m[1] < self.numVars
            for e in m[0]:
                assert e < self.numVars
            MarabouCore.addMaxConstraint(ipq, m[0], m[1])

        for l in self.lowerBounds:
            assert l < self.numVars
            ipq.setLowerBound(l, self.lowerBounds[l])

        for u in self.upperBounds:
            assert u < self.numVars
            ipq.setUpperBound(u, self.upperBounds[u])
            
        return ipq

    def solve(self, summaryFilePath="", fixedReluFilePath="", filename="", verbose=True, options=None):
        """
        Function to solve query represented by this network
        Arguments:
            filename: (string) path to redirect output to
            verbose: (bool) whether to print out solution after solve finishes
            timeout: (int) time in seconds when Marabou will time out
            verbosity: (int) determines how much Marabou prints during solving
                    0: print out minimal information
                    1: print out statistics only in the beginning and the end
                    2: print out statistics during solving
        Returns:
            vals: (dict: int->float) empty if UNSAT, else SATisfying solution
            stats: (Statistics) a Statistics object as defined in Marabou,
                    it has multiple methods that provide information related
                    to how an input query was solved.
        """
        ipq = self.getMarabouQuery()
        if options == None:
            options = MarabouCore.Options()
        if options._focusLayer > 0:
            centroid = []
            i = 0
            for inputVarArray in self.inputVars:
                for inputVar in inputVarArray.flatten():
                    centroid.append((self.lowerBounds[i] + self.upperBounds[i])/2)
                    i+=1
            shape = np.array(self.inputVars).shape[1:]
            centroid = np.array(centroid).reshape(shape)
            pattern = self.getReluPattern(centroid)
            i = 0
            for layer in range(min(len(pattern), options._focusLayer)):
                for p in pattern[layer]:
                    if options._biasStrategy == "centroid":
                        MarabouCore.setDirection(ipq, i, p)
                    elif options._biasStrategy == "random":
                        r = np.random.choice([0, 1])
                        MarabouCore.setDirection(ipq, i, r)
                    i += 1

        vals, stats = MarabouCore.solve(ipq, options, summaryFilePath, fixedReluFilePath, filename)
        if verbose:
            if stats.hasTimedOut():
                print("TO")
            elif len(vals)==0:
                print("unsat")
            else:
                print("sat")
                for j in range(len(self.inputVars)):
                    for i in range(self.inputVars[j].size):
                        print("input {} = {}".format(i, vals[self.inputVars[j].item(i)]))

                for i in range(self.outputVars.size):
                    print("output {} = {}".format(i, vals[self.outputVars.item(i)]))

        return [vals, stats]

    def saveQuery(self, filename=""):
        """
        Serializes the inputQuery in the given filename
        Arguments:
            filename: (string) file to write serialized inputQuery
        Returns:
            None
        """
        ipq = self.getMarabouQuery()
        MarabouCore.saveQuery(ipq, filename)

    def evaluateWithMarabou(self, inputValues, filename="evaluateWithMarabou.log", options=None):
        """
        Function to evaluate network at a given point using Marabou as solver
        Arguments:
            inputValues: list of (np arrays) representing input to network
            filename: (string) path to redirect output
        Returns:
            outputValues: (np array) representing output of network
        """
        inputVars = self.inputVars # list of numpy arrays
        outputVars = self.outputVars

        inputDict = dict()
        inputVarList = np.concatenate(inputVars, axis=-1).ravel()
        inputValList = np.concatenate(inputValues).ravel()
        assignList = zip(inputVarList, inputValList)
        for x in assignList:
            inputDict[x[0]] = x[1]

        ipq = self.getMarabouQuery()
        for k in inputDict:
            ipq.setLowerBound(k, inputDict[k])
            ipq.setUpperBound(k, inputDict[k])

        if options == None:
            options = MarabouCore.Options()
        outputDict = MarabouCore.solve(ipq, options, "", "", filename)
        outputValues = outputVars.reshape(-1).astype(np.float64)
        for i in range(len(outputValues)):
            outputValues[i] = (outputDict[0])[outputValues[i]]
        outputValues = outputValues.reshape(outputVars.shape)
        return outputValues

    def evaluate(self, inputValues, useMarabou=True, options=None):
        """
        Function to evaluate network at a given point
        Arguments:
            inputValues: list of (np arrays) representing input to network
            useMarabou: (bool) whether to use Marabou solver or TF/NNet
        Returns:
            outputValues: (np array) representing output of network
        """
        if useMarabou:
            return self.evaluateWithMarabou(inputValues, options=options)
        if not useMarabou:
            return self.evaluateWithoutMarabou(inputValues)

    def findError(self, inputs):
        """
        Function to find error between Marabou solver and TF/Nnet at a given point
        Arguments:
            inputs: (np array) representing input to network
        Returns:
            err: (np array) representing error in each output variable
        """
        outMar = self.evaluate(inputs, useMarabou=True)
        outNotMar = self.evaluate(inputs, useMarabou=False)
        err = np.abs(outMar - outNotMar)
        return err
