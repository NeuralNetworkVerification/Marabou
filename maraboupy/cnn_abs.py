import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product
#pip install keras2onnx
import tensorflow as tf
#from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import numpy as np

import copy

from tensorflow.keras.models import load_model
cfg_freshModelOrig = True
cfg_freshModelAbs = True
savedModelOrig = "cnn_abs_orig.h5"
savedModelAbs = "cnn_abs_abs.h5"

cfg_dis_w = True
cfg_dis_b = False

#Log:
# Passing - True, True

def maskAndDensifyNDimConv(origW, origB, mask, convInShape, convOutShape, strides):
    if convOutShape[0] == None:
        convOutShape = convOutShape[1:]
    if convInShape[0] == None:
        convInShape = convInShape[1:]        
    replaceW = np.zeros((np.prod(convInShape), np.prod(convOutShape)))
    fDim = origW.shape[:-2] # W/O in/out channels.

    sOff = [int(np.prod(convInShape[i+1:]))  for i in range(len(convInShape))]
    tOff = [int(np.prod(convOutShape[i+1:])) for i in range(len(convOutShape))]

    tCoors = product(*[range(d) for d in convOutShape[:-1]])
    inChNum = origW.shape[-2]
    outChNum = origW.shape[-1]
    for tCoor in tCoors:                
        if mask[tCoor]:
            sCoors = product(*[[coor for coor in [j*s+t for j in range(f)] if coor < i] for f,s,t,i in zip(fDim, strides, tCoor, convInShape)])
            sCoors = list(sCoors)
            wMats  = [origW[coor] for coor in product(*[range(d) for d in fDim])]
            for sCoor, wMat in zip(sCoors, wMats):
                for in_ch, out_ch in product(range(inChNum), range(outChNum)):
                    sCoorFlat = sum([coor * off for coor, off in zip((*sCoor, in_ch) , sOff)])
                    tCoorFlat = sum([coor * off for coor, off in zip((*tCoor, out_ch), tOff)])
                    replaceW[sCoorFlat, tCoorFlat] = np.ones(wMat[in_ch, out_ch].shape) if cfg_dis_w else wMat[in_ch, out_ch]
    replaceB = np.tile(origB, np.prod(convOutShape[:-1]))
    return replaceW, replaceB

    '''
    replace_w = np.zeros(replace_dense.get_weights()[0].shape) 
    filter_dim = orig_w.shape[0:-2]
    soff = [np.prod(orig_w.shape[i+1:-1]) for i in range(len(orig_w.shape[:-2]))] + [1]
    toff = [np.prod(c2out[i+2:]) for i in range(len(c2out)-2)] + [1]

    for target_coor in product(*[range(d) for d in c2out[1:-1]]):
        if mask[target_coor]:
            for source_coor, wMat in zip(product(*[[i*s+t for i in range(d)] for d,s,t in zip(filter_dim, strides, target_coor)]), [orig_w[c] for c in product(*[range(d) for d in filter_dim])]):
                for in_ch, out_ch in product(range(orig_w.shape[-2]), range(orig_w.shape[-1])):
                    flat_s = sum([c * off for c,off in zip((*source_coor, in_ch) , soff)])
                    flat_t = sum([c * off for c,off in zip((*target_coor, out_ch), toff)])                
                    replace_w[flat_s, flat_t] =   np.ones(wMat[in_ch, out_ch].shape) if cfg_dis_w else wMat[in_ch, out_ch]

    replace_b = np.tile(orig_b, np.prod(c2out[1:-1]))'''

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
input_shape = (28 * 28,1)

# the data, split between train and test sets
(x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()

# Scale images to the [0, 1] range
x_train = x_train.astype("float32") / 255
x_test = x_test.astype("float32") / 255
# Make sure images have shape (28, 28, 1)
x_train = x_train.reshape(x_train.shape[0], 28 * 28)
x_test = x_test.reshape(x_test.shape[0], 28 * 28)
x_train = np.expand_dims(x_train, -1)
x_test = np.expand_dims(x_test, -1)
print("x_train shape:", x_train.shape)
print(x_train.shape[0], "train samples")
print(x_test.shape[0], "test samples")

# convert class vectors to binary class matrices
y_train = tf.keras.utils.to_categorical(y_train, num_classes)
y_test = tf.keras.utils.to_categorical(y_test, num_classes)

if cfg_freshModelOrig:
    modelOrig = tf.keras.Sequential(
        [
            tf.keras.Input(shape=input_shape),
            layers.Conv1D(32, kernel_size=(3,), activation="relu", name="c1"),
            layers.MaxPooling1D(pool_size=(2,), name="mp1"),
            layers.Conv1D(64, kernel_size=(3,), activation="relu", name="c2"),
            layers.MaxPooling1D(pool_size=(2,), name="mp2"),
            layers.Flatten(name="f1"),
            layers.Dropout(0.5, name="do1"),
            layers.Dense(num_classes, activation="softmax", name="sm1"),
        ]
    )

    modelOrig.summary()

    #batch_size = 128
    #epochs = 15
    batch_size = 60
    epochs = 1 

    modelOrig.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
    modelOrig.fit(x_train, y_train, batch_size=batch_size, epochs=epochs, validation_split=0.1)

    #score = modelOrig.evaluate(x_test, y_test, verbose=0)
    #print("Test loss:", score[0])
    #print("Test accuracy:", score[1])
    modelOrig.save(savedModelOrig)  # creates a HDF5 file 'my_model.h5'
else:
    modelOrig = load_model(savedModelOrig)

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
    
    c2out = modelOrig.get_layer(name="c2").output_shape
    c2in = modelOrig.get_layer(name="c2").input_shape    
    replace_dense = layers.Dense(units=np.prod(c2out[1:]),name="dReplace")
    strides = modelOrig.get_layer(name="c2").strides

    print("Clone modelOrig")
    modelAbs = tf.keras.Sequential(
        [
            tf.keras.Input(shape=input_shape),
            modelOrig.get_layer(name="c1"),
            modelOrig.get_layer(name="mp1"),
            layers.Flatten(name="FlattenReplace"),
            replace_dense,
            layers.Reshape(modelOrig.get_layer(name="c2").output_shape[1:], name="reshapeReplace"),
            modelOrig.get_layer(name="mp2"),
            modelOrig.get_layer(name="f1"),
            modelOrig.get_layer(name="do1"),
            modelOrig.get_layer(name="sm1")
        ]
    )

    modelAbs.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
    modelAbs.summary()
        
    orig_w = modelOrig.get_layer(name="c2").get_weights()[0]
    orig_b = modelOrig.get_layer(name="c2").get_weights()[1]
    mask = np.ones(c2out[1:-1]) #Mask is not considering different channels

    replace_w, replace_b = maskAndDensifyNDimConv(orig_w, orig_b, mask, c2in, c2out, strides)
    replace_dense.set_weights([replace_w, replace_b])

    #score = modelAbs.evaluate(x_test, y_test, verbose=0)
    #print("Test loss:", score[0])
    #print("Test accuracy:", score[1])

    if np.all(np.isclose(modelAbs.predict(x_test), modelOrig.predict(x_test))):
        print("Prediction aligned")    
    else:
        print("Prediction not aligned")
        
    modelAbs.save(savedModelAbs)  # creates a HDF5 file 'my_model.h5'
else:
    modelAbs = load_model(savedModelAbs)

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
dReplace = modelAbs.get_layer(name="dReplace")
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
origModelIn.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

origModelSlice = tf.keras.Sequential([ tf.keras.Input(shape=slice_input_shape), c2 ])
origModelSlice.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

modelAbsSlice = tf.keras.Sequential( [tf.keras.Input(shape=slice_input_shape),
                                      modelAbs.get_layer(name="FlattenReplace"),
                                      dReplace,
                                      modelAbs.get_layer(name="reshapeReplace")])
modelAbsSlice.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])

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

        for origM,absM,e in zip(np.nditer(origModelSlice.predict(test)), np.nditer(modelAbsSlice.predict(test)), np.nditer(evalSlice)):
            if not e:
                if not np.isclose(origM, absM):
                    print("False: \n\torig: {} = {} \n\tabs:  {} = {}".format(origM, np.round(origM, 4),absM,  np.round(absM, 4)))

        print(OrigB)                    
        print(AbsB)

