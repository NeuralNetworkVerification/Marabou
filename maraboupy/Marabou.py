'''
/* *******************                                                        */
/*! \file Marabou.py
 ** \verbatim
 ** Top contributors (to current version):
 **   Christopher Lazarus, Kyle Julian, Andrew Wu
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
import warnings
from maraboupy.MarabouCore import *

# Import parsers if required packages are installed
try:
    from maraboupy.MarabouNetworkNNet import *
except ImportError:
    warnings.warn("NNet parser is unavailable because the numpy package is not installed")
try:
    from maraboupy.MarabouNetworkTF import *
except ImportError:
    warnings.warn("Tensorflow parser is unavailable because tensorflow package is not installed")
try:
    from maraboupy.MarabouNetworkONNX import *
except ImportError:
    warnings.warn("ONNX parser is unavailable because onnx or onnxruntime packages are not installed")

def read_nnet(filename, normalize=False):
    """
    Constructs a MarabouNetworkNnet object from a .nnet file

    Args:
        filename: (string) path to the .nnet file.
        normalize: (bool) If true, incorporate input/output normalization
                      into first and last layers of network
    Returns:
        marabouNetworkNNet: (MarabouNetworkNNet) representing network
    """
    return MarabouNetworkNNet(filename, normalize=normalize)


def read_tf(filename, inputNames=None, outputName=None, modelType="frozen", savedModelTags=[]):
    """
    Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf

    Args:
        filename: (string) If savedModel is false, path to the frozen graph .pb file.
                           If savedModel is true, path to SavedModel folder, which
                           contains .pb file and variables subdirectory.
        inputNames: (list of strings) optional, list of operation names corresponding to inputs.
        outputName: (string) optional, name of operation corresponding to output.
        modelType: (string) optional, type of model to read. The default is "frozen" for a frozen graph.
                            Can also use "savedModel_v1" or "savedModel_v2" for the SavedModel format
                            created from either tensorflow versions 1.X or 2.X respectively.
        savedModelTags: (list of strings) If loading a SavedModel, the user must specify tags used.
    Returns:
        marabouNetworkTF: (MarabouNetworkTF) representing network
    """
    return MarabouNetworkTF(filename, inputNames, outputName, modelType, savedModelTags)

def read_onnx(filename, inputNames=None, outputName=None):
    """
    Constructs a MarabouNetworkONNX object from an ONNX file

    Args:
        filename: (string) Path to the ONNX file
        inputNames: (list of strings) optional, list of node names corresponding to inputs.
        outputName: (string) optional, name of node corresponding to output.
    Returns:
        marabouNetworkONNX: (MarabouNetworkONNX) representing network
    """
    return MarabouNetworkONNX(filename, inputNames, outputName)

def load_query(filename):
    """
    Load the serialized inputQuery from the given filename
    Arguments:
        filename: (string) file to read for loading inputQuery
    Returns:
        MarabouCore.InputQuery object
    """
    return MarabouCore.loadQuery(filename)


def solve_query(ipq, filename="", verbose=True, options=None):
    """
    Function to solve query represented by this network
    Arguments:
        ipq: (MarabouCore.InputQuery) InputQuery object, which can be obtained from
                MarabouNetwork.getInputQuery or load_query
        filename: (string) path to redirect output to
        verbose: (bool) whether to print out solution after solve finishes
        options: (MarabouCore.Options) object for specifying Marabou options
    Returns:
        vals: (dict: int->float) empty if UNSAT, else SATisfying solution
        stats: (Statistics) a Statistics object as defined in Marabou,
                it has multiple methods that provide information related
                to how an input query was solved.
    """
    if options is None:
        options = createOptions()
    vals, stats = MarabouCore.solve(ipq, options, filename)
    if verbose:
        if stats.hasTimedOut():
            print ("TO")
        elif len(vals)==0:
            print("unsat")
        else:
            print("sat")
            for i in range(ipq.getNumInputVariables()):
                print("input {} = {}".format(i, vals[ipq.inputVariableByIndex(i)]))
            for i in range(ipq.getNumOutputVariables()):
                print("output {} = {}".format(i, vals[ipq.outputVariableByIndex(i)]))

    return [vals, stats]

def createOptions( numWorkers=4, initialTimeout=5, initialDivides=0, onlineDivides=2,
                   timeoutInSeconds=0, timeoutFactor=1.5, verbosity=2, dnc=False):
    """
    Create an option object
    """
    options = Options()
    options._numWorkers = numWorkers
    options._initialTimeout = initialTimeout
    options._initialDivides = initialDivides
    options._onlineDivides = onlineDivides
    options._timeoutInSeconds = timeoutInSeconds
    options._timeoutFactor = timeoutFactor
    options._verbosity = verbosity
    options._dnc = dnc
    return options
