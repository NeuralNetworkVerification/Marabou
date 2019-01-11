#Marabou File
from .MarabouNetworkNNet import *
from .MarabouNetworkTF import *

def read_nnet(filename, sbt=False):
    """
    Constructs a MarabouNetworkNnet object from a .nnet file

    Args:
        filename: (string) path to the .nnet file.
    Returns:
        marabouNetworkNNet: (MarabouNetworkNNet) representing network
    """
    return MarabouNetworkNNet(filename, sbt=sbt)


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

def load_query(filename, verbose=True, timeout=0):
    MarabouNetwork.loadQuery(filename, verbose, timeout=0)
