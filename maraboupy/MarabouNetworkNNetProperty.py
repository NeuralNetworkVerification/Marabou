import MarabouNetworkNNet
#import MarabouNetworkNNetExtendedParent
from Property import *



class MarabouNetworkNNetProperty(MarabouNetworkNNet.MarabouNetworkNNet):
    """
    Class that implements a MarabouNetwork from an NNet file with a property from a property file.
    """

    def __init__ (self, filename, property_filename="", perform_sbt=False, compute_ipq=False):
        """
        Constructs a MarabouNetworkNNet object from an .nnet file.
        Imports a property from a property file

        Args:
            filename: path to the .nnet file.
            property_filename: path to the property file
        Attributes:

            property         Property object

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
        super(MarabouNetworkNNetProperty,self).__init__(filename=filename,perform_sbt=perform_sbt)

        print('property filename = ', property_filename)

        self.property = Property(property_filename)



