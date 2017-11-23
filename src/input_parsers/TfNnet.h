/*********************                                                        */
/*! \file TfNnet.h
** \verbatim
** Top contributors (to current version):
**   Kyle Julian
** This file is part of the Marabou project.
** Copyright (c) 2016-2017 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved. See the file COPYING in the top-level source
** directory for licensing information.\endverbatim
**/

#ifndef __TfNnet_h__
#define __TfNnet_h__

#include "tensorflow/core/public/session.h"
#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/graph_def_util.h"
#include "tensorflow/core/protobuf/meta_graph.pb.h"

using namespace tensorflow;

class TfNnet {
public:
    std::vector<Eigen::Tensor<float,2>> weights;
    std::vector<Eigen::Tensor<float,1>> biases;
    std::vector<std::string> activations;
    int num_inputs;
    int num_outputs;
    int num_layers;
    int max_layer;
    int *layer_sizes;
    Session *session;
    std::string output_name;
    std::vector<string> input_names;
};

TfNnet *load_network(const char *filename);
int num_inputs(TfNnet *network);
int num_outputs(TfNnet *network);
int num_layers(TfNnet *network);
int printVariables(std::map<std::string, Eigen::Tensor<float,2>>& varValuesMatrix, std::map<string, Eigen::Tensor<float,1>>& varValuesVector);
string cleanName(string name);
int printVariables(int num_inputs, std::vector<Eigen::Tensor<float,2>>& weights, std::vector<Eigen::Tensor<float,1>>& biases, std::vector<string> activations);
int convertTensorData(Session* session, std::vector<string> varNames, std::map<string, Eigen::Tensor<float,2>>& varValuesMatrix, std::map<string, Eigen::Tensor<float,1>>& varValuesVector);
void destroy_network(TfNnet *network);
int evaluate_network(TfNnet *network, double *input, double *output, bool normInputs, bool normOutputs);
#endif // __TfNnet_h__

//
// Local Variables:
// compile-command: "make -C ../../.. "
// tags-file-name: "../../TAGS"
// c-basic-offset: 4
// End:
//
