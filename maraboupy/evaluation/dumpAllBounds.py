
import json
import os
import copy
from CnnAbs import CnnAbs, ModelUtils, DataSet, QueryUtils, AdversarialProperty

from maraboupy import Marabou
import tensorflow as tf
import keras2onnx
import MarabouNetworkONNX

#tf.compat.v1.enable_v2_behavior()

networks = ['network{}.h5'.format(i) for i in ['A','B','C']]
samples = range(2)
distances = [0.01, 0.02, 0.03]

options = dict(verbosity=0, timeoutInSeconds=800, milpTightening='lp', dumpBounds=True, tighteningStrategy='sbt', milpSolverTimeout=100)
options = Marabou.createOptions(**options)
logDir="/".join([CnnAbs.maraboupyPath, "logs_CnnAbs", 'dumpedJsonGeneral'])
os.makedirs(logDir, exist_ok=True)
modelUtils = ModelUtils(DataSet('mnist'), options, logDir)

for network in networks:
    modelTF = modelUtils.loadModel('/'.join([CnnAbs.maraboupyPath, 'evaluation', network]))

    modelOnnx = keras2onnx.convert_keras(modelTF, modelTF.name + "_onnx", debug_mode=1)
    modelOnnxName = ModelUtils.onnxNameFormat(modelTF)
    keras2onnx.save_model(modelOnnx, logDir + modelOnnxName)
    model = MarabouNetworkONNX.MarabouNetworkONNX(logDir + modelOnnxName)
    
#    model = modelUtils.tf2Model(modelTF)    
    for sampleIndex in samples:
        for distance in distances:

            modelCopy = copy.deepcopy(model)
            
            sample = modelUtils.ds.x_test[sampleIndex]
            yAdv = modelUtils.ds.y_test[sampleIndex]
            yPredict = modelTF.predict(np.array([sample]))
            yMax = yPredict.argmax()
            yPredictNoMax = np.copy(yPredict)
            yPredictNoMax[0][yMax] = np.min(yPredict)
            ySecond = yPredictNoMax.argmax()
            if ySecond == yMax:
                ySecond = 0 if yMax > 0 else 1
            
            QueryUtils.setAdversarial(model, sample, distance, yMax, ySecond, valueRange=modelUtils.ds.valueRange)
            property = AdversarialProperty(sample, yMax, ySecomd, distance, sampleIndex)
            savedFile = CnnAbs.dumpBoundsDir + CnnAbs.boundsFilePath(network, property)
            
            print('Started dumping {}'.format(savedFile))
            ipq = QueryUtils.preprocessQuery(modelCopy, modelUtils.options, modelUtils.logDir)
            dumpBoundsFile = modelUtils.logDir + '/' + 'dumpBounds.json'            
            if ipq.getNumberOfVariables() == 0:
                boundDict = dict()
            else:
                with open(dumpBoundsFile, "r") as f:
                    boundList = json.load(f)
                boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}
            if os.path.exists(dumpBoundsFile):
                os.remove(dumpBoundsFile)
            with open(CnnAbs.dumpBoundsDir + CnnAbs.boundsFilePath(network, property), "w") as f:
                json.dump(boundDict, f, indent = 4)
            print('Finished dumping {}'.format(savedFile))
