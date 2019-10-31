import sys
import warnings
import numpy as np

sys.path.append('/home/haozewu/Projects/marabou-dev/Marabou/')

from maraboupy import Marabou

nnet_file_name = "Marabou/src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"

net1 = Marabou.read_nnet(nnet_file_name)
net1.setLowerBound(net1.outputVars[0][0], .5)

options = Marabou.createOptions(dnc=True, verbosity=0, initialDivides=2);
vals1, stats1 = net1.solve(options=options)
