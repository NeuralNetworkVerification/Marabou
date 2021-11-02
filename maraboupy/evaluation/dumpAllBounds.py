
import json
import os
import copy
import numpy as np
import argparse
from CnnAbs import CnnAbs, ModelUtils, DataSet, QueryUtils, AdversarialProperty

from maraboupy import Marabou

parser = argparse.ArgumentParser(description='Bound Dumping')
parser.add_argument("--net", type=str, choices=['network' + i for i in ['A','B','C']] + [''], required=False, help="Chosen network to dump bounds on")
parser.add_argument("--sample", type=int, required=False, default=-1, help="Chosen sample")
parser.add_argument("--distance", type=float, required=False, default=-1.0, help="Chosen distance")
args = parser.parse_args()

if not args.net:
    networks = ['network{}.h5'.format(i) for i in ['A','B','C']]
else:
    networks = [args.net + '.h5']
if args.sample < 0:
    samples = range(100)
else:
    samples = [args.sample]
if args.distance < 0:
    distances = [0.01, 0.02, 0.03]
else:
    distances = [args.distance]

options = dict(verbosity=0, timeoutInSeconds=800, milpTightening='lp', dumpBounds=True, tighteningStrategy='sbt', milpSolverTimeout=100)
logDir="/".join([CnnAbs.maraboupyPath, "logs_CnnAbs", 'dumpedJsonGeneral']) + "/"
os.makedirs(logDir, exist_ok=True)
modelUtils = ModelUtils(DataSet('mnist'), Marabou.createOptions(**options), logDir)

for network in networks:
    modelTF = modelUtils.loadModel('/'.join([CnnAbs.maraboupyPath, 'evaluation', network]))
    model = modelUtils.tf2Model(modelTF)
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
            
            QueryUtils.setAdversarial(modelCopy, sample, distance, yMax, ySecond, valueRange=modelUtils.ds.valueRange)
            property = AdversarialProperty(sample, yMax, ySecond, distance, sampleIndex)
            savedFile = CnnAbs.dumpBoundsDir + '/' + CnnAbs.boundsFilePath(network, property)
            if os.path.exists(savedFile):
                continue
            
            print('Started dumping {}'.format(savedFile))
            ipq = QueryUtils.preprocessQuery(modelCopy, modelUtils.options, modelUtils.logDir)
            dumpBoundsFile = modelUtils.logDir + 'dumpBounds.json'
            if ipq.getNumberOfVariables() == 0:
                boundDict = dict()
            else:
                with open(dumpBoundsFile, "r") as f:
                    boundList = json.load(f)
                boundDict = {bound["variable"] : (bound["lower"], bound["upper"]) for bound in boundList}
            if os.path.exists(dumpBoundsFile):
                os.remove(dumpBoundsFile)
            os.makedirs('/'.join(savedFile.split('/')[:-1]), exist_ok=True)
            with open(savedFile, "w") as f:
                json.dump(boundDict, f, indent = 4)
            print('Finished dumping {}'.format(savedFile))
