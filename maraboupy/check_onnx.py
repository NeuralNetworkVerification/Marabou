#https://github.com/onnx/keras-onnx/blob/master/tutorial/TensorFlow_Keras_MNIST.ipynb

import sys
import os
import tensorflow as tf
print("TensorFlow version is "+tf.__version__)

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import time

mnist = tf.keras.datasets.mnist
# Load its data into training and test vectors
(x_train, y_train),(x_test, y_test) = mnist.load_data()
# Normalize the data
x_train, x_test = x_train / 255.0, x_test / 255.0
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)
# Collect feature size info
imgSize0=len(x_train[0])
imgSize1=len(x_train[0][0])
numPixels=imgSize0*imgSize1
numTrainImages=len(x_train)
featureShape=(1,imgSize0,imgSize1)

print("Training dataset has "+str(numTrainImages))
print("Testing dataset has "+str(len(x_test)))
print("Feature shape is "+str(featureShape))
train_labels_count = np.unique(y_train, return_counts=True)
dataframe_train_labels = pd.DataFrame({'Label':train_labels_count[0], 'Count':train_labels_count[1]})
dataframe_train_labels
test_labels_count = np.unique(y_test, return_counts=True)
dataframe_test_labels = pd.DataFrame({'Label':test_labels_count[0], 'Count':test_labels_count[1]})
dataframe_test_labels

# Clearup everything before running
tf.keras.backend.clear_session()

print ('creating a new model')
# Clearup everything before running
tf.keras.backend.clear_session()
# Create model
#model = tf.keras.models.Sequential()
## Add layers
## The first layer MUST have input_shape. See the observation above.
#model.add(tf.keras.layers.Flatten(input_shape=(28, 28, 1)))
#model.add(tf.keras.layers.Dense(8, activation='relu'))
#model.add(tf.keras.layers.Dense(10, activation='softmax'))

from cnn_abs import genCnnForAbsTest

model, replaceLayerName = genCnnForAbsTest()

# Build model and print summary

#model.build(input_shape=featureShape)
#model.summary()
#model.compile(optimizer='adam', loss='sparse_categorical_crossentropy', metrics=['accuracy'])
#history = model.fit(x_train, y_train, epochs=1)

# Train
##print("Training Model")
# increase the number of epochs if you wish better accuracy
####plt.plot(history.history["loss"], color='r')
####plt.title("Loss")
####plt.xlabel("Epoch")
####plt.show()
####plt.plot(history.history["accuracy"], color='b')
####plt.title("Accuracy")
####plt.xlabel("Epoch")
####plt.show()


keras_test_loss, keras_test_acc = model.evaluate(x_test, y_test)

print('Test loss:', keras_test_loss)
print('Test accuracy:', keras_test_acc)

# if the model is just created, them I want to update saved model
model.save('./mnist-model.h5')

##### get a random image index from the test set
image_index = int(np.random.randint(0, x_test.shape[0], size=1)[0])
expected_label = y_test[image_index]
digit_image = np.reshape(x_test[image_index], (28,28))
##### and plot it
####plt.title('Example %d. Label: %d' % (image_index, expected_label))
####plt.imshow(digit_image, cmap='Greys')
####plt.show()


# uncomment the following two lines to save sample images with digits
#pil_img = tf.keras.preprocessing.image.array_to_img(digit_image.reshape((imgSize0,imgSize1,1)))
#pil_img.save('./str(expected_label)+'.bmp')

# reshape the image for inference/prediction
digit_image = digit_image.reshape(1, imgSize0, imgSize1, 1)
# repeat few times to take the average execution time
loop_count = 10

start_time = time.time()
for i in range(loop_count):
    prediction = model.predict(digit_image)
print("Keras inferences with %s second in average" %((time.time() - start_time) / loop_count))

print(prediction)
predicted_label = prediction.argmax()
print('Predicted value:', predicted_label)
if (expected_label == predicted_label):
  print('Correct prediction !')
else:
  print('Wrong prediction !')

# one suggestion is to place this and the previous cell inside a for loop to perform several inferences

import keras2onnx
print("keras2onnx version is "+keras2onnx.__version__)
# convert to onnx model
onnx_model = keras2onnx.convert_keras(model, 'mnist-onnx', debug_mode=1)
output_model_path = "./mnist-model.onnx"
# and save the model in ONNX format
keras2onnx.save_model(onnx_model, output_model_path)

import onnxruntime

sess_options = onnxruntime.SessionOptions()
sess = onnxruntime.InferenceSession(output_model_path, sess_options)
data = [digit_image.astype(np.float32)]
input_names = sess.get_inputs()
feed = dict([(input.name, data[n]) for n, input in enumerate(sess.get_inputs())])

start_time = time.time()
for i in range(loop_count):
    onnx_predicted_label = sess.run(None, feed)[0].argmax()
print("ONNX inferences with %s second in average" %((time.time() - start_time) / loop_count))

print('ONNX predicted value:', onnx_predicted_label)
if (expected_label == onnx_predicted_label):
  print('Correct prediction !')
else:
  print('Wrong prediction !')

if (predicted_label == onnx_predicted_label):
  print("The ONNX's and keras' prediction are matching !")
else:
  print("The ONNX's and keras' prediction does not match !")


print("featureShape={}".format(featureShape))
