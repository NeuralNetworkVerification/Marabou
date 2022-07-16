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

MARABOU_LOC = os.path.join(str(pathlib.Path(__file__).parent.absolute()), "../build/Marabou")

def main():
    args = arguments().parse_args()
    networkPath = args.network
    propPath = args.prop
    ipqName = getInputQueryName(networkPath, propPath, args.benchmark_dir)
    resultName = args.result
    #subprocess.run("{} --input-query {} --summary-file {}".format(MARABOU_LOC,
    #                                                              ipqName, args.result).split())
    options="--num-workers=8"
    try:
        r = subprocess.run("{} --input-query {} --summary-file {}"
                           " --verbosity=2 {}".format(MARABOU_LOC,
                                                      ipqName, resultName, options).split(),
                           timeout=args.timeout)
    except:
        f = open(resultName, 'w')
        f.write("unknown\n")
        f.close()

    os.remove(ipqName)

def getInputQueryName(networkPath, propPath, benchmarkDir):
    netname = os.path.basename(networkPath)
    propname = os.path.basename(propPath)
    return os.path.join(benchmarkDir, netname + "-" + propname)

def arguments():
    ################################ Arguments parsing ##############################
    parser = argparse.ArgumentParser(description="Script to run some canonical benchmarks with Marabou (e.g., ACAS benchmarks, l-inf robustness checks on mnist/cifar10).")
    # benchmark
    parser.add_argument('network', type=str, nargs='?', default=None,
                        help='The network file name, the extension can be only .pb, .nnet, and .onnx')
    parser.add_argument('prop', type=str, nargs='?', default=None,
                        help='The property file name')
    parser.add_argument('result', type=str, nargs='?', default=None,
                        help='The file to store the result')
    parser.add_argument('benchmark_dir', type=str, nargs='?', default=None,
                        help='The directory to store benchmarks')
    parser.add_argument('--debug', action="store_true", default=False)
    parser.add_argument('--timeout', type=float, default=0, required=True)

    return parser

if __name__ == "__main__":
    main()
