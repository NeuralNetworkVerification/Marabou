#!/usr/bin/env python3

# This script generates targeted attack properties for networks trained on the
# MNIST dataset. To generate a query that checks if the first image in the
# training data can be perturbed to be 8 with 0.04 perturbation bound, run:
#  - python dump_mnist_targeted_attack.py -i 1 -e 0.04 -t 8

import warnings
warnings.filterwarnings("ignore")
from keras.datasets import mnist
import numpy as np
import argparse

def dumpMNISTTargetedAttackProperty(index, epsilon, target):
    (x_train, y_train), (X_test, y_test) = mnist.load_data()
    X = np.array(x_train[index]).flatten() / 255
    orig = y_train[index]
    if orig == target:
        print("Failed! Original label same as targeted adversarial label!")
        exit()

    # target: 0..9 corresponding to y0..y9
    for i, x in enumerate(X):
        print('x{} >= {}'.format(i, x - epsilon))
        print('x{} <= {}'.format(i, x + epsilon))
    for i in range(10):
        if i != target:
            print('+y{} -y{} <= 0'.format(i, target))

def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument('-i', '--index', required=True, type=int, help = 'The index of image in the training set')
    p.add_argument('-e', '--epsilon', required=True, type=float, help = 'The perturbation')
    p.add_argument('-t', '--target', required=True, type=int, help = 'The target label')
    opts = p.parse_args()
    return opts

def main():
    opts = parse_args()
    index = opts.index
    epsilon = opts.epsilon
    target = opts.target
    dumpMNISTTargetedAttackProperty(index, epsilon, target)

if __name__ == "__main__":
    main()
