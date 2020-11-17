
import sys
import os
import tensorflow as tf
import numpy as np
import keras2onnx
import onnx
import onnxruntime
from cnn_abs import *
import logging

#sys.path.append("/cs/labs/guykatz/matanos/Marabou")
#sys.path.append("/cs/labs/guykatz/matanos/Marabou/maraboupy")

from itertools import product, chain
#from maraboupy import MarabouCore, Marabou
from maraboupy import MarabouNetworkONNX as monnx
from tensorflow.keras import datasets, layers, models
import matplotlib.pyplot as plt
    
logger = logging.getLogger('cnn_abs_testbench.log')
logger.setLevel(logging.DEBUG)

logger = logging.getLogger('cnnAbsTB')
logger.setLevel(logging.DEBUG)
fh = logging.FileHandler('cnnAbsTB.log')
fh.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.ERROR)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)
logger.addHandler(fh)
logger.addHandler(ch)

logger.info("Started model building")
modelOrig, replaceLayerName = genCnnForAbsTest()
logger.info("Finished model building")

logger.info("Choosing adversarial example")
xAdvInd = int(np.random.randint(0, mnistProp.x_test.shape[0], size=1)[0])
xAdv = mnistProp.x_test[xAdvInd]
yAdv = mnistProp.y_test[xAdvInd]
yPredict = modelOrig.predict(np.array([xAdv]))
yMax = yPredict.argmax()
yPredictNoMax = np.copy(yPredict)
yPredictNoMax[0][yMax] = 0
ySecond = yPredictNoMax.argmax()
inDist = 0.01
if ySecond == yMax:
    ySecond = 0 if yMax > 0 else 1
fName = "xAdv.png"
logger.info("Printing original input: {}".format(fName))
plt.title('Example %d. Label: %d' % (xAdvInd, yAdv))
plt.imshow(xAdv.reshape(xAdv.shape[:-1]), cmap='Greys')
plt.savefig(fName)

logger.info("Starting Abstractions")
maskList = []

for mask in maskList:
    
    
else:
    logger.info("Started transalting original model to ONNX")
    origMOnnx = keras2onnx.convert_keras(modelOrig, modelOrig.name+"_onnx", debug_mode=1)
    origMOnnxName = mnistProp.output_model_path(modelOrig)
    keras2onnx.save_model(origMOnnx, origMOnnxName)
    logger.info("Finished transalting original model to ONNX")
    
