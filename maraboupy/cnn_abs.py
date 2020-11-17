
import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#pip install keras2onnx
import keras2onnx
import tensorflow as tf
#from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import numpy as np

from tensorflow.keras.models import load_model
cfg_freshModelAbs = True
savedModelAbs = "cnn_abs_abs.h5"


class mnistProp:
    num_classes = 10
    input_shape = (28,28,1)
    (x_train, y_train), (x_test, y_test) = tf.keras.datasets.mnist.load_data()
    x_train, x_test = x_train / 255.0, x_test / 255.0
    x_train = np.expand_dims(x_train, -1)
    x_test = np.expand_dims(x_test, -1)
    featureShape=(1,28,28)
    loss='sparse_categorical_crossentropy'
    optimizer='adam'
    metrics=['accuracy']
    output_model_path = lambda m : "./{}.onnx".format(m.name)
    savedModelAbs = "cnn_abs_abs.h5"
    cfg_dis_w = False
    cfg_dis_b = False    
    

def maskAndDensifyNDimConv(origW, origB, mask, convInShape, convOutShape, strides, cfg_dis_w=mnistProp.cfg_dis_w):
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

def cloneAndMaskConvModel(origM, rplcLayerName, mask):
    rplcIn = origM.get_layer(name=rplcLayerName).input_shape        
    rplcOut = origM.get_layer(name=rplcLayerName).output_shape
    origW = origM.get_layer(name=rplcLayerName).get_weights()[0]
    origB = origM.get_layer(name=rplcLayerName).get_weights()[1]    
    clnDense = layers.Dense(units=np.prod(rplcOut[1:]),name="clnDense")
    strides = origM.get_layer(name=rplcLayerName).strides

    tf.keras.backend.clear_session()
    origMCloneTemp = models.clone_model(origM)
    clnM = tf.keras.Sequential(
        #[tf.keras.Input(shape=origM.get_layer(index=0).input_shape[1:])] + 
        list(chain.from_iterable([[l] if l.name != rplcLayerName else [layers.Flatten(name="rplcFlat"),clnDense,layers.Reshape(rplcOut[1:], name="rplcReshape")] for l in origMCloneTemp.layers])),
        name="AbsModel"        
    )
    for l in clnM.layers:
        l._name = l.name + "_clnM"

    clnM.build(input_shape=mnistProp.featureShape)
    clnM.compile(loss=mnistProp.loss, optimizer=mnistProp.optimizer, metrics=mnistProp.metrics)
    clnM.summary()

    clnW, clnB = maskAndDensifyNDimConv(origW, origB, mask, rplcIn, rplcOut, strides)
    clnDense.set_weights([clnW, clnB])

    return clnM


def genCnnForAbsTest(cfg_limitCh=True, cfg_freshModelOrig=True, savedModelOrig="cnn_abs_orig.h5"):

    print("Starting model building")
    #https://keras.io/examples/vision/mnist_convnet/

    if cfg_freshModelOrig:
        num_ch = 2 if cfg_limitCh else 32
        tf.keras.backend.clear_session()
        origM = tf.keras.Sequential(
            [                    
                layers.Conv2D(num_ch, kernel_size=(3,3), activation="relu", name="c1", input_shape=mnistProp.input_shape),
                layers.MaxPooling2D(pool_size=(2,2), name="mp1"),
                layers.Conv2D(num_ch, kernel_size=(3,3), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2,2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dropout(0.5, name="do1"),
                layers.Dense(mnistProp.num_classes, activation="softmax", name="sm1")
            ],
            name="origModel"
        )

        batch_size = 60 #FIXME
        epochs = 1 #FIXME

        origM.build(input_shape=mnistProp.featureShape)
        origM.summary()        
        origM.compile(optimizer=mnistProp.optimizer, loss=mnistProp.loss, metrics=mnistProp.metrics)
        origM.fit(mnistProp.x_train, mnistProp.y_train, epochs=1, batch_size=batch_size, validation_split=0.1)
        score = origM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print("Test loss:", score[0])
        print("Test accuracy:", score[1])
        origM.save(savedModelOrig)
        
    else:
        pring("Error")
        exit(1)
        origM = load_model(savedModelOrig)

    rplcLayerName = "c2"
    return origM, rplcLayerName

