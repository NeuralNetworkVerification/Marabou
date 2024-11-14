#ifndef __CustomDNN_h__
#define __CustomDNN_h__

#include "NetworkLevelReasoner.h"

#include <vector>
#define CUSTOM_DNN_LOG( x, ... ) MARABOU_LOG( GlobalConfiguration::CUSTOM_DNN_LOGGING, "customDNN: %s\n", x )
#undef Warning
#include <torch/torch.h>

/*
  Custom differentiation function for max pooling, implementing the forward and backward propagation for
  the max pooling operation according to each variable's source layer as defined in the nlr.
*/
class CustomMaxPoolFunction : public torch::autograd::Function<CustomMaxPoolFunction>
{
public:
    static torch::Tensor forward( torch::autograd::AutogradContext *ctx,
                                  torch::Tensor x,
                                  const NLR::NetworkLevelReasoner *nlr,
                                  unsigned layerIndex );

    static std::vector<torch::Tensor> backward( torch::autograd::AutogradContext *ctx,
                                                std::vector<torch::Tensor> grad_output );
};

class CustomMaxPool : public torch::nn::Module
{
public:
    CustomMaxPool( const NLR::NetworkLevelReasoner *nlr, unsigned layerIndex );
    torch::Tensor forward( torch::Tensor x ) const;

private:
    const NLR::NetworkLevelReasoner *_networkLevelReasoner;
    unsigned _maxLayerIndex;
};

/*
 torch implementation of the network according to the nlr.
 */
class CustomDNN : public torch::nn::Module
{
public:
    static void setWeightsAndBiases( torch::nn::Linear &linearLayer,
                                 const NLR::Layer *layer,
                                 unsigned sourceLayer,
                                 unsigned inputSize,
                                 unsigned outputSize );
    void weightedSum( unsigned i, const NLR::Layer *layer );
    explicit CustomDNN( const NLR::NetworkLevelReasoner *networkLevelReasoner );

    torch::Tensor forward( torch::Tensor x );
    const Vector<unsigned> &getLayerSizes() const;
private:
    const NLR::NetworkLevelReasoner *_networkLevelReasoner;
    Vector<unsigned> _layerSizes;
    Vector<torch::nn::ReLU> _reluLayers;
    Vector<torch::nn::LeakyReLU> _leakyReluLayers;
    Vector<torch::nn::Sigmoid> _sigmoidLayers;
    Vector<std::shared_ptr<CustomMaxPool>> _customMaxPoolLayers;
    Vector<torch::nn::Linear> _linearLayers;
    Vector<NLR::Layer::Type> _layersOrder;
    unsigned _numberOfLayers;
};


#endif // __CustomDNN_h__