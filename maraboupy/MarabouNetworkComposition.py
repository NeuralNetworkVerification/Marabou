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
        self.inputVars, self.outputVars = self.getInputOutputVars(filename, inputNames, outputNames, reindexOutputVars=True)

        network = MarabouNetworkONNX.MarabouNetworkONNX(filename, reindexOutputVars=reindexOutputVars, threshold=threshold)
        print(f'TG: input vars: {network.inputVars}')
        print(f'TG: output vars: {network.outputVars}')

        network.saveQuery('q1.ipq')
        self.ipqs.append('q1.ipq')
        self.ipqToInVars['q1.ipq'] = network.inputVars
        self.ipqToOutVars['q1.ipq'] = network.outputVars

        index = 2
        # while if post_split.onnx exists
        while os.path.exists('post_split.onnx'):
            # delete network
            del network
            network = MarabouNetworkONNX.MarabouNetworkONNX('post_split.onnx', reindexOutputVars=reindexOutputVars, threshold=threshold)
            network.saveQuery(f'q{index}.ipq')
            self.ipqs.append(f'q{index}.ipq')
            self.ipqToInVars[f'q{index}.ipq'] = network.inputVars
            self.ipqToOutVars[f'q{index}.ipq'] = network.outputVars
            # self.ipq_to_inVars['q{index}.ipq'] = network.inputVars
            index += 1
            print(network.inputVars)
            print(network.outputVars)
        print(self.ipqs)
        # print(inputQuery.inputVars)
        # MarabouNetworkONNX.readONNX(filename, reindexOutputVars=reindexOutputVars, threshold=threshold)
        # network, post_network_file = MarabouNetworkONNX.readONNX(filename, reindexOutputVars=reindexOutputVars, threshold=threshold)

    def solve(self):
        # https://github.com/wu-haoze/Marabou/blob/1a3ca6010b51bba792ef8ddd5e1ccf9119121bd8/resources/runVerify.py#L200-L225
        options = Marabou.createOptions(verbosity = 1)
        for i, ipqFile in enumerate(self.ipqs):
            # ipq = Marabou.load_query(ipqFile)
            # load inputquery
            ipq = Marabou.loadQuery(ipqFile)

            if i == 0:
                self.encodeInput(ipq)

            if i > 0:
                self.encodeCalculateInputBounds(ipq, i, bounds)
                # print(bounds)
            
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
                # print(f'TG: bounds: {bounds}')
            else:
                ret, bounds, stats = MarabouCore.calculateBounds(ipq, options)
                print(f'TG: bounds: {bounds}')
        print(f'TG: bounds: {bounds}')

    def encodeCalculateInputBounds(self, ipq, i, bounds):
        print('TG: encodeCalculateInputBounds')
        previousOutputVars = self.ipqToOutVars[f'q{i}.ipq']
        currentInputVars = self.ipqToInVars[f'q{i+1}.ipq']
        print('TG: ', self.ipqToOutVars[f'q{i+1}.ipq'])
        print(f'TG: previous output vars: {previousOutputVars}')
        print(f'TG: current input vars: {currentInputVars}')      
        
        for previousOutputVar, currentInputVar in zip(previousOutputVars, currentInputVars):
            for previousOutputVarElement, currentInputVarElement in zip(previousOutputVar.flatten(), currentInputVar.flatten()):
                print(f'TG: previous output var element: {previousOutputVarElement}')
                print(f'TG: current input var element: {currentInputVarElement}')
                print(f'TG: bounds: {bounds[previousOutputVarElement]}')
                ipq.setLowerBound(currentInputVarElement, bounds[previousOutputVarElement][0])
                ipq.setUpperBound(currentInputVarElement, bounds[previousOutputVarElement][1])

    def encodeInput(self, ipq):
        inputVars = self.ipqToInVars['q1.ipq']
        # print('TG: ', self.ipqToOutVars[f'q1.ipq'])
        # print('TG: ', self.ipqToInVars[f'q2.ipq'])
        for array in inputVars:
            for var in array.flatten():
                ipq.setLowerBound(var, self.lowerBounds[var])
                ipq.setUpperBound(var, self.upperBounds[var])

    def encodeOutput(self, ipq, i):
        outputVars = self.ipqToOutVars[f'q{i+1}.ipq']
        originalOutputVars = self.outputVars
        print(f'TG: original output vars: {originalOutputVars}')
        print(f'TG: output vars: {outputVars}')
        
        for originalOutputVar, outputVar in zip(originalOutputVars, outputVars):
            for originalOutputVarElement, outputVarElement in zip(originalOutputVar.flatten(), outputVar.flatten()):                
                if originalOutputVarElement in self.lowerBounds:
                    ipq.setLowerBound(outputVarElement, self.lowerBounds[originalOutputVarElement])
                if originalOutputVarElement in self.upperBounds:
                    ipq.setUpperBound(outputVarElement, self.upperBounds[originalOutputVarElement])


    # def getInputOutVars(self, filename, inputNames, outputNames, reindexOutputVars=True, threshold=None):
    def getInputOutputVars(self, filename, inputNames, outputNames, reindexOutputVars=True):
        """Read an ONNX file and create a MarabouNetworkONNX object

        Args:
            filename: (str): Path to the ONNX file
            inputNames: (list of str): List of node names corresponding to inputs
            outputNames: (list of str): List of node names corresponding to outputs
            reindexOutputVars: (bool): Reindex the variables so that the output variables are immediate after input variables.

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
                # self.inputVars += [np.array(self.varMap[node.name])]
                inputVars += [v]
        print(f'TG: get input vars: {inputVars}')

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
        print(f'TG: get ouput vars: {outputVars}')
        
        return inputVars, outputVars

        # # Recursively create remaining shapes and equations as needed
        # for outputName in self.outputNames: # TG: 
        #     print(f'TG: outputName: {outputName}')
        #     self.makeGraphEquations(outputName, True)
    


        # If the given inputNames/outputNames specify only a portion of the network, then we will have
        # shape information saved not relevant to the portion of the network. Remove extra shapes.
        # self.cleanShapes() # TG: needed?

        # TG: needed?

        # if reindexOutputVars:
        #     # Other Marabou input parsers assign output variables immediately after input variables and before any
        #     # intermediate variables. This function reassigns variable numbering to match other parsers.
        #     # If this is skipped, the output variables will be the last variables defined.
        #     self.reassignOutputVariables()
        # else:
        #     self.outputVars = [self.varMap[outputName] for outputName in self.outputNames]
        


    def makeNewVariables(self, nodeName):
        """Assuming the node's shape is known, return a set of new variables in the same shape

        Args:
            nodeName (str): Name of node

        Returns:
            (numpy array): Array of variable numbers

        :meta private:
        """
        # assert nodeName not in self.varMap
        shape = self.shapeMap[nodeName]
        size = np.prod(shape)
        v = np.array([self.getNewVariable() for _ in range(size)]).reshape(shape)
        # self.varMap[nodeName] = v
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
            raise RuntimeError("Cannot set bounds on input or output variables")
        

    def setUpperBound(self, x, v):
        """Function to set upper bound for variable

        Args:
            x (int): Variable number to set
            v (float): Value representing upper bound
        """
        if any(x in arr for arr in self.inputVars) or any(x in arr for arr in self.outputVars):
            self.upperBounds[x] = v
        else:
            raise RuntimeError("Cannot set bounds on input or output variables")