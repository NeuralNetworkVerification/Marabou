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
from .MarabouNetworkTFWeightsAsVar import *

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

def read_tf_weights_as_var(filename, inputVals, inputName=None, outputName=None, savedModel=False, savedModelTags=[]):
    """
    Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf and an input. The network weights are the variables 

    Args:
        filename: (string) If savedModel is false, path to the frozen graph .pb file.
                           If savedModel is true, path to SavedModel folder, which
                           contains .pb file and variables subdirectory.
        inputVals: (array) The network input.
        inputName: (string) optional, name of operation corresponding to input.
        outputName: (string) optional, name of operation corresponding to output.
        savedModel: (bool) If false, load frozen graph. If true, load SavedModel object.
        savedModelTags: (list of strings) If loading a SavedModel, the user must specify tags used.
    Returns:
        marabouNetworkTF: (MarabouNetworkTF) representing network
    """
    return MarabouNetworkTFWeightsAsVar(filename, inputVals, inputName, outputName, savedModel, savedModelTags)


def load_query(filename, verbose=True, timeout=0):
    MarabouNetwork.loadQuery(filename, verbose, timeout=0)
