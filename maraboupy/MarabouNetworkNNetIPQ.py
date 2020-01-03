

import MarabouNetworkNNet
import numpy as np

class MarabouNetworkNNetIPQ(MarabouNetworkNNet.MarabouNetworkNNet):
    """
    Class that implements a MarabouNetwork from an NNet file.
    """
    def __init__ (self, filename, perform_sbt=False, compute_ipq = False):
        """
        Constructs a MarabouNetworkNNet object from an .nnet file.

        Args:
            filename: path to the .nnet file.
        Attributes:
            ipq             an Input Query object comtaining the Input Query corresponding to the network

            numLayers        (int) The number of layers in the network
            layerSizes       (list of ints) Layer sizes.
            inputSize        (int) Size of the input.
            outputSize       (int) Size of the output.
            maxLayersize     (int) Size of largest layer.
            inputMinimums    (list of floats) Minimum value for each input.
            inputMaximums    (list of floats) Maximum value for each input.
            inputMeans       (list of floats) Mean value for each input.
            inputRanges      (list of floats) Range for each input
            weights          (list of list of lists) Outer index corresponds to layer
                                number.
            biases           (list of lists) Outer index corresponds to layer number.
            sbt              The SymbolicBoundTightener object

            inputVars
            b_variables
            f_variables
            outputVars

        Attributes from parent (MarabouNetwork)

            self.numVars
            self.equList = []
            self.reluList = []
            self.maxList = []
            self.varsParticipatingInConstraints = set()
            self.lowerBounds = dict()
            self.upperBounds = dict()
            self.inputVars = []
            self.outputVars = np.array([])


        """
        super().__init__(filename,perform_sbt)
        if compute_ipq:
            self.ipq = self.getMarabouQuery()
        else:
            self.ipq = MarabouCore.InputQuery()


    def computeIPQ(self):
        self.ipq = self.getMarabouQuery()


    def readProperty(self,filename):
        PropertyParser.parse(filename,self.ipq)


    # Re-tightens bounds on variables from the Input Query
    def tightenBounds(self):
        self.tightenInputBounds()
        self.tightenOutputBounds()
        self.tighten_fBounds()
        self.tighten_bBounds()

    def tightenInputBounds(self):
        for var in self.inputVars:
             true_lower_bound = ipq.getLowerBound(var)
             true_upper_bound = ipq.getUpperBound(var)
             if self.lowerBounds(var) < true_lower_bound:
                 self.setLowerBounds(var,true_lower_bound)
                 print("Adjusting lower bound for input variable",var,"to be",true_lower_bound)
             if self.upperBounds(var) > true_upper_bound:
                 self.setUpperBounds(var,true_upper_bound)
                 print("Adjusting upper bound for input variable",var,"to be",true_upper_bound)








