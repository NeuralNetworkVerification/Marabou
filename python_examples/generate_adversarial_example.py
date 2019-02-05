from maraboupy import Marabou, MarabouUtils
import numpy as np
import matplotlib.pyplot as plt

def find_ae(proto_file, input_x_file, pred, ae_target, delta):
    """Returns an adversarial example for an MNIST network around a given input point

    Arguments:
    proto_file: frozen protobuf network file
    input_x_file: input point npy file
    pred: predicted label for the input point by the given network
    ae_target: adversarial label for the misclassification
    delta: search delta around the input point(using L-infinity norm)
    
    Returns:
    val: assignment array for the adversarial example. If there is no adversarial example found in the given search region then returns an empty array
    stats: statistics object for the given run
    """
    net = Marabou.read_tf(proto_file)
    input_x = np.load(input_x_file)
    for i_var, i in enumerate(net.inputVars[0][0]):
        net.setLowerBound(i_var, max(input_x[i]-delta, 0.))
        net.setUpperBound(i_var, min(input_x[i]+delta, 1.))
        
    MarabouUtils.addInequality(net, [net.outputVars[0][pred], net.outputVars[0][ae_target]], [1, -1], 0)
    val, stats = net.solve('', False)
    return val, stats

input_x_file = 'inputs/145.npy'
proto_file = 'models/conv.pb'

delta = 0.08
pred = 1
ae_target = 3;

# find an adversarial example within delta distance 0.08 from the input point which would be misclassified as 3
val, stats = find_ae(proto_file, input_x_file, pred, ae_target, delta)
print(('Adversarial Example Found: %r, Time Taken: %f\n')%(len(val)!=0, stats.getTotalTime()/1000))

delta = 0.1
# find an adversarial example within delta distance 0.1 from the input point which would be misclassified as 3
val, stats = find_ae(proto_file, input_x_file, pred, ae_target, delta)
print(('Adversarial Example Found: %r, Time Taken: %f\n')%(len(val)!=0, stats.getTotalTime()/1000))

# input 374.npy has the predicted label of 8 and can also be used in a similar way