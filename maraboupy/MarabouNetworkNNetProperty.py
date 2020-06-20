
'''
/* *******************                                                        */
/*! \file MarabouNetworkNNetProperty.py
 ** \verbatim
 ** Top contributors (to current version):
 ** Alex Usvyatsov
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** \brief
 ** This class extends MarabouNetworkNNet class
 ** Adds property (to be proven) as an attribute
 **
 ** [[ Add lengthier description here ]]
 **/
'''

try:
    import MarabouNetworkNNetExtended
    from Property import *
except ImportError:
    import maraboupy.MarabouNetworkNNetExtended
    from maraboupy.Property import *

class MarabouNetworkNNetProperty(MarabouNetworkNNetExtended.MarabouNetworkNNetExtended):
    """
    Class that implements a MarabouNetwork from an NNet file with a property from a property file.
    """

    def __init__ (self, filename="", property_filename="", normalize = False, compute_internal_ipq=False, use_nlr=False):
        """
        Constructs a MarabouNetworkNNetProperty object from an .nnet file.
        Imports a property from a property file

        Args:
            filename: path to the .nnet file.
            property_filename: path to the property file
        Attributes:

            property         Property object

        Attributes from MarabouNetworkNNet:

            numLayers        (int) The number of layers in the network
            layerSizes       (list of ints) Layer sizes.
            inputSize        (int) Size of the input.
            outputSize       (int) Size of the output.
            maxLayersize     (int) Size of largest layer.
            inputMinimums    (list of floats) Minimum value for each input.
            inputMaximums    (list of floats) Maximum value for each input.
            inputMeans       (list of floats) Mean value for each input.
            inputRanges      (list of floats) Range for each input
            outputMean       (float) Mean value of outputs
            outputRange      (float) Range of output values
            weights          (list of list of lists) Outer index corresponds to layer
                                number.
            biases           (list of lists) Outer index corresponds to layer number.

            inputVars
            b_variables
            f_variables
            outputVars

        Attributes from MarabouNetwork

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
        super(MarabouNetworkNNetProperty,self).__init__(filename=filename, normalize=normalize, use_nlr=use_nlr)

        print('property filename = ', property_filename)

        self.property = Property(property_filename)



    # def writePropertyToFile(self,property_filename: str):
    #     '''
    #         Write Property to file in the correct format
    #     '''
    #
    #     TO IMPLEMENT


