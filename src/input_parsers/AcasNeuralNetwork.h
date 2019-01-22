/*********************                                                        */
/*! \file AcasNeuralNetwork.h
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

#ifndef __AcasNeuralNetwork_h__
#define __AcasNeuralNetwork_h__

#include "AcasNnet.h"
#include "MString.h"
#include "Vector.h"
#include <cassert>
#include <iomanip>
#include <sstream>

class AcasNeuralNetwork
{
public:
    /*
      Parse a neural network stored in a file.
    */
    AcasNeuralNetwork( const String &path );
    ~AcasNeuralNetwork();

    /*
      Returns the number of layers in the network.
    */
    int getNumLayers() const;

    /*
      Returns the size of a specified layer.
    */
    unsigned getLayerSize( unsigned layer ) const;

    /*
      Returns the weight of the edge between two neurons.
    */
    double getWeight( int sourceLayer, int sourceNeuron, int targetNeuron );
    String getWeightAsString( int sourceLayer, int sourceNeuron, int targetNeuron );

    /*
      Returns the bias of a given neuron.
    */
    double getBias( int layer, int neuron );
    String getBiasAsString( int layer, int neuron );

    /*
      Evaluate the network for a given vector of inputs.
    */
    void evaluate( const Vector<double> &inputs, Vector<double> &outputs, unsigned outputSize ) const;

    /*
      Returns the input range [min, max] for the input node specified by index.
    */
    void getInputRange( unsigned index, double &min, double &max );

private:
    AcasNnet *_network;
};

#endif // __AcasNeuralNetwork_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
