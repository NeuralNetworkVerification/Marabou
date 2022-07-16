#! /usr/bin/env python3

import argparse
import numpy as np
import os
import sys
import tempfile
import pathlib
sys.path.insert(0, os.path.join(str(pathlib.Path(__file__).parent.absolute()), "../"))
from maraboupy.MarabouNetworkONNX import *
from maraboupy.VNNLibParser import readVNNLibFile
from maraboupy import MarabouCore
import subprocess

def main():
    args = arguments().parse_args()
    query, filename = createQuery(args)
    print("Writing query file to {}".format(filename))
    if query == None:
        print("Unable to create an input query!")
        print("There are three options to define the benchmark:\n"
              "1. Provide an input query file.\n"
              "2. Provide a network and a property file.\n"
              "3. Provide a network, a dataset (--dataset), an epsilon (-e), "
              "target label (-t), and the index of the point in the test set (-i).")
        exit(1)
    MarabouCore.saveQuery(query, filename)

def getInputQueryName(networkPath, propPath, benchmarkDir):
    netname = os.path.basename(networkPath)
    propname = os.path.basename(propPath)
    return os.path.join(benchmarkDir, netname + "-" + propname)

def createQuery(args):
    networkPath = args.network
    propPath = args.prop
    suffix = networkPath.split('.')[-1]
    assert(suffix == "onnx")
    network = MarabouNetworkONNX(networkPath)
    if args.debug:
        inputs = np.random.uniform(0,1, np.array(network.inputVars).shape)
        outputOnnx = network.evaluateWithoutMarabou(inputs)
        print("output onnx:", outputOnnx)
        outputMarabou = network.evaluateWithMarabou(inputs)
        print("output marabou:", outputMarabou)
        assert((abs(np.array(outputOnnx).flatten() - np.array(outputMarabou).flatten()) < 0.00001).all())
        print("Pass check!")
    if propPath != None:
        readVNNLibFile(propPath, network)

    print("Number of relus: {}".format(len(network.reluList)))
    print("Number of maxs: {}".format(len(network.maxList)))
    print("Number of disjunctions: {}".format(len(network.disjunctionList)))
    print("Number of equations: {}".format(len(network.equList)))
    print("Number of variables: {}".format(network.numVars))
    ipq = network.getMarabouQuery()

    return ipq, getInputQueryName(networkPath, propPath, args.benchmark_dir)

def arguments():
    ################################ Arguments parsing ##############################
    parser = argparse.ArgumentParser(description="Script to run some canonical benchmarks with Marabou (e.g., ACAS benchmarks, l-inf robustness checks on mnist/cifar10).")
    # benchmark
    parser.add_argument('network', type=str, nargs='?', default=None,
                        help='The network file name, the extension can be only .pb, .nnet, and .onnx')
    parser.add_argument('prop', type=str, nargs='?', default=None,
                        help='The property file name')
    parser.add_argument('benchmark_dir', type=str, nargs='?', default=None,
                        help='The directory to store benchmarks')
    parser.add_argument('--work-dir', type=str, default="./",
                        help='The working dir')
    parser.add_argument('--debug', action="store_true", default=False)

    return parser

if __name__ == "__main__":
    main()
