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

#include "Map.h"

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

    void dump() const;

    // Extract the discovered bounds
    double getLowerBound( unsigned layer, unsigned neuron ) const;
    double getUpperBound( unsigned layer, unsigned neuron ) const;

private:
    unsigned _numberOfLayers;
    unsigned *_layerSizes;

    Map<unsigned,unsigned> _inputLowerBounds;
    Map<unsigned,unsigned> _inputUpperBounds;

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

    double *_previousLayerLowerBounds;
    double *_previousLayerUpperBounds;
};

#endif // __SymbolicBoundTightener_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
