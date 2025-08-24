'''
Top contributors (to current version):
    - Teruhiro Tagomori

This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

MarabouNetworkComposition represents split subnets of a neural network with piecewise linear constraints derived from the ONNX format
'''

import numpy as np
import onnx
import sys
import pathlib
import os
sys.path.insert(0, os.path.join(str(pathlib.Path(__file__).parent.absolute()), "../"))

from maraboupy import Marabou
from maraboupy import MarabouCore
from maraboupy import MarabouNetwork
from maraboupy import MarabouNetworkONNX

class MarabouNetworkComposition(MarabouNetwork.MarabouNetwork):
    """Constructs a MarabouNetworkComposition object from an ONNX file
        This class splits into subnets every time the number of linear equations reaches maxNumberOfLinearEquations.
        It provides the function to propagate bounds for each subnet.

    Args:
        filename (str): Path to the ONNX file
        inputNames: (list of str, optional): List of node names corresponding to inputs
        outputNames: (list of str, optional): List of node names corresponding to outputs
        maxNumberOfLinearEquations (int, optional): Threshold for the number of linear equations.
                                                    If the number of linear equations is greater than this threshold,
                                                    the network will be split into two networks. Defaults to None.

    Returns:
        :class:`~maraboupy.Marabou.MarabouNetworkComposition.MarabouNetworkComposition`
    """
    def __init__(self, filename, inputNames=None, outputNames=None, maxNumberOfLinearEquations=None):
        super().__init__()
        self.shapeMap = {}
        self.madeGraphEquations = []
        self.ipqs = []
        self.ipqToInVars = {}
        self.ipqToOutVars = {}
        self.inputVars, self.outputVars = self.getInputOutputVars(filename, inputNames, outputNames)

        # Instantiate the first subnet
        network = MarabouNetworkONNX.MarabouNetworkONNX(filename, maxNumberOfLinearEquations=maxNumberOfLinearEquations)

        savedInputQueryName = 'q1.ipq'
        network.saveQuery(savedInputQueryName)
        self.ipqs.append(savedInputQueryName)
        self.ipqToInVars[savedInputQueryName] = network.inputVars
        self.ipqToOutVars[savedInputQueryName] = network.outputVars

        # index of ipq file
        index = 2

        while os.path.exists('post_split.onnx'):
            # delete network
            del network

            # Instantiate the next subnet
            network = MarabouNetworkONNX.MarabouNetworkONNX('post_split.onnx',
                                                            maxNumberOfLinearEquations=maxNumberOfLinearEquations)
            # name of the input query file
            savedInputQueryName = f'q{index}.ipq'

            # save input query
            network.saveQuery(savedInputQueryName)

            # append input query to the lsit
            self.ipqs.append(savedInputQueryName)

            # save input and output variables so that this can map them to the next input query
            self.ipqToInVars[savedInputQueryName] = network.inputVars
            self.ipqToOutVars[savedInputQueryName] = network.outputVars
            
            # increment index
            index += 1

    def solve(self, filename="", verbose=True, options=None):
        """Function to solve query represented by this network

        Args:
            filename (string): Path for redirecting output (Only for the last subnet)
            verbose (bool): If true, print out solution after solve finishes
            options (:class:`~maraboupy.MarabouCore.Options`): Object for specifying Marabou options, defaults to None

        Returns:
            (tuple): tuple containing:
                - exitCode (str): A string representing the exit code (unsat/TIMEOUT/ERROR/UNKNOWN/QUIT_REQUESTED).
                - vals (Dict[int, float]): Empty dictionary. This is for compatibility with MarabouNetwork.
                - stats (:class:`~maraboupy.MarabouCore.Statistics`): A Statistics object to how Marabou performed (Only for the last subnet)
        """
        if options == None:
            options = MarabouCore.Options()        
        
        for i, ipqFile in enumerate(self.ipqs):
            # load input query
            ipq = Marabou.loadQuery(ipqFile)

            # If the first subnet, encode input variables with the input bounds of the original network
            if i == 0:
                self.encodeInput(ipq)

            # If not the first subnet, encode input variables with the output bounds of the previous subwork
            if i > 0:
                self.encodeCalculateInputBounds(ipq, i, bounds)
            
            if i == len(self.ipqs) - 1:
                # If the last subnet, encode output variables with the output bounds of the original network
                self.encodeOutput(ipq, i)

                # If the last subnet, propagate bounds and return the exit code, values, and statistics
                exitCode, bounds, stats = MarabouCore.calculateBounds(ipq, options, str(filename))
                if exitCode == "":
                    exitCode = "UNKNOWN"
                if verbose:
                    print(exitCode)
                return [exitCode, {}, stats]
            else:
                # If not the last subnet, propagate bounds
                _, bounds, _ = MarabouCore.calculateBounds(ipq, options)

    def encodeCalculateInputBounds(self, ipq, i, bounds):
        """Function to encode input variables and set bounds for the current subnet

        Args:
            ipq (:class:`~maraboupy.MarabouCore.InputQuery`): InputQuery object to encode input variables
            i (int): Index of the previous subnet
            bounds (dict): Dictionary containing bounds for variables of the previous subnet

        Returns:
            None

        :meta private:
        """
        # Output variables of the previous subnet
        previousOutputVars = self.ipqToOutVars[f'q{i}.ipq']

        # Input variables of the current subnet
        currentInputVars = self.ipqToInVars[f'q{i+1}.ipq']    
        
        # Set bounds for the current subnet
        for previousOutputVar, currentInputVar in zip(previousOutputVars, currentInputVars):
            for previousOutputVarElement, currentInputVarElement in zip(previousOutputVar.flatten(), currentInputVar.flatten()):
                ipq.setLowerBound(currentInputVarElement, bounds[previousOutputVarElement][0])
                ipq.setUpperBound(currentInputVarElement, bounds[previousOutputVarElement][1])

    def encodeInput(self, ipq):
        """Function to encode input variables
        
        Args:
            ipq (:class:`~maraboupy.MarabouCore.InputQuery`): InputQuery object to encode input variables
        Returns:
            None

        :meta private:
        """
        inputVars = self.ipqToInVars['q1.ipq']

        # Set bounds for the first subnet
        for array in inputVars:
            for var in array.flatten():
                ipq.setLowerBound(var, self.lowerBounds[var])
                ipq.setUpperBound(var, self.upperBounds[var])

    def encodeOutput(self, ipq, i):
        """Function to encode output variables
        Args:
            ipq:    (:class:`~maraboupy.MarabouCore.InputQuery`): InputQuery object to encode output variables
            i:      (int): Index of the previous subnet
        
        Returns:
            None

        :meta private:
        """
        # Output variables of the current subnet
        outputVars = self.ipqToOutVars[f'q{i+1}.ipq']

        # Set bounds for the current subnet
        originalOutputVars = self.outputVars
        
        # Set bounds for the last subnet
        for originalOutputVar, outputVar in zip(originalOutputVars, outputVars):
            for originalOutputVarElement, outputVarElement in zip(originalOutputVar.flatten(), outputVar.flatten()):                
                if originalOutputVarElement in self.lowerBounds:
                    ipq.setLowerBound(outputVarElement, self.lowerBounds[originalOutputVarElement])
                if originalOutputVarElement in self.upperBounds:
                    ipq.setUpperBound(outputVarElement, self.upperBounds[originalOutputVarElement])

    def getInputOutputVars(self, filename, inputNames, outputNames):
        """Get input and output variables of an original network

        Args:
            filename: (str): Path to the ONNX file
            inputNames: (list of str): List of node names corresponding to inputs
            outputNames: (list of str): List of node names corresponding to outputs

        :meta private:
        """
        self.filename = filename
        self.graph = onnx.load(filename).graph

        # Get default inputs/outputs if no names are provided
        if not inputNames:
            assert len(self.graph.input) >= 1
            initNames = [node.name for node in self.graph.initializer]
            inputNames = [inp.name for inp in self.graph.input if inp.name not in initNames]
        if not outputNames:
            assert len(self.graph.output) >= 1
            initNames = [node.name for node in self.graph.initializer]
            outputNames = [out.name for out in self.graph.output if out.name not in initNames]
        elif isinstance(outputNames, str):
            outputNames = [outputNames]

        # Check that input/outputs are in the graph
        for name in inputNames:
            if not len([nde for nde in self.graph.node if name in nde.input]):
                raise RuntimeError("Input %s not found in graph!" % name)
        for name in outputNames:
            if not len([nde for nde in self.graph.node if name in nde.output]):
                raise RuntimeError("Output %s not found in graph!" % name)

        self.inputNames = inputNames
        self.outputNames = outputNames

        # Process the shapes and values of the graph while making Marabou equations and constraints
        self.foundnInputFlags = 0

        # Add shapes for the graph's inputs
        inputVars = []
        for node in self.graph.input:
            self.shapeMap[node.name] = list([dim.dim_value if dim.dim_value > 0 else 1 for dim in node.type.tensor_type.shape.dim])

            # If we find one of the specified inputs, create new variables
            if node.name in self.inputNames:
                self.madeGraphEquations += [node.name]
                self.foundnInputFlags += 1
                v = self.makeNewVariables(node.name)
                inputVars += [v]

        # Add shapes for the graph's outputs
        outputVars = []
        for node in self.graph.output:
            self.shapeMap[node.name] = list([dim.dim_value if dim.dim_value > 0 else 1 for dim in node.type.tensor_type.shape.dim])

            # If we find one of the specified inputs, create new variables
            if node.name in self.outputNames:
                self.madeGraphEquations += [node.name]
                self.foundnInputFlags += 1
                v = self.makeNewVariables(node.name)
                outputVars += [v]
        return inputVars, outputVars

    def makeNewVariables(self, nodeName):
        """Assuming the node's shape is known, return a set of new variables in the same shape

        Args:
            nodeName (str): Name of node

        Returns:
            (numpy array): Array of variable numbers

        :meta private:
        """
        shape = self.shapeMap[nodeName]
        size = np.prod(shape)
        v = np.array([self.getNewVariable() for _ in range(size)]).reshape(shape)
        assert all([np.equal(np.mod(i, 1), 0) for i in v.reshape(-1)]) # check if integers
        return v

    def setLowerBound(self, x, v):
        """Function to set lower bound for variable

        Args:
            x (int): Variable number to set
            v (float): Value representing lower bound
        """
        if any(x in arr for arr in self.inputVars) or any(x in arr for arr in self.outputVars):
            self.lowerBounds[x] = v
        else:
            raise RuntimeError("Can set bounds only on either input or output variables")
        
    def setUpperBound(self, x, v):
        """Function to set upper bound for variable

        Args:
            x (int): Variable number to set
            v (float): Value representing upper bound
        """
        if any(x in arr for arr in self.inputVars) or any(x in arr for arr in self.outputVars):
            self.upperBounds[x] = v
        else:
            raise RuntimeError("Can set bounds only on either input or output variables")
