import numpy as np
import sys
import copy

from maraboupy import Marabou

from maraboupy import MarabouUtils
nnet_file_name = "../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"
# proto_file_name = "./networks/ACASXU_TF12_run3_DS_Minus15_Online_tau0_pra1_200Epochs.pb"
proto_file_name = "./examples/networks/ACASXU_TF12_run3_DS_Minus15_Online_tau1_pra1_200Epochs.pb"
input0 = -0.328422874212265
input1 = 0.40932923555374146
input2 = -0.017379289492964745
input3 = -0.2747684121131897
input4 = -0.30628132820129395
inputs = [input0, input1, input2, input3, input4]
output0 = 0.5
output1 = -0.18876336514949799
output2 = 0.8081545233726501
output3 = -2.764213800430298
output4 = -0.12992984056472778
outputs = [output0, output1, output2, output3, output4]
size = 5

def solve(nnet, epsilon, delta):
    for i in range(5):
        ###provides L_inf range +/- delta for inputs
        nnet.setLowerBound(nnet.inputVars[0][i], inputs[i] - delta)
        nnet.setUpperBound(nnet.inputVars[0][i], inputs[i] + delta)

    ###rs is |out_0[i] - out[i]| for i in dim_out
    rs = set()

    ###for each output
    for i in range(5):
        ###   x = out_? - out_0
        x = nnet.getNewVariable()
        e = MarabouUtils.Equation()
        e.setScalar(outputs[i])
        e.addAddend(1,nnet.outputVars[0][i])
        e.addAddend(-1, x)
        nnet.addEquation(e)

        ###   y = out_0 - out_?
        y = nnet.getNewVariable()
        f = MarabouUtils.Equation()
        f.setScalar(0)
        f.addAddend(1, y)
        f.addAddend(1, x)
        nnet.addEquation(f)

        ###   r1 = max(0, out_? - out_0)
        r1 = nnet.getNewVariable()
        nnet.addRelu(x,r1)

        ###   r2 = max(0, out_0 - out_?)
        r2 = nnet.getNewVariable()
        nnet.addRelu(y, r2)

        ###   r = |out_0 - output_?|
        r = nnet.getNewVariable()
        c = MarabouUtils.Equation()
        c.addAddend(1,r1)
        c.addAddend(1,r2)
        c.addAddend(-1,r)
        c.setScalar(0)
        nnet.addEquation(c)
        rs.add(r)

    ###   m = max class score difference
    m = nnet.getNewVariable()
    nnet.addMaxConstraint(rs, m)

    ###   max class score difference at least epsilon to break robustness
    nnet.setLowerBound(m, epsilon)


    return nnet.solve()

def binSearch(nnet_file_name, epsilon, low = 0.0, high=.50):
    while low < (high - .001):
        print(high)
        print(low)
        mid = (high+low)/2
        print(mid)
        nnet = Marabou.read_nnet(nnet_file_name)
# #         nnet_copy = copy.deepcopy(nnet)
        s = solve(nnet, epsilon, mid)
#         s = solve(nnet_copy, epsilon, mid)
        if len(s[0]) == 0: ###UNSAT
            low = mid
        else:              ###SAT
            high = mid
    return mid

d_opt = binSearch(nnet_file_name, .5, .0390625, .046875)
print(d_opt)
