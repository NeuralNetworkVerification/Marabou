
import sys
import os
import tensorflow as tf
import numpy as np

(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

with open('mnist/x_train.npy', 'wb') as f:
            np.save(f, x_train)
with open('mnist/y_train.npy', 'wb') as f:
            np.save(f, y_train)
with open('mnist/x_test.npy', 'wb') as f:
            np.save(f, x_test)
with open('mnist/y_test.npy', 'wb') as f:
    np.save(f, y_test)


