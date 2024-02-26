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
import tempfile

import pathlib
sys.path.insert(0, os.path.join(str(pathlib.Path(__file__).parent.absolute()), "../"))
from maraboupy import Marabou
from maraboupy import MarabouCore
from maraboupy import MarabouUtils

import subprocess

def main():
        args, unknown = arguments().parse_known_args()
        query, network = createQuery(args)
        if query == None:
            print("Unable to create an input query!")
            print("There are three options to define the benchmark:\n"
                  "1. Provide an input query file.\n"
                  "2. Provide a network and a property file.\n"
                  "3. Provide a network, a dataset (--dataset), an epsilon (-e), "
                  "target label (-t), and the index of the point in the test set (-i).")
            exit(1)

        marabou_binary = args.marabou_binary
        if not os.access(marabou_binary, os.X_OK):
            sys.exit('"{}" does not exist or is not executable'.format(marabou_binary))

        temp = tempfile.NamedTemporaryFile(dir=args.temp_dir, delete=False)
        name = temp.name
        MarabouCore.saveQuery(query, name)

        print("Running Marabou with the following arguments: ", unknown)
        subprocess.run([marabou_binary] + ["--input-query={}".format(name)] + unknown )
        os.remove(name)

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
        query = network.getInputQuery()
        MarabouCore.loadProperty(query, args.prop)
        return query, network

    if args.dataset == 'mnist':
        encode_mnist_linf(network, args.index, args.epsilon, args.target_label)
        return network.getInputQuery(), network
    elif args.dataset == 'cifar10':
        encode_cifar10_linf(network, args.index, args.epsilon, args.target_label)
        return network.getInputQuery(), network
    else:
        """
        ENCODE YOUR CUSTOMIZED PROPERTY HERE!
        """
        print("No property encoded!")

        return network.getInputQuery(), network

def encode_mnist_linf(network, index, epsilon, target_label):
    from tensorflow.keras.datasets import mnist
    (X_train, Y_train), (X_test, Y_test) = mnist.load_data()
    point = np.array(X_test[index]).flatten() / 255
    print("correct label: {}".format(Y_test[index]))
    for x in np.array(network.inputVars).flatten():
        network.setLowerBound(x, max(0, point[x] - epsilon))
        network.setUpperBound(x, min(1, point[x] + epsilon))
    if target_label == -1:
        print("No output constraint!")
    else:
        outputVars = network.outputVars[0].flatten()
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
        lb[i] = max(0, point[i] - epsilon)
        ub[i] = min(1, point[i] + epsilon)
    for i in range(1024):
        lb[1024 + i] = max(0, point[1024 + i] - epsilon)
        ub[1024 + i] = min(1, point[1024 + i] + epsilon)
    for i in range(1024):
        lb[2048 + i] = max(0, point[2048 + i] - epsilon)
        ub[2048 + i] = min(1, point[2048 + i] + epsilon)
    print("correct label: {}".format(y))
    if target_label == -1:
        print("No output constraint!")
    else:
        for i in range(3072):
            network.setLowerBound(i, lb[i])
            network.setUpperBound(i, ub[i])
        for i in range(10):
            if i != target_label:
                network.addInequality([network.outputVars[0][0][i],
                                       network.outputVars[0][0][target_label]],
                                      [1, -1], 0)
    return

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
    parser.add_argument('-t', '--target-label', type=int, default=-1,
                        help='The target of the adversarial attack')
    parser.add_argument('-i,', '--index', type=int, default=0,
                        help='The index of the point in the test set')
    parser.add_argument('--temp-dir', type=str, default="/tmp/",
                        help='Temporary directory')
    marabou_path = os.path.join(str(pathlib.Path(__file__).parent.absolute()),
                                "../build/Marabou" )
    parser.add_argument('--marabou-binary', type=str, default=marabou_path,
                        help='The path to Marabou binary')

    return parser

if __name__ == "__main__":
    main()
