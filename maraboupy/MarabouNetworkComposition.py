'''
Top contributors (to current version):
    - Teruhiro Tagomori

This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

MarabouNetworkComposition represents neural networks with piecewise linear constraints derived from the ONNX format
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

    Args:
        filename (str): Path to the ONNX file
        inputNames: (list of str, optional): List of node names corresponding to inputs
        outputNames: (list of str, optional): List of node names corresponding to outputs
        reindexOutputVars: (bool, optional): Reindex the variables so that the output variables are immediate after input variables.
        threshold: (float, optional): Threshold for equation functions. If the absolute value of the lower bound of a variable is less than this value, the variable is considered to be 0.

    Returns:
        :class:`~maraboupy.Marabou.MarabouNetworkComposition.MarabouNetworkComposition`
    """
    def __init__(self, filename, inputNames=None, outputNames=None, reindexOutputVars=True, threshold=None):
        super().__init__()
        self.shapeMap = {}
        self.madeGraphEquations = []
        self.ipqs = []
        self.ipqToInVars = {}
        self.ipqToOutVars = {}
        self.inputVars, self.outputVars = self.getInputOutputVars(filename, inputNames, outputNames)

        network = MarabouNetworkONNX.MarabouNetworkONNX(filename, reindexOutputVars=reindexOutputVars, threshold=threshold)

        network.saveQuery('q1.ipq')
        self.ipqs.append('q1.ipq')
        self.ipqToInVars['q1.ipq'] = network.inputVars
        self.ipqToOutVars['q1.ipq'] = network.outputVars

        index = 2

        while os.path.exists('post_split.onnx'):
            # delete network
            del network
            network = MarabouNetworkONNX.MarabouNetworkONNX('post_split.onnx', reindexOutputVars=reindexOutputVars, threshold=threshold)
            network.saveQuery(f'q{index}.ipq')
            self.ipqs.append(f'q{index}.ipq')
            self.ipqToInVars[f'q{index}.ipq'] = network.inputVars
            self.ipqToOutVars[f'q{index}.ipq'] = network.outputVars
            index += 1

    def solve(self):
        # https://github.com/wu-haoze/Marabou/blob/1a3ca6010b51bba792ef8ddd5e1ccf9119121bd8/resources/runVerify.py#L200-L225
        options = Marabou.createOptions(verbosity = 1) # TG: Option を引数でとる
        for i, ipqFile in enumerate(self.ipqs):
            # load input query
            ipq = Marabou.loadQuery(ipqFile)

            if i == 0:
                self.encodeInput(ipq)

            if i > 0:
                self.encodeCalculateInputBounds(ipq, i, bounds)
            
            if i == len(self.ipqs) - 1:
                self.encodeOutput(ipq, i)
                # exitCode, vals, _ = MarabouCore.solve(ipq, options)
                # print(f'TG: exit code: {exitCode}')
                # if exitCode == "sat":
                #     for j in range(len(self.inputVars)):
                #         for i in range(self.inputVars[j].size):
                #             print("input {} = {}".format(i, vals[self.inputVars[j].item(i)]))

                #     for j in range(len(self.outputVars)): #TG: Original のものを表示してしまっているので修正
                #         for i in range(self.outputVars[j].size):
                #             print("output {} = {}".format(i, vals[self.outputVars[j].item(i)]))
                ret, bounds, stats = MarabouCore.calculateBounds(ipq, options)
            else:
                ret, bounds, stats = MarabouCore.calculateBounds(ipq, options)

    def encodeCalculateInputBounds(self, ipq, i, bounds):
        previousOutputVars = self.ipqToOutVars[f'q{i}.ipq']
        currentInputVars = self.ipqToInVars[f'q{i+1}.ipq']    
        
        for previousOutputVar, currentInputVar in zip(previousOutputVars, currentInputVars):
            for previousOutputVarElement, currentInputVarElement in zip(previousOutputVar.flatten(), currentInputVar.flatten()):
                ipq.setLowerBound(currentInputVarElement, bounds[previousOutputVarElement][0])
                ipq.setUpperBound(currentInputVarElement, bounds[previousOutputVarElement][1])

    def encodeInput(self, ipq):
        inputVars = self.ipqToInVars['q1.ipq']
        for array in inputVars:
            for var in array.flatten():
                ipq.setLowerBound(var, self.lowerBounds[var])
                ipq.setUpperBound(var, self.upperBounds[var])

    def encodeOutput(self, ipq, i):
        outputVars = self.ipqToOutVars[f'q{i+1}.ipq']
        originalOutputVars = self.outputVars
        
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