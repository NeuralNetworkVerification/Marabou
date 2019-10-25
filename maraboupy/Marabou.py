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

#Marabou File
from .MarabouNetworkNNet import *
from .MarabouNetworkTF import *
from .MarabouCore import *

def read_nnet(filename, sbt=False):
    """
    Constructs a MarabouNetworkNnet object from a .nnet file

    Args:
        filename: (string) path to the .nnet file.
    Returns:
        marabouNetworkNNet: (MarabouNetworkNNet) representing network
    """
    return MarabouNetworkNNet(filename, perform_sbt=sbt)


def read_tf(filename, inputName=None, outputName=None, savedModel=False, savedModelTags=[]):
    """
    Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf

    Args:
        filename: (string) If savedModel is false, path to the frozen graph .pb file.
                           If savedModel is true, path to SavedModel folder, which
                           contains .pb file and variables subdirectory.
        inputName: (string) optional, name of operation corresponding to input.
        outputName: (string) optional, name of operation corresponding to output.
        savedModel: (bool) If false, load frozen graph. If true, load SavedModel object.
        savedModelTags: (list of strings) If loading a SavedModel, the user must specify tags used.
    Returns:
        marabouNetworkTF: (MarabouNetworkTF) representing network
    """
    return MarabouNetworkTF(filename, inputName, outputName, savedModel, savedModelTags)


def load_query(filename):
    """
    Load the serialized inputQuery from the given filename
    Arguments:
        filename: (string) file to read for loading inputQuery
    Returns:
        MarabouCore.InputQuery object
    """
    return MarabouCore.loadQuery(filename)


def solve_query(ipq, filename="", verbose=True, timeout=0, verbosity=2):
    """
    Function to solve query represented by this network
    Arguments:
        ipq: (MarabouCore.InputQuery) InputQuery object, which can be obtained from 
                MarabouNetwork.getInputQuery or load_query
        filename: (string) path to redirect output to
        timeout: (int) time in seconds when Marabou will time out
        verbose: (bool) whether to print out solution after solve finishes
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
    vals, stats = MarabouCore.solve(ipq, filename, timeout, verbosity)
    if verbose:
        if stats.hasTimedOut():
            print ("TIMEOUT")
        elif len(vals)==0:
            print("UNSAT")
        else:
            print("SAT")
            for i in range(ipq.getNumInputVariables()):
                print("input {} = {}".format(i, vals[ipq.inputVariableByIndex(i)]))
            for i in range(ipq.getNumOutputVariables()):
                print("output {} = {}".format(i, vals[ipq.outputVariableByIndex(i)]))

    return [vals, stats]

