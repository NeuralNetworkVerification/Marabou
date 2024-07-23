#ifndef CUSTOM_DNN_H
#define CUSTOM_DNN_H

#include "NetworkLevelReasoner.h"

#include <string>
#include <vector>

#undef Warning
#include <torch/torch.h>

class CustomDNNImpl : public torch::nn::Module
{
public:
    enum Type {
        INPUT,
        LINEAR,
        ACTIVATION,
    };
    Vector<unsigned> layerSizes;
    Vector<NLR::Layer::Type> activations;
    Vector<Vector<Vector<float>>> weights;
    Vector<std::vector<float>> biases;
    Vector<torch::nn::Linear> linearLayers;
    Vector<unsigned> layersOrder;
    unsigned numberOfLayers;

    explicit CustomDNNImpl( const NLR::NetworkLevelReasoner *networkLevelReasoner );

    torch::Tensor forward( torch::Tensor x );
};

#endif