
import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#pip install keras2onnx
#pip install onnx
#pip install onnxruntime
import keras2onnx
import onnx
import onnxruntime
import tensorflow as tf
from maraboupy import MarabouCore, Marabou
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import numpy as np
from cnn_abs import *

from tensorflow.keras.models import load_model
cfg_freshModelAbs = True
savedModelAbs = "cnn_abs_abs.h5"

cfg_dis_w = False
cfg_dis_b = True

#Log:
# Passing - True, True
# Failing - True, False

#manual_result = lambda w,b,x : w * x + b

#replaceW, replaceB = maskAndDensifyNDimConv(np.ones((2,2,1,1)), np.array([0.5]), np.ones((3,3,1)), (3,3,1), (3,3,1), (1,1))
#print(replaceW)
#print(replaceB)
#print("Vis = {}".format(replaceW[:,0]))
#exit()

#################################################
#### _____              _____  _   _  _   _  ####
####|  __ \            /  __ \| \ | || \ | | ####
####| |  \/ ___ _ __   | /  \/|  \| ||  \| | ####
####| | __ / _ \ '_ \  | |    | . ` || . ` | ####
####| |_\ \  __/ | | | | \__/\| |\  || |\  | ####
#### \____/\___|_| |_|  \____/\_| \_/\_| \_/ ####
####                                         ####                                   
#################################################


print("Starting model building")
#https://keras.io/examples/vision/mnist_convnet/
# Model / data parameters
num_classes = 10
## input_shape = (28 * 28,1) FIXME
input_shape = (28,28,1)
# the data, split between train and test sets
(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
# Scale images to the [0, 1] range
x_train = x_train.astype("float32") / 255
x_test = x_test.astype("float32") / 255
# Make sure images have shape (28, 28, 1)
#x_train = x_train.reshape(x_train.shape[0], 28 * 28) FIXME 
#x_test = x_test.reshape(x_test.shape[0], 28 * 28) FIXME 
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)
print("x_train shape:", x_train.shape)
print(x_train.shape[0], "train samples")
print(x_test.shape[0], "test samples")
# convert class vectors to binary class matrices
#y_train = tf.keras.utils.to_categorical(y_train, num_classes)
#y_test = tf.keras.utils.to_categorical(y_test, num_classes)
loss='sparse_categorical_crossentropy'
optimizer='adam'
metrics=['accuracy']
    
modelOrig, replaceLayerName = genCnnForAbsTest()
#modelOrig.fit(x_train, y_train, epochs=1)
#################################################################################
#### _____ _                              ______           _                 ####
####/  __ \ |                     ___     | ___ \         | |                ####
####| /  \/ | ___  _ __   ___    ( _ )    | |_/ /___ _ __ | | __ _  ___ ___  ####
####| |   | |/ _ \| '_ \ / _ \   / _ \/\  |    // _ \ '_ \| |/ _` |/ __/ _ \ ####
####| \__/\ | (_) | | | |  __/  | (_>  <  | |\ \  __/ |_) | | (_| | (_|  __/ ####
#### \____/_|\___/|_| |_|\___|   \___/\/  \_| \_\___| .__/|_|\__,_|\___\___| ####
####                                                | |                      ####
####                                                |_|                      ####
#################################################################################

if cfg_freshModelAbs:

    modelAbs = cloneAndMaskConvModel(modelOrig, replaceLayerName, np.ones(modelOrig.get_layer(name=replaceLayerName).output_shape[1:-1]))

    score = modelAbs.evaluate(x_test, y_test, verbose=0)
    print("Test loss:", score[0])
    print("Test accuracy:", score[1])

    if np.all(np.isclose(modelAbs.predict(x_test), modelOrig.predict(x_test))):
        print("Prediction aligned")    
    else:
        print("Prediction not aligned")
        
    modelAbs.save(savedModelAbs)  # creates a HDF5 file 'my_model.h5'
else:
    modelAbs = load_model(savedModelAbs)

#####################################
####  _____ _   _  _   _ __   __ ####
#### |  _  | \ | || \ | |\ \ # # ####
#### | | | |  \| ||  \| | \ V #  ####
#### | | | | . ` || . ` | #   \  ####
#### \ \_# # |\  || |\  |# #^\ \ ####
####  \___#\_| \_#\_| \_#\#   \# ####
####                             ####
#####################################

output_model_path = lambda m : "./{}.onnx".format(m.name)

print("\n\ncreate origMOnnx:\n")
for l in modelOrig.layers:
    print("{} shapes: in={},out={}".format(l.name, l.input_shape, l.output_shape))
origMOnnx = keras2onnx.convert_keras(modelOrig, modelOrig.name+"_onnx", debug_mode=1)
keras2onnx.save_model(origMOnnx, output_model_path(modelOrig))

print("\n\ncreate absMOnnx:\n")
absMOnnx = keras2onnx.convert_keras(modelAbs, modelAbs.name+"_onnx", debug_mode=1)
keras2onnx.save_model(absMOnnx, output_model_path(modelAbs))

print("\n\ncreate origMOnnxMbou:\n")
origMOnnxMbou = monnx(origMOnnx)
print("\n\ncreate absMOnnxMbou:\n")
absMOnnxMbou  = monnx(absMOnnx)
    
exit()
##################################################################
####______     _                    _____ _                   ####
####|  _  \   | |                  /  __ \ |                  ####
####| | | |___| |__  _   _  __ _   | /  \/ | ___  _ __   ___  ####
####| | | / _ \ '_ \| | | |/ _` |  | |   | |/ _ \| '_ \ / _ \ ####
####| |/ /  __/ |_) | |_| | (_| |  | \__/\ | (_) | | | |  __/ ####
####|___/ \___|_.__/ \__,_|\__, |   \____/_|\___/|_| |_|\___| ####
####                        __/ |                             ####
####                       |___/                              ####
##################################################################

print("\n\n\n Evaluating \n\n\n")
c2 = modelOrig.get_layer(name="c2")
dReplace = modelAbs.get_layer(name="clnDense")
slice_input_shape = modelOrig.get_layer(name="c2").input_shape[1:]

if cfg_dis_w:
    OrigW = np.ones(c2.get_weights()[0].shape)
else:
    OrigW = c2.get_weights()[0]
AbsW = dReplace.get_weights()[0]
if cfg_dis_b:
    OrigB = np.ones(c2.get_weights()[1].shape)
    AbsB  = np.ones(dReplace.get_weights()[1].shape)
else:
    OrigB = c2.get_weights()[1]
    AbsB  = dReplace.get_weights()[1]

c2.set_weights([OrigW, OrigB])
dReplace.set_weights([AbsW, AbsB])

origModelIn = tf.keras.Sequential([ tf.keras.Input(shape=input_shape), modelOrig.get_layer(name="c1"), modelOrig.get_layer(name="mp1") ])
origModelIn.compile(loss=loss, optimizer=optimizer, metrics=metrics)

origModelSlice = tf.keras.Sequential([ tf.keras.Input(shape=slice_input_shape), c2 ])
origModelSlice.compile(loss=loss, optimizer=optimizer, metrics=metrics)

modelAbsSlice = tf.keras.Sequential( [tf.keras.Input(shape=slice_input_shape),
                                      modelAbs.get_layer(name="rplcFlat"),
                                      dReplace,
                                      modelAbs.get_layer(name="rplcReshape")])
modelAbsSlice.compile(loss=loss, optimizer=optimizer, metrics=metrics)

slice_test = [
    np.array([np.zeros(slice_input_shape)]),
    np.array([np.ones(slice_input_shape)]),    
    origModelIn.predict(x_test[:10])
]

for i,test in enumerate(slice_test):
    evalSlice = np.isclose(modelAbsSlice.predict(test),origModelSlice.predict(test))
    if np.all(evalSlice):
        print("Slice {} Prediction aligned".format(i))
    else:
        print("Slice {} Prediction not aligned".format(i))
        print(np.mean([1 if b else 0 for b in np.nditer(evalSlice)]))

        '''for origM,absM,e in zip(np.nditer(origModelSlice.predict(test)), np.nditer(modelAbsSlice.predict(test)), np.nditer(evalSlice)):
            if not e:
                if not np.isclose(origM, absM):
                    print("False: \n\torig: {} = {} \n\tabs:  {} = {}".format(origM, np.round(origM, 4),absM,  np.round(absM, 4)))'''

        print("OrigW, shape={} :: {}".format(OrigW.shape, OrigW))
        print("AbsW, shape={} :: {}".format(AbsW.shape, AbsW))
        print("OrigB, shape={} :: {}".format(OrigB.shape, OrigB))
        print("AbsB, shape={} :: {}".format(AbsB.shape, AbsB))        
        #print("Manual orig:{}".format(manual_result(OrigW, OrigB, test[0])))
        #print("Manual abs:{}".format(manual_result(AbsW, AbsB, test[0])))
