/*********************                                                        */
/*! \file SymbolicBoundTightener.h
 ** \verbatim
 ** Top contributors (to current version):
 **   Guy Katz
 ** This file is part of the Marabou project.
 ** Copyright (c) 2017-2019 by the authors listed in the file AUTHORS
 ** in the top-level source directory) and their institutional affiliations.
 ** All rights reserved. See the file COPYING in the top-level source
 ** directory for licensing information.\endverbatim
 **
 ** [[ Add lengthier description here ]]

**/

#ifndef __SymbolicBoundTightener_h__
#define __SymbolicBoundTightener_h__

#include "MString.h"
#include "Map.h"

// Todo: remove this include later
#include "ReluConstraint.h"

/*
  A utility class for performing symbolic bound tightening.
  It currently makes the following assumptions:

    1. The network only has ReLU activations functions
    2. The network is fully connected
    3. An external caller has stored the weights and topology
*/

class SymbolicBoundTightener
{
public:
    struct WeightMatrix
    {
        double *_positiveValues;
        double *_negativeValues;
        unsigned _rows;
        unsigned _columns;
    };

    struct NodeIndex
    {
        NodeIndex()
            : _layer( 0 )
            , _neuron( 0 )
        {
        }

        NodeIndex( unsigned layer, unsigned neuron )
            : _layer( layer )
            , _neuron( neuron )
        {
        }

        unsigned _layer;
        unsigned _neuron;

        bool operator<( const NodeIndex &other ) const
        {
            if ( _layer != other._layer )
                return _layer < other._layer;

            return _neuron < other._neuron;
        }
    };

    SymbolicBoundTightener();
    ~SymbolicBoundTightener();

    /*
      Initialization methods for reporting the topology of the network:

      - Number of layers
      - Layer sizes
      - Allocating the required internal memory
      - Biases and weights
      - Lower and upper bounds for input neurons
    */
    void setNumberOfLayers( unsigned layers );
    void setLayerSize( unsigned layer, unsigned layerSize );
    void allocateWeightAndBiasSpace();
    void setBias( unsigned layer, unsigned neuron, double bias );
    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight );
    void setInputLowerBound( unsigned neuron, double bound );
    void setInputUpperBound( unsigned neuron, double bound );

    /*
      Setting the connections between the network topology and the simplex variables
    */
    void setReluBVariable( unsigned layer, unsigned neuron, unsigned b );
    void setReluFVariable( unsigned layer, unsigned neuron, unsigned f );

    NodeIndex nodeIndexFromB( unsigned b ) const;
    const Map<NodeIndex, unsigned> &getNodeIndexToFMapping() const;

    void updateVariableIndices( const Map<unsigned, unsigned> &oldIndexToNewIndex,
                                const Map<unsigned, unsigned> &mergedVariables,
                                const Map<unsigned, double> &fixedVariableValues );

    /*
      Report that a ReLU constraint has become permanently fixed (i.e., at decision level 0)
    */
    void setEliminatedRelu( unsigned layer, unsigned neuron, ReluConstraint::PhaseStatus status );

    /*
      Prior to running, we can use these methods to report any ReLUs that have becomes
      fixed, or to clear previously-reported information
    */
    void setReluStatus( unsigned layer, unsigned neuron, ReluConstraint::PhaseStatus status );
    void clearReluStatuses();

    /*
      Running the tool, with or without linear concertization
    */
    void run();
    void run( bool useLinearConcretization );

    /*
      After running the tools, these methods will extract the discovered
      bounds for every neuron
    */
    double getLowerBound( unsigned layer, unsigned neuron ) const;
    double getUpperBound( unsigned layer, unsigned neuron ) const;

    /*
      Duplicate the tightener
    */
    void storeIntoOther( SymbolicBoundTightener &other ) const;

private:
    // The number of layers and their sizes
    unsigned _numberOfLayers;
    unsigned *_layerSizes;
    unsigned _inputLayerSize;
    unsigned _maxLayerSize;

    // The network's weights and biases
    double **_biases;
    WeightMatrix *_weights;

    // Lower and upper bounds for input neurons
    Map<unsigned,double> _inputLowerBounds;
    Map<unsigned,double> _inputUpperBounds;

    // Lower and upper bounds for internal neurons
    double **_lowerBounds;
    double **_upperBounds;

    // Mapping from ReLUs to simplex variables
    Map<NodeIndex, unsigned> _nodeIndexToBVariable;
    Map<NodeIndex, unsigned> _nodeIndexToFVariable;
    Map<unsigned, NodeIndex> _bVariableToNodeIndex;

    // Information about the phase statuses of ReLU nodes
    Map<NodeIndex, ReluConstraint::PhaseStatus> _nodeIndexToReluState;
    Map<NodeIndex, ReluConstraint::PhaseStatus> _nodeIndexToEliminatedReluState;

    // To account for input variable renaming as part of preprocessing
    Map<unsigned, unsigned> _inputNeuronToIndex;

    /*
      Work space for the bound computation
    */
    double *_currentLayerLowerBounds;
    double *_currentLayerUpperBounds;
    double *_currentLayerLowerBias;
    double *_currentLayerUpperBias;

    double *_previousLayerLowerBounds;
    double *_previousLayerUpperBounds;
    double *_previousLayerLowerBias;
    double *_previousLayerUpperBias;

    void freeMemoryIfNeeded();
    static void log( const String &message );
};

#endif // __SymbolicBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
