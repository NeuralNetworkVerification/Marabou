
import sys
import os

sys.path.append("/cs/labs/guykatz/matanos/Marabou")
sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#pip install keras2onnx
import keras2onnx
import tensorflow as tf
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import numpy as np
import logging
import matplotlib.pyplot as plt

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
    cfg_fresh = False
    numClones = 0
    
#replaceW, replaceB = maskAndDensifyNDimConv(np.ones((2,2,1,1)), np.array([0.5]), np.ones((3,3,1)), (3,3,1), (3,3,1), (1,1))
def maskAndDensifyNDimConv(origW, origB, mask, convInShape, convOutShape, strides, cfg_dis_w=mnistProp.cfg_dis_w):
    #https://stackoverflow.com/questions/36966392/python-keras-how-to-transform-a-dense-layer-into-a-convolutional-layer  
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
            #print("wMats={}".format(wMats))
            for sCoor, wMat in zip(sCoors, wMats):
                for in_ch, out_ch in product(range(inChNum), range(outChNum)):
                    sCoorFlat = sum([coor * off for coor, off in zip((*sCoor, in_ch) , sOff)])
                    tCoorFlat = sum([coor * off for coor, off in zip((*tCoor, out_ch), tOff)])
                    if wMat[in_ch, out_ch].shape != ():
                        raise Exception("wMat[x,y] values should be scalars. shape={}".format( wMat[in_ch, out_ch].shape))
                    replaceW[sCoorFlat, tCoorFlat] = np.ones(wMat[in_ch, out_ch].shape) if cfg_dis_w else wMat[in_ch, out_ch]
    replaceB = np.tile(origB, np.prod(convOutShape[:-1]))
    return replaceW, replaceB

def cloneAndMaskConvModel(origM, rplcLayerName, mask, cfg_freshModelAbs=True):
    if cfg_freshModelAbs:
        rplcIn = origM.get_layer(name=rplcLayerName).input_shape        
        rplcOut = origM.get_layer(name=rplcLayerName).output_shape
        origW = origM.get_layer(name=rplcLayerName).get_weights()[0]
        origB = origM.get_layer(name=rplcLayerName).get_weights()[1]    
        clnDense = layers.Dense(units=np.prod(rplcOut[1:]),name="clnDense")
        strides = origM.get_layer(name=rplcLayerName).strides
        origMCloneTemp = models.clone_model(origM)
        origMCloneTemp.set_weights(origM.get_weights())     
        clnM = tf.keras.Sequential(
            list(chain.from_iterable([[l] if l.name != rplcLayerName else [layers.Flatten(name="rplcFlat"),clnDense,layers.Reshape(rplcOut[1:], name="rplcReshape")] for l in origMCloneTemp.layers])),
            name=("AbsModel_{}".format(mnistProp.numClones))
        )
        
        for lTemp,lCln in [(t,c) for t in origMCloneTemp.layers for c in clnM.layers]:
            if lCln.name == lTemp.name:
                lCln.set_weights(lTemp.get_weights())
        for l in clnM.layers:
            l._name = l.name + "_clnM" + str(mnistProp.numClones)

        mnistProp.numClones += 1            

        clnM.build(input_shape=mnistProp.featureShape)
        clnM.compile(loss=mnistProp.loss, optimizer=mnistProp.optimizer, metrics=mnistProp.metrics)
        clnM.summary()

        clnW, clnB = maskAndDensifyNDimConv(origW, origB, mask, rplcIn, rplcOut, strides)
        clnDense.set_weights([clnW, clnB])

        score = clnM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        score1 = clnM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        score2 = clnM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print(score,score1,score2)
        print("(Clone, neurons masked:{}%) Test loss:".format(100*(1 - np.average(mask))), score[0])
        print("(Clone, neurons masked:{}%) Test accuracy:".format(100*(1 - np.average(mask))), score[1])
         
        if np.all(np.isclose(clnM.predict(mnistProp.x_test), origM.predict(mnistProp.x_test))):
            print("Prediction aligned")    
        else:
            print("Prediction not aligned")

        #clnM.save(mnistProp.savedModelAbs) FIXME
    else:
        clnM = models.load_model(mnistProp.savedModelAbs)

    return clnM


def genCnnForAbsTest(cfg_limitCh=True, cfg_freshModelOrig=mnistProp.cfg_fresh, savedModelOrig="cnn_abs_orig.h5"):

    print("Starting model building")
    #https://keras.io/examples/vision/mnist_convnet/

    if cfg_freshModelOrig:
        num_ch = 2 if cfg_limitCh else 32
        #tf.keras.backend.clear_session()
        origM = tf.keras.Sequential(
            [                    
                layers.Conv2D(num_ch, kernel_size=(3,3), activation="relu", name="c1", input_shape=mnistProp.input_shape),
                layers.MaxPooling2D(pool_size=(2,2), name="mp1"),
                layers.Conv2D(num_ch, kernel_size=(3,3), activation="relu", name="c2"),
                layers.MaxPooling2D(pool_size=(2,2), name="mp2"),
                layers.Flatten(name="f1"),
                layers.Dropout(0.5, name="do1"),
                #layers.Dense(mnistProp.num_classes, activation="softmax", name="sm1") FIXME TODO
                layers.Dense(mnistProp.num_classes, activation="relu", name="sm1")
            ],
            name="origModel"
        )

        batch_size = 128
        epochs = 15

        origM.build(input_shape=mnistProp.featureShape)
        origM.summary()        
        origM.compile(optimizer=mnistProp.optimizer, loss=mnistProp.loss, metrics=mnistProp.metrics)
        origM.fit(mnistProp.x_train, mnistProp.y_train, epochs=epochs, batch_size=batch_size, validation_split=0.1)
        score = origM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print("(Original) Test loss:", score[0])
        print("(Original) Test accuracy:", score[1])
        origM.save(savedModelOrig)
        
    else:
        origM = load_model(savedModelOrig)
        score = origM.evaluate(mnistProp.x_test, mnistProp.y_test, verbose=0)
        print("(Original) Test loss:", score[0])
        print("(Original) Test accuracy:", score[1])

    rplcLayerName = "c2"
    return origM, rplcLayerName

def getBoundsInftyBall(x, r, pos=True):
    if pos:
        return np.maximum(x - r,np.zeros(x.shape)), x + r
    return x - r, x + r

def inBoundsInftyBall(x, r, p, pos=True):
    l,u = getBoundsInftyBall(x,r,pos=pos)
    return np.all(np.less_equal(l,p)) and np.all(np.greater_equal(u,p))
    
def setAdversarial(net, x, inDist,yCorrect,yBad):
    inAsNP = np.array(net.inputVars)
    x = x.reshape(inAsNP.shape)
    xDown, xUp = getBoundsInftyBall(x, inDist)
    for i,d,u in zip(np.nditer(inAsNP),np.nditer(xDown),np.nditer(xUp)):    
        net.setLowerBound(i.item(),d.item())
        net.setUpperBound(i.item(),u.item())
    for j,o in enumerate(np.nditer(np.array(net.outputVars))):
        if j == yCorrect:
            yCorrectVar = o.item()
        if j == yBad:
            yBadVar = o.item()
    net.addInequality([yCorrectVar, yBadVar], [1,-1], 0) # correct - bad <= 0
    return net
    
def cexToImage(net, valDict,xAdv):
    cex = np.array([valDict[i.item()] for i in np.nditer(np.array(net.inputVars))]).reshape(xAdv.shape)
    print(np.array(net.outputVars))
    cexPrediction = np.array([valDict[o.item()] for o in np.nditer(np.array(net.outputVars))])
    return cex, cexPrediction
    
def runMarabouOnKeras(model, logger, xAdv, inDist, yMax, ySecond):
    logger.info("Started converting model ({}) to ONNX".format(model.name))
    modelOnnx = keras2onnx.convert_keras(model, model.name+"_onnx", debug_mode=(1 if logger.level==logging.DEBUG else 0))
    modelOnnxName = mnistProp.output_model_path(model)
    keras2onnx.save_model(modelOnnx, modelOnnxName)
    logger.info("Finished converting model ({}) to ONNX. Output file is {}".format(model.name, modelOnnxName))
    logger.info("Started converting model ({}) from ONNX to MarabouNetwork".format(model.name))
    modelOnnxMarabou  = monnx.MarabouNetworkONNX(modelOnnxName)
    setAdversarial(modelOnnxMarabou, xAdv, inDist, yMax, ySecond)
    logger.info("Finished converting model ({}) from ONNX to MarabouNetwork".format(model.name))
    logger.info("Started solving query ({})".format(model.name))
    vals, stats = modelOnnxMarabou.solve(verbose=False)
    sat = len(vals) > 0
    logger.info("Finished solving query ({}). Result is ".format(model.name, 'SAT' if sat else 'UNSAT'))
    if not sat:
        return False, np.array([]), np.array([])
    cex, cexPrediction = cexToImage(modelOnnxMarabou, vals, xAdv)
    fName = "Cex.png"
    mbouPrediction = cexPrediction.argmax()
    kerasPrediction = model.predict(np.array([cex])).argmax()
    logger.info("Printing counter example: {}. MarabouY={}, modelY={}".format(fName,mbouPrediction, kerasPrediction))        
    plt.title('CEX, MarabouY={}, modelY={}'.format(mbouPrediction, kerasPrediction))
    plt.imshow(cex.reshape(xAdv.shape[:-1]), cmap='Greys')
    plt.savefig(fName)
    return True, cex, cexPrediction

def isCEXSporious(model, x, inDist, yCorrect, yBad, cex, logger):
    if not inBoundsInftyBall(x, inDist, cex):
        logger.info("CEX out of bounds")
        raise Exception("CEX out of bounds")
    if model.predict(np.array([cex])).argmax() == yCorrect:
        logger.info("Found sporious CEX")
        return True
    logger.info("Found real, not sporious, CEX")
    if model.predict(np.array([cex])).argmax() != yBad:
        logger.info("CEX prediction value is not the bad value described in the adversarial property.")
    return False

def genMask(shape, lBound, uBound):
    onesInd = list(product(*[range(l,min(u+1,dim)) for dim, l, u in zip(shape, lBound, uBound)]))
    mask = np.zeros(shape)
    for ind in onesInd:
        mask[ind] = 1
    return mask
