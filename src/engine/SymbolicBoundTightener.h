/*********************                                                        */
/*! \file SymbolicBoundTightener.h
** \verbatim
** Top contributors (to current version):
**   Duligur Ibeling
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __SymbolicBoundTightener_h__
#define __SymbolicBoundTightener_h__

#include "MString.h"
#include "Map.h"

// Hack: SBT should not know about ReLUs directly. Fix this.
#include "ReluConstraint.h"

/*
  A utility class for performing symbolic bound tightening.
  It currently makes the following assumptions:

    1. The network only has ReLU activations functions
    2. The network is fully connected
    3. An external caller has stored the weights and topology
*/

const double BOUND_ROUNDING_CONSTANT = 0.00000005;

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

    void setNumberOfLayers( unsigned layers );
    void setLayerSize( unsigned layer, unsigned layerSize );
    void allocateWeightAndBiasSpace();
    void setBias( unsigned layer, unsigned neuron, double bias );
    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight );

    void setInputLowerBound( unsigned neuron, double bound );
    void setInputUpperBound( unsigned neuron, double bound );

    void run();

    // Extract the discovered bounds
    double getLowerBound( unsigned layer, unsigned neuron ) const;
    double getUpperBound( unsigned layer, unsigned neuron ) const;

    // For informing the tightener about ReLUs that have become fixed
    void clearReluStatuses();
    void setReluStatus( unsigned layer, unsigned neuron, ReluConstraint::PhaseStatus status );

    static void log( const String &message );

    // This permanently sets a relu's status
    void setEliminatedRelu( unsigned layer, unsigned neuron, ReluConstraint::PhaseStatus status );

    void setReluBVariable( unsigned layer, unsigned neuron, unsigned b );
    NodeIndex nodeIndexFromB( unsigned b ) const;

    Map<NodeIndex, unsigned> _nodeIndexToVar; // For bound tightening

    void updateVariableIndex( unsigned oldIndex, unsigned newIndex );

private:
    unsigned _numberOfLayers;
    unsigned *_layerSizes;

    Map<unsigned,double> _inputLowerBounds;
    Map<unsigned,double> _inputUpperBounds;

    double **_biases;
    WeightMatrix *_weights;

    double **_lowerBounds;
    double **_upperBounds;

    void freeMemoryIfNeeded();

    unsigned _inputLayerSize;
    unsigned _maxLayerSize;

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

    Map<NodeIndex, unsigned> _nodeIndexToBVariable;
    Map<unsigned, NodeIndex> _bVariableToNodeIndex;

    Map<NodeIndex, ReluConstraint::PhaseStatus> _nodeIndexToReluState;
    Map<NodeIndex, ReluConstraint::PhaseStatus> _nodeIndexToEliminatedReluState;
};

#endif // __SymbolicBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
