
import json
import os
import copy
import CnnAbs

import tensorflow as tf
import keras2onnx
import MarabouNetworkONNX

networks = ['network{}.h5'.format(i) for i in ['A','B','C']]
samples = range(2)
distances = [0.01, 0.02, 0.03]

options = dict(verbosity=0, timeoutInSeconds=800, milpTightening='lp', dumpBounds=True, tighteningStrategy='sbt', milpSolverTimeout=100)
logDir="/".join([CnnAbs.CnnAbs.maraboupyPath, "logs_CnnAbs", 'dumpedJsonGeneral'])
os.makedirs(logDir, exist_ok=True)
modelUtils = CnnAbs.ModelUtils(CnnAbs.DataSet('mnist'), options, logDir)

for network in networks:
    modelTF = modelUtils.loadModel('/'.join([CnnAbs.CnnAbs.maraboupyPath, 'evaluation', network]))

    modelOnnx = keras2onnx.convert_keras(modelTF, modelTF.name + "_onnx", debug_mode=1)
    modelOnnxName = CnnAbs.ModelUtils.onnxNameFormat(modelTF)
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
            property = CnnAbs.AdversarialProperty(sample, yMax, ySecomd, distance, sampleIndex)
            savedFile = CnnAbs.CnnAbs.dumpBoundsDir + CnnAbs.CnnAbs.boundsFilePath(network, property)
            
            print('Started dumping {}'.format(savedFile))
            ipq = CnnAbs.QueryUtils.preprocessQuery(modelCopy, modelUtils.options, modelUtils.logDir)
            dumpBoundsFile = modelUtils.logDir + '/' + 'dumpBounds.json'            
            if ipq.getNumberOfVariables() == 0:
                boundDict = dict()
            else:
                with open(dumpBoundsFile, "r") as f:
                    boundList = json.load(f)
                boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}
            if os.path.exists(dumpBoundsFile):
                os.remove(dumpBoundsFile)
            with open(CnnAbs.CnnAbs.dumpBoundsDir + CnnAbs.CnnAbs.boundsFilePath(network, property), "w") as f:
                json.dump(boundDict, f, indent = 4)
            print('Finished dumping {}'.format(savedFile))
