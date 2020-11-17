#https://github.com/onnx/keras-onnx/blob/master/tutorial/TensorFlow_Keras_MNIST.ipynb

import sys
import os
import tensorflow as tf
print("TensorFlow version is "+tf.__version__)

import numpy as np
import keras2onnx
import onnx
import onnxruntime
from cnn_abs import *
#from cnn_abs import genCnnForAbsTest, mnistProp

#print("Still updated")
#exit()

#imgSize0=len(mnistProp.x_train[0])
#imgSize1=len(mnistProp.x_train[0][0])

model, replaceLayerName = genCnnForAbsTest()

'''
image_index = int(np.random.randint(0, mnistProp.x_test.shape[0], size=1)[0])
expected_label = mnistProp.y_test[image_index]
digit_image = np.reshape(mnistProp.x_test[image_index], (28,28))


digit_image = digit_image.reshape(1, imgSize0, imgSize1, 1)
loop_count = 10


for i in range(loop_count):
    prediction = model.predict(digit_image)


print(prediction)
predicted_label = prediction.argmax()
print('Predicted value:', predicted_label)
if (expected_label == predicted_label):
  print('Correct prediction !')
else:
  print('Wrong prediction !')
'''

onnx_model = keras2onnx.convert_keras(model, 'mnist-onnx', debug_mode=1)
keras2onnx.save_model(onnx_model, mnistProp.output_model_path(model))

'''
sess_options = onnxruntime.SessionOptions()
sess = onnxruntime.InferenceSession(output_model_path, sess_options)
data = [digit_image.astype(np.float32)]
input_names = sess.get_inputs()
feed = dict([(input.name, data[n]) for n, input in enumerate(sess.get_inputs())])

for i in range(loop_count):
    onnx_predicted_label = sess.run(None, feed)[0].argmax()

print('ONNX predicted value:', onnx_predicted_label)
if (expected_label == onnx_predicted_label):
  print('Correct prediction !')
else:
  print('Wrong prediction !')

if (predicted_label == onnx_predicted_label):
  print("The ONNX's and keras' prediction are matching !")
else:
  print("The ONNX's and keras' prediction does not match !")


print("featureShape={}".format(mnistProp.featureShape))
'''
