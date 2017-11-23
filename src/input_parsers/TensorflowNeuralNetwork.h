/*********************                                                        */
/*! \file TensorflowNeuralNetwork.h
** \verbatim
** Top contributors (to current version):
**   Kyle Julian
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/


#ifndef __TensorflowNeuralNetwork_h__
#define __TensorflowNeuralNetwork_h__

#include "TfNnet.h"
#include "MString.h"
#include "Vector.h"
#include <cassert>
#include <iomanip>
#include <sstream>



class TensorflowNeuralNetwork
{
public:
    /*
      Parse a neural network stored in a file.
    */
    TensorflowNeuralNetwork( const String &path );
    ~TensorflowNeuralNetwork();

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

private:
    TfNnet *_network;
};

#endif // __AcasNeuralNetwork_h__

//
// Local Variables:
// compile-command: "make -C ../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//

