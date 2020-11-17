
import sys
import os
import tensorflow as tf
import numpy as np
import keras2onnx
import onnx
import onnxruntime
from cnn_abs import *

#sys.path.append("/cs/labs/guykatz/matanos/Marabou")
#sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#from maraboupy import MarabouCore, Marabou
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models

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
    
modelOrig, replaceLayerName = genCnnForAbsTest()
origMOnnx = keras2onnx.convert_keras(modelOrig, modelOrig.name+"_onnx", debug_mode=1)
keras2onnx.save_model(origMOnnx, mnistProp.output_model_path(modelOrig))

exit()

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

    score = modelAbs.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
    print("Test loss:", score[0])
    print("Test accuracy:", score[1])

    if np.all(np.isclose(modelAbs.predict(mnistProp.x_test), modelOrig.predict(mnistProp.x_test))):
        print("Prediction aligned")    
    else:
        print("Prediction not aligned")
        
    modelAbs.save(mnistProp.savedModelAbs)  # creates a HDF5 file 'my_model.h5'
else:
    modelAbs = models.load_model(mnistProp.savedModelAbs)

#####################################
####  _____ _   _  _   _ __   __ ####
#### |  _  | \ | || \ | |\ \ # # ####
#### | | | |  \| ||  \| | \ V #  ####
#### | | | | . ` || . ` | #   \  ####
#### \ \_# # |\  || |\  |# #^\ \ ####
####  \___#\_| \_#\_| \_#\#   \# ####
####                             ####
#####################################

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

if mnistProp.cfg_dis_w:
    OrigW = np.ones(c2.get_weights()[0].shape)
else:
    OrigW = c2.get_weights()[0]
AbsW = dReplace.get_weights()[0]
if mnistProp.cfg_dis_b:
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
