/*********************                                                        */
/*! \file NetworkLevelReasoner.h
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

#ifndef __NetworkLevelReasoner_h__
#define __NetworkLevelReasoner_h__

#include "Map.h"

/*
  A class for performing operations that require knowledge of network
  level structure and topology.
*/

class NetworkLevelReasoner
{
public:
    NetworkLevelReasoner();
    ~NetworkLevelReasoner();

    /*
      Interface methods for populating the network: settings its
      number of layers and the layer sizes, kinds of activation
      functions, weights and biases, etc.
    */
    enum ActivationFunction {
        ReLU,
    };

    void setNumberOfLayers( unsigned numberOfLayers );
    void setLayerSize( unsigned layer, unsigned size );
    void allocateWeightMatrices();
    void setNeuronActivationFunction( unsigned layer, unsigned neuron, ActivationFunction activationFuction );
    void setWeight( unsigned sourceLayer, unsigned sourceNeuron, unsigned targetNeuron, double weight );
    void setBias( unsigned layer, unsigned neuron, double bias );

    /*
      Interface methods for performing operations on the network.
    */
    void evaluate( double *input, double *output ) const;

    /*
      Duplicate the reasoner
    */
    void storeIntoOther( NetworkLevelReasoner &other ) const;

private:
    struct Index
    {
        Index( unsigned layer, unsigned neuron )
            : _layer( layer )
            , _neuron( neuron )
        {
        }

        bool operator<( const Index &other ) const
        {
            if ( _layer < other._layer )
                return true;
            if ( _layer > other._layer )
                return false;

            return _neuron < other._neuron;
        }

        unsigned _layer;
        unsigned _neuron;
    };

    unsigned _numberOfLayers;
    Map<unsigned, unsigned> _layerSizes;
    Map<Index, ActivationFunction> _neuronToActivationFunction;
    double **_weights;
    Map<Index, double> _bias;

    unsigned _maxLayerSize;

    double *_work1;
    double *_work2;

    void freeMemoryIfNeeded();
};

#endif // __NetworkLevelReasoner_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
