'''
Top contributors (to current version):
    - Christopher Lazarus
    - Kyle Julian
    - Andrew Wu
    
This file is part of the Marabou project.
Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.

Marabou defines key functions that make up the main user interface to Maraboupy
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
    """Constructs a MarabouNetworkNnet object from a .nnet file

    Args:
        filename (str): Path to the .nnet file
        normalize (bool, optional): If true, incorporate input/output normalization
                  into first and last layers of network

    Returns:
        :class:`~maraboupy.MarabouNetworkNNet.MarabouNetworkNNet`
    """
    return MarabouNetworkNNet(filename, normalize=normalize)


def read_tf(filename, inputNames=None, outputName=None, modelType="frozen", savedModelTags=[]):
    """Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf

    Args:
        filename (str): Path to tensorflow network
        inputNames (list of str, optional): List of operation names corresponding to inputs
        outputName (str, optional): Name of operation corresponding to output
        modelType (str, optional): Type of model to read. The default is "frozen" for a frozen graph.
                            Can also use "savedModel_v1" or "savedModel_v2" for the SavedModel format
                            created from either tensorflow versions 1.X or 2.X respectively.
        savedModelTags (list of str, optional): If loading a SavedModel, the user must specify tags used, default is []

    Returns:
        :class:`~maraboupy.MarabouNetworkTF.MarabouNetworkTF`
    """
    return MarabouNetworkTF(filename, inputNames, outputName, modelType, savedModelTags)

def read_onnx(filename, inputNames=None, outputName=None):
    """Constructs a MarabouNetworkONNX object from an ONNX file

    Args:
        filename (str): Path to the ONNX file
        inputNames (list of str, optional): List of node names corresponding to inputs
        outputName (str, optional): Name of node corresponding to output

    Returns:
        :class:`~maraboupy.MarabouNetworkONNX.MarabouNetworkONNX`
    """
    return MarabouNetworkONNX(filename, inputNames, outputName)

def load_query(filename):
    """Load the serialized inputQuery from the given filename

    Args:
        filename (str): File to read for loading input query

    Returns:
        :class:`~maraboupy.MarabouCore.InputQuery`
    """
    return MarabouCore.loadQuery(filename)

def solve_query(ipq, filename="", verbose=True, options=None):
    """Function to solve query represented by this network

    Args:
        ipq (:class:`~maraboupy.MarabouCore.InputQuery`): InputQuery object, which can be obtained from
                   :func:`~maraboupy.MarabouNetwork.getInputQuery` or :func:`~maraboupy.Marabou.load_query`
        filename (str, optional): Path to redirect output to, defaults to ""
        verbose (bool, optional): Whether to print out solution after solve finishes, defaults to True
        options: (:class:`~maraboupy.MarabouCore.Options`): Object for specifying Marabou options

    Returns:
        (tuple): tuple containing:
            - vals (Dict[int, float]): Empty dictionary if UNSAT, otherwise a dictionary of SATisfying values for variables
            - stats (:class:`~maraboupy.MarabouCore.Statistics`, optional): A Statistics object to how Marabou performed
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

def createOptions(numWorkers=1, initialTimeout=5, initialDivides=0, onlineDivides=2,
                  timeoutInSeconds=0, timeoutFactor=1.5, verbosity=2, snc=False,
                  splittingStrategy="auto", sncSplittingStrategy="auto",
                  restoreTreeStates=False, splitThreshold=20, solveWithMILP=False,
                  preprocessorBoundTolerance=0.0000000001, dumpBounds=False,
                  tighteningStrategy="deeppoly" ):
    """Create an options object for how Marabou should solve the query

    Args:
        numWorkers (int, optional): Number of workers to use in DNC mode, defaults to 4
        initialTimeout (int, optional): Initial timeout in seconds for DNC mode before dividing, defaults to 5
        initialDivides (int, optional): Number of time sto perform the initial partitioning.
            This creates 2^(initialDivides) sub-problems for DNC mode, defaults to 0
        onlineDivides (int, optional): Number of times to perform the online partitioning when a sub-query
            time out. This creates 2^(onlineDivides) sub-problems for DNC mode, defaults to 2
        timeoutInSeconds (int, optional): Timeout duration for Marabouin seconds, defaults to 0
        timeoutFactor (float, optional): Timeout factor for DNC mode, defaults to 1.5
        verbosity (int, optional): Verbosity level for Marabou, defaults to 2
        snc (bool, optional): If SnC mode should be used, defaults to False
        splittingStrategy (string, optional): Specifies which partitioning strategy to use (auto/largest-interval/relu-violation/polarity/earliest-relu)
        sncSplittingStrategy (string, optional): Specifies which partitioning strategy to use in the DNC mode (auto/largest-interval/polarity).
        restoreTreeStates (bool, optional): Whether to restore tree states in dnc mode, defaults to False
        solveWithMILP ( bool, optional): Whther to solve the input query with a MILP encoding. Currently only works when Gurobi is installed. Defaults to False.
        preprocessorBoundTolerance ( float, optional): epsilon value for preprocess bound tightening . Defaults to 10^-10.
        dumpBounds (bool, optional): Print out the bounds of each neuron after preprocessing. defaults to False
        tighteningStrategy (string, optional): The abstract-interpretation-based bound tightening techniques used during the search (deeppoly/sbt/none). default to deeppoly.
    Returns:
        :class:`~maraboupy.MarabouCore.Options`
    """
    options = Options()
    options._numWorkers = numWorkers
    options._initialTimeout = initialTimeout
    options._initialDivides = initialDivides
    options._onlineDivides = onlineDivides
    options._timeoutInSeconds = timeoutInSeconds
    options._timeoutFactor = timeoutFactor
    options._verbosity = verbosity
    options._snc = snc
    options._splittingStrategy = splittingStrategy
    options._sncSplittingStrategy = sncSplittingStrategy
    options._restoreTreeStates = restoreTreeStates
    options._splitThreshold = splitThreshold
    options._solveWithMILP = solveWithMILP
    options._preprocessorBoundTolerance = preprocessorBoundTolerance
    options._dumpBounds = dumpBounds
    options._tighteningStrategy = tighteningStrategy
    return options
