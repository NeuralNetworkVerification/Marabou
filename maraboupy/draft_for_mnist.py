import tensorflow as tf
import pandas as pd
import numpy as np
import gzip
import sys
import pickle



def change_input_dimension(X):
    return X.reshape((X.shape[0],X.shape[1]*X.shape[2]))


def check_mnist_digit_equivalence():
    # download mnist from the internet
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    x_test = change_input_dimension(x_test)

    # check mnist_test csv
    mnist_digit_df = pd.read_csv("/cs/labs/guykatz/guyam/digit_test_set.csv") # todo - make with general path

    for i in range(10000):
        label_and_data = np.array(mnist_digit_df)[i]

        label = label_and_data[0:1:][0] # todo - this is before normalization
        sample = label_and_data[1::]

        assert (False not in (x_test[i] == sample))
        assert (y_test[i] == label)
    print("test passed - for MNIST DIGITS")


def check_mnist_fashion_equivalence():
    # download mnist fashion from the internet
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.fashion_mnist.load_data()
    x_test = change_input_dimension(x_test)

    # check mnist_test csv
    mnist_fashion_df = pd.read_csv("/cs/labs/guykatz/guyam/fashion_test_set.csv")  # todo - make with general path

    for i in range(10000):
        sample = np.array(mnist_fashion_df)[i] # todo - this is before normalization
        assert (False not in (x_test[i] == sample))

    print("test passed - for MNIST FASHION")



if __name__ == '__main__':
    check_mnist_digit_equivalence()
    check_mnist_fashion_equivalence()



