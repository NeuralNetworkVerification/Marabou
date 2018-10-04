from . import MarabouCore
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

        for e in self.equList:
            eq = MarabouCore.Equation(e.EquationType)
            for (c, v) in e.addendList:
                assert v < self.numVars
                eq.addAddend(c, v)
            eq.setScalar(e.scalar)
            ipq.addEquation(eq)

        for r in self.reluList:
            assert r[1] < self.numVars and r[0] < self.numVars
            MarabouCore.addReluConstraint(ipq, r[0], r[1])

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

    def solve(self, filename="", verbose=True):
        """
        Function to solve query represented by this network
        Arguments:
            filename: (string) path to redirect output to
            verbose: (bool) whether to print out solution
        Returns:
            vals: (dict: int->float) empty if UNSAT, else SATisfying solution
            stats: (Statistics) a Statistics object as defined in Marabou,
                    it has multiple methods that provide information related
                    to how an input query was solved.
        """
        ipq = self.getMarabouQuery()
        vals, stats = MarabouCore.solve(ipq, filename)
        if verbose:
            if len(vals)==0:
                print("UNSAT")
            else:
                print("SAT")
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
            filename: (string) path to redirect output to
        Returns:
            None
        """
        ipq = self.getMarabouQuery()
        MarabouCore.saveQuery(ipq, filename)

    def loadQuery(self, filename="", verbose=True):
        """
        Function to solve query represented by this network
        Arguments:
            filename: (string) path to redirect output to
            verbose: (bool) whether to print out solution
        Returns:
            vals: (dict: int->float) empty if UNSAT, else SATisfying solution
            stats: (Statistics) a Statistics object as defined in Marabou,
                    it has multiple methods that provide information related
                    to how an input query was solved.
        """
        #ipq = self.getMarabouQuery()
        ipq = MarabouCore.loadQuery(filename)
        vals, stats = MarabouCore.solve(ipq, filename)
        if verbose:
            if len(vals)==0:
                print("UNSAT")
            else:
                print("SAT")
                for i in range(self.inputVars.size):
                    print("input {} = {}".format(i, vals[self.inputVars.item(i)]))

                for i in range(self.outputVars.size):
                    print("output {} = {}".format(i, vals[self.outputVars.item(i)]))

        return [vals, stats]

    def evaluateWithMarabou(self, inputValues, filename="evaluateWithMarabou.log"):
        """
        Function to evaluate network at a given point using Marabou as solver
        Arguments:
            inputValues: list of (np arrays) representing input to network
            filename: (string) path to redirect output
        Returns:
            outputValues: (np array) representing output of network
        """
        print("Evaluating with Marabou\n")
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

        outputDict = MarabouCore.solve(ipq, filename)
        outputValues = outputVars.reshape(-1).astype(np.float64)
        for i in range(len(outputValues)):
            outputValues[i] = (outputDict[0])[outputValues[i]]
        outputValues = outputValues.reshape(outputVars.shape)
        return outputValues

    def evaluate(self, inputValues, useMarabou=True):
        """
        Function to evaluate network at a given point
        Arguments:
            inputValues: list of (np arrays) representing input to network
            useMarabou: (bool) whether to use Marabou solver or TF/NNet
        Returns:
            outputValues: (np array) representing output of network
        """
        if useMarabou:
            return self.evaluateWithMarabou(inputValues)
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
