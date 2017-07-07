/*********************                                                        */
/*! \file AcasNnet.h
** \verbatim
** Top contributors (to current version):
**   Kyle Julian
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __AcasNnet_h__
#define __AcasNnet_h__

//Neural Network Struct
class AcasNnet {
public:
    int symmetric;     //1 if network is symmetric, 0 otherwise
    int numLayers;     //Number of layers in the network
    int inputSize;     //Number of inputs to the network
    int outputSize;    //Number of outputs to the network
    int maxLayerSize;  //Maximum size dimension of a layer in the network
    int *layerSizes;   //Array of the dimensions of the layers in the network

    double *mins;      //Minimum value of inputs
    double *maxes;     //Maximum value of inputs
    double *means;     //Array of the means used to scale the inputs and outputs
    double *ranges;    //Array of the ranges used to scale the inputs and outputs
    double ****matrix; //4D jagged array that stores the weights and biases
                       //the neural network.
    double *inputs;    //Scratch array for inputs to the different layers
    double *temp;      //Scratch array for outputs of different layers
};

//Functions Implemented
extern "C" AcasNnet *load_network(const char *filename);
extern "C" int   num_inputs(void *network);
extern "C" int   num_outputs(void *network);
extern "C" int   evaluate_network(void *network, double *input, double *output, bool normalizeInput, bool normalizeOutput);
extern "C" void  destroy_network(AcasNnet *network);

#endif // __AcasNnet_h__

//
// Local Variables:
// compile-command: "make -C .. "
// tags-file-name: "../TAGS"
// c-basic-offset: 4
// End:
//
