import warnings
warnings.filterwarnings('ignore',category=FutureWarning)
warnings.filterwarnings('ignore',category=DeprecationWarning)

import sys
sys.path.append('.')

import unittest
from maraboupy import Marabou
from maraboupy import MarabouCore
import numpy as np

## Global settings
OPT = MarabouCore.Options()      # Define options 
OPT._verbosity = 0               # Turn off printing
TOL = 1e-5                       # Set tolerance for checking Marabou evaluations
FOLDER = "resources/onnx/" # Folder for test networks

class Test_ONNX(unittest.TestCase):
    
    def test_fc1(self):
        """
        Test a simple fully-connected neural network
        Uses Gemm, ReLU, and Identity layers
        """
        # Load network
        filename = FOLDER + "fc1.onnx"
        network = Marabou.read_onnx(filename)
        
        # Evaluate example point using both Marabou and ONNX
        testInput = np.ones(network.inputVars[0].shape)
        marabouEval = network.evaluateWithMarabou([testInput], options = OPT, filename = "").flatten()
        onnxEval = network.evaluateWithoutMarabou([testInput]).flatten()
        
        # Assert that both evaluations are the same within the set tolerance
        self.assertTrue(max(abs(marabouEval.flatten() - onnxEval.flatten())) < TOL)
        
if __name__ == '__main__':
    unittest.main()
