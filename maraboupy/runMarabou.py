#! /usr/bin/env python3
'''
Top contributors (to current version):
    - Andrew Wu

This file is part of the Marabou project.
Copyright (c) 2017-2021 by the authors listed in the file AUTHORS
in the top-level source directory) and their institutional affiliations.
All rights reserved. See the file COPYING in the top-level source
directory for licensing information.
'''

import argparse
import numpy as np
import os
import sys

import pathlib
sys.path.insert(0, os.path.join(str(pathlib.Path(__file__).parent.absolute()), "../"))
from maraboupy import Marabou
from maraboupy import MarabouCore
from maraboupy import MarabouUtils

def main():
        args = arguments().parse_args()
        print(args)
        query, network = createQuery(args)
        if (args.query_dump_file != ""):
                MarabouCore.saveQuery(query, args.query_dump_file)
                exit()
        if query == None:
            print("Unable to create an input query!")
            print("There are three options to define the benchmark:\n"
                  "1. Provide an input query file.\n"
                  "2. Provide a network and a property file.\n"
                  "3. Provide a network, a dataset (--dataset), an epsilon (-e), "
                  "target label (-t), and the index of the point in the test set (-i).")
            exit(1)
        options = createOptions(args)
        vals, stats = MarabouCore.solve(query, options)
        if stats.hasTimedOut():
            print ("TIMEOUT")
        elif len(vals)==0:
            print("unsat")
        else:
            for i in range(query.getNumInputVariables()):
                print("x{} = {}".format(i, vals[query.inputVariableByIndex(i)]))
            if network:
                outputs = network.evaluateWithoutMarabou(np.array([[vals[query.inputVariableByIndex(i)] for i in range(query.getNumInputVariables())]]))
                for i, output in enumerate(list(outputs.flatten())):
                    print("y{} = {}".format(i, output))
            else:
                for i in range(query.getNumOutputVariables()):
                    print("y{} = {}".format(i, vals[query.outputVariableByIndex(i)]))

            print("sat")

def createQuery(args):
    if args.input_query:
        query = Marabou.load_query(args.input_query)
        return query, None
    networkPath = args.network

    suffix = networkPath.split('.')[-1]
    if suffix == "nnet":
        network = Marabou.read_nnet(networkPath)
    elif suffix == "pb":
        network = Marabou.read_tf(networkPath)
    elif suffix == "onnx":
        network = Marabou.read_onnx(networkPath)
    else:
        print("The network must be in .pb, .nnet, or .onnx format!")
        return None, None

    if  args.prop != None:
        query = network.getMarabouQuery()
        if MarabouCore.loadProperty(query, args.prop):
            return query, network

    if args.dataset == 'mnist':
        encode_mnist_linf(network, args.index, args.epsilon, args.target_label)
        return network.getMarabouQuery(), network
    elif args.dataset == 'cifar10':
        encode_cifar10_linf(network, args.index, args.epsilon, args.target_label)
        return network.getMarabouQuery(), network
    else:
        print("No property encoded! The dataset must be taxi or mnist or cifar10.")
        return network.getMarabouQuery(), network

def encode_mnist_linf(network, index, epsilon, target_label):
    from tensorflow.keras.datasets import mnist
    (X_train, Y_train), (X_test, Y_test) = mnist.load_data()
    point = np.array(X_test[index]).flatten() / 255
    print("correct label: {}".format(Y_test[index]))
    for x in np.array(network.inputVars).flatten():
        network.setLowerBound(x, max(0, point[x] - epsilon))
        network.setUpperBound(x, min(1, point[x] + epsilon))
    outputVars = network.outputVars.flatten()
    for i in range(10):
        if i != target_label:
            network.addInequality([outputVars[i],
                                   outputVars[target_label]],
                                  [1, -1], 0)
    return

def encode_cifar10_linf(network, index, epsilon, target_label):
    import torchvision.datasets as datasets
    import torchvision.transforms as transforms
    cifar_test = datasets.CIFAR10('./data/cifardata/', train=False, download=True, transform=transforms.ToTensor())
    X,y = cifar_test[index]
    point = X.unsqueeze(0).numpy().flatten()
    lb = np.zeros(3072)
    ub = np.zeros(3072)
    for i in range(1024):
        lb[i] = (max(0, point[i] - epsilon) - 0.485) / 0.225
        ub[i] = (min(1, point[i] + epsilon) - 0.485) / 0.225
    for i in range(1024):
        lb[1024 + i] = (max(0, point[1024 + i] - epsilon) - 0.456) / 0.225
        ub[1024 + i] = (min(1, point[1024 + i] + epsilon) - 0.456) / 0.225
    for i in range(1024):
        lb[2048 + i] = (max(0, point[2048 + i] - epsilon) - 0.406) / 0.225
        ub[2048 + i] = (min(1, point[2048 + i] + epsilon) - 0.406) / 0.225
    print("correct label: {}, target label: {}".format(y, target_label))
    for i in range(3072):
        network.setLowerBound(i, lb[i])
        network.setUpperBound(i, ub[i])
    for i in range(10):
        if i != target_label:
            network.addInequality([network.outputVars[0][i],
                                   network.outputVars[0][target_label]],
                                  [1, -1], 0)
    return

def createOptions(args):
    options = MarabouCore.Options()
    options._numWorkers = args.num_workers
    options._initialTimeout = args.initial_timeout
    options._initialDivides = args.initial_divides
    options._onlineDivides = args.num_online_divides
    options._timeoutInSeconds = args.timeout
    options._timeoutFactor = args.timeout_factor
    options._verbosity = args.verbosity
    options._snc = args.snc
    options._splittingStrategy = args.branch
    options._sncSplittingStrategy = args.split_strategy
    options._splitThreshold = args.split_threshold
    options._solveWithMILP = args.milp
    options._preprocessorBoundTolerance = args.preprocessor_bound_tolerance
    options._dumpBounds = args.dump_bounds
    options._tighteningStrategy = args.tightening_strategy
    options._milpTighteningStrategy = args.milp_tightening
    options._milpTimeout = args.milp_timeout

    return options

def arguments():
    ################################ Arguments parsing ##############################
    parser = argparse.ArgumentParser(description="Script to run some canonical benchmarks with Marabou (e.g., ACAS benchmarks, l-inf robustness checks on mnist/cifar10).")
    # benchmark
    parser.add_argument('network', type=str, nargs='?', default=None,
                        help='The network file name, the extension can be only .pb, .nnet, and .onnx')
    parser.add_argument('prop', type=str, nargs='?', default=None,
                        help='The property file name')
    parser.add_argument('-q', '--input-query', type=str, default=None,
                        help='The input query file name')
    parser.add_argument('--dataset', type=str, default=None,
                        help="the dataset (mnist,cifar10)")
    parser.add_argument('-e', '--epsilon', type=float, default=0,
                        help='The epsilon for L_infinity perturbation')
    parser.add_argument('-t', '--target-label', type=int, default=0,
                        help='The target of the adversarial attack')
    parser.add_argument('-i,', '--index', type=int, default=0,
                        help='The index of the point in the test set')

    parser.add_argument('--query-dump-file', type=str, default="",
                        help='Dump the query to a file')

    options = MarabouCore.Options()
    # runtime options
    parser.add_argument('--snc', action="store_true",
                        help='Use the split-and-conquer solving mode.')
    parser.add_argument("--dump-bounds", action="store_true",
                        help="Dump the bounds after preprocessing" )
    parser.add_argument( "--num-workers", type=int, default=options._numWorkers,
                         help="(SnC) Number of workers" )
    parser.add_argument( "--split-strategy", type=str, default=options._sncSplittingStrategy,
                         help="(SnC) The splitting strategy" )
    parser.add_argument( "--tightening-strategy", type=str, default=options._tighteningStrategy,
                         help="type of bound tightening technique to use: sbt/deeppoly/none. default: deeppoly" )
    parser.add_argument( "--initial-divides", type=int, default=options._initialDivides,
                         help="(SnC) Number of times to initially bisect the input region" )
    parser.add_argument( "--initial-timeout", type=int, default=options._initialTimeout,
                         help="(SnC) The initial timeout" )
    parser.add_argument( "--num-online-divides", type=int, default=options._onlineDivides,
                         help="(SnC) Number of times to further bisect a sub-region when a timeout occurs" )
    parser.add_argument( "--timeout", type=int, default=options._timeoutInSeconds,
                         help="Global timeout" )
    parser.add_argument( "--verbosity", type=int, default=options._verbosity,
                         help="Verbosity of engine::solve(). 0: does not print anything (for SnC), 1: print"
                         "out statistics in the beginning and end, 2: print out statistics during solving." )
    parser.add_argument( "--split-threshold", type=int, default=options._splitThreshold,
                         help="Max number of tries to repair a relu before splitting" )
    parser.add_argument( "--timeout-factor", type=float, default=options._timeoutFactor,
                         help="(SnC) The timeout factor" )
    parser.add_argument( "--preprocessor-bound-tolerance", type=float, default=options._preprocessorBoundTolerance,
                          help="epsilon for preprocessor bound tightening comparisons" )
    parser.add_argument( "--milp", action="store_true", default=options._solveWithMILP,
                         help="Use a MILP solver to solve the input query" )
    parser.add_argument( "--milp-tightening", type=str, default=options._milpTighteningStrategy,
                         help="The MILP solver bound tightening type:"
                         "lp/lp-inc/milp/milp-inc/iter-prop/none. default: lp" )
    parser.add_argument( "--milp-timeout", type=float, default=options._milpTimeout,
                         help="Per-ReLU timeout for iterative propagation" )
    parser.add_argument( "--branch", type=str, default=options._splittingStrategy,
                         help="The branching strategy" )
    return parser

if __name__ == "__main__":
    main()
