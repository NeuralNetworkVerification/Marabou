#https://github.com/onnx/keras-onnx/blob/master/tutorial/TensorFlow_Keras_MNIST.ipynb

import sys
import os
import tensorflow as tf
import numpy as np
import keras2onnx
import onnx
import onnxruntime
from cnn_abs import *

#print("Still updated")
#exit()

model, replaceLayerName = genCnnForAbsTest()
onnx_model = keras2onnx.convert_keras(model, 'mnist-onnx', debug_mode=1)
keras2onnx.save_model(onnx_model, mnistProp.output_model_path(model))

