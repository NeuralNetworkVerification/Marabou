#Marabou File
from .MarabouNetworkNNet import *
from .MarabouNetworkTF import *

def read_nnet(filename):
    """
    Constructs a MarabouNetworkNnet object from a .nnet file

    Args:
        filename: (string) path to the .nnet file.
    Returns:
        marabouNetworkNNet: (MarabouNetworkNNet) representing network
    """
    return MarabouNetworkNNet(filename)


def read_tf(filename, inputName=None, outputName=None):
    """
    Constructs a MarabouNetworkTF object from a frozen Tensorflow protobuf

    Args:
        filename: (string) path to the .nnet file.
        inputName: (string) optional, name of operation corresponding to input
        outputName: (string) optional, name of operation corresponding to output
    Returns:
        marabouNetworkTF: (MarabouNetworkTF) representing network
    """
    return MarabouNetworkTF(filename, inputName, outputName)