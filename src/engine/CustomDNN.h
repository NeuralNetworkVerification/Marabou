#ifndef CUSTOM_DNN_H
#define CUSTOM_DNN_H

#include "NetworkLevelReasoner.h"
#include <vector>

#undef Warning
#include <torch/torch.h>

class CustomMaxPool : public torch::nn::Module {
public:
    CustomMaxPool(const NLR::NetworkLevelReasoner* nlr, unsigned layerIndex);
    torch::Tensor forward(torch::Tensor x) const;

private:
    const NLR::NetworkLevelReasoner* networkLevelReasoner;
    unsigned maxLayerIndex;
};

class CustomDNNImpl : public torch::nn::Module
{
public:

    const NLR::NetworkLevelReasoner* networkLevelReasoner;
    Vector<unsigned> layerSizes;
    Vector<torch::nn::ReLU> reluLayers;
    Vector<torch::nn::LeakyReLU> leakyReluLayers;
    Vector<torch::nn::Sigmoid> sigmoidLayers;
    Vector<std::shared_ptr<CustomMaxPool>> customMaxPoolLayers;
    Vector<NLR::Layer::Type> activations;
    Vector<Vector<Vector<float>>> weights;
    Vector<std::vector<float>> biases;
    Vector<torch::nn::Linear> linearLayers;
    Vector<NLR::Layer::Type> layersOrder;
    unsigned numberOfLayers;
    Vector<unsigned> maxLayerIndices;

    static void setWeightsAndBiases( torch::nn::Linear &linearLayer,
                              const NLR::Layer *layer,
                              unsigned sourceLayer,
                              unsigned inputSize,
                              unsigned outputSize );
    void weightedSum( unsigned i, const NLR::Layer *layer );
    explicit CustomDNNImpl( const NLR::NetworkLevelReasoner *networkLevelReasoner );

    torch::Tensor customMaxPool(unsigned max_layer_inxes, torch::Tensor x ) const;
    torch::Tensor forward( torch::Tensor x );
};


#endif