
import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#pip install keras2onnx
import tensorflow as tf
#from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import numpy as np

from tensorflow.keras.models import load_model
cfg_freshModelOrig = True
cfg_freshModelAbs = True
savedModelOrig = "cnn_abs_orig.h5"
savedModelAbs = "cnn_abs_abs.h5"

def maskAndDensifyNDimConv(origW, origB, mask, convInShape, convOutShape, strides, cfg_dis_w=False):
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

    clnM = tf.keras.Sequential(
        [tf.keras.Input(shape=origM.get_layer(index=0).input_shape[1:])] + 
        list(chain.from_iterable([[l] if l.name != rplcLayerName else [layers.Flatten(name="rplcFlat"),clnDense,layers.Reshape(rplcOut[1:], name="rplcReshape")] for l in origM.layers]))
    )

    clnM.compile(loss="categorical_crossentropy", optimizer="adam", metrics=["accuracy"])
    clnM.summary()

    clnW, clnB = maskAndDensifyNDimConv(origW, origB, mask, rplcIn, rplcOut, strides)
    clnDense.set_weights([clnW, clnB])

    return clnM


