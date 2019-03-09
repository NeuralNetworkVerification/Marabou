import numpy as np
import sys
import copy
import os

from maraboupy import Marabou
from maraboupy import MarabouUtils

nnet_file_name = "../../src/input_parsers/acas_example/ACASXU_run2a_1_1_tiny_2.nnet"
nnet = Marabou.read_nnet(nnet_file_name)
i = 0
for f in os.listdir():
    if f[0] != '1':
        continue
    print(i)
    nnet.loadQuery(f)
