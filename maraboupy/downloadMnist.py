
import sys
import os
import tensorflow as tf
import numpy as np

for (x, y), name in zip(tf.keras.datasets.mnist.load_data(), ['train', 'test']):
    os.makedirs('mnist/{}'.format(name), exist_ok=True)
    for (i, sample),label in zip(enumerate(x), y):
        with open('mnist/{}/{}-{}.npy'.format(name, i, label), 'wb') as f:
            np.save(f, sample)



