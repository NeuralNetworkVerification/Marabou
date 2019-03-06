import numpy as np
import sys
import copy

from maraboupy import Marabou
from maraboupy import MarabouUtils

nnet_file_name = "../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"
nnet = Marabou.read_nnet(nnet_file_name)
for i in range(0,18):
    nnet.loadQuery("cv_query"+str(i)+".txt")
