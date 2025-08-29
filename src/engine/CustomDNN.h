#ifdef BUILD_TORCH
#ifndef _CustomDNN_h_
#define _CustomDNN_h_

#include "Layer.h"
#include "Vector.h"

#include <vector>

#undef Warning
#include <torch/torch.h>

#define CUSTOM_DNN_LOG( x, ... )                                                                   \
    MARABOU_LOG( GlobalConfiguration::CUSTOM_DNN_LOGGING, "customDNN: %s\n", x )

/*
  Custom differentiation function for ReLU, implementing the forward and backward propagation
  for the ReLU operation according to each variable's source layer as defined in the nlr.
*/
namespace NLR {
class CustomReluFunction : public torch::autograd::Function<CustomReluFunction>
{
public:
    static torch::Tensor forward( torch::autograd::AutogradContext *ctx,
                                  torch::Tensor x,
                                  const NetworkLevelReasoner *nlr,
                                  unsigned layerIndex );

    static std::vector<torch::Tensor> backward( torch::autograd::AutogradContext *ctx,
                                                std::vector<torch::Tensor> grad_output );
};

class CustomRelu : public torch::nn::Module
{
public:
    CustomRelu( const NetworkLevelReasoner *nlr, unsigned layerIndex );
    torch::Tensor forward( torch::Tensor x ) const;

private:
    const NetworkLevelReasoner *_networkLevelReasoner;
    unsigned _reluLayerIndex;
};

/*
  Custom differentiation function for max pooling, implementing the forward and backward propagation
  for the max pooling operation according to each variable's source layer as defined in the nlr.
*/
class CustomMaxPoolFunction : public torch::autograd::Function<CustomMaxPoolFunction>
{
public:
    static torch::Tensor forward( torch::autograd::AutogradContext *ctx,
                                  torch::Tensor x,
                                  const NetworkLevelReasoner *nlr,
                                  unsigned layerIndex );

    static std::vector<torch::Tensor> backward( torch::autograd::AutogradContext *ctx,
                                                std::vector<torch::Tensor> grad_output );
};

class CustomMaxPool : public torch::nn::Module
{
public:
    CustomMaxPool( const NetworkLevelReasoner *nlr, unsigned layerIndex );
    torch::Tensor forward( torch::Tensor x ) const;

private:
    const NetworkLevelReasoner *_networkLevelReasoner;
    unsigned _maxLayerIndex;
};

/*
 torch implementation of the network according to the nlr.
 */
class CustomDNN : public torch::nn::Module
{
public:
    static void setWeightsAndBiases( torch::nn::Linear &linearLayer,
                                     const Layer *layer,
                                     unsigned sourceLayer,
                                     unsigned inputSize,
                                     unsigned outputSize );
    void weightedSum( unsigned i, const Layer *layer );
    explicit CustomDNN( const NetworkLevelReasoner *networkLevelReasoner );
    torch::Tensor getLayerWeights( unsigned layerIndex ) const;
    torch::Tensor getLayerBias( unsigned layerIndex ) const;
    torch::Tensor forward( torch::Tensor x );
    const Vector<unsigned> &getLayerSizes() const;
    void getInputBounds( torch::Tensor &lbTensor, torch::Tensor &ubTensor ) const;
    std::vector<List<NeuronIndex>> getMaxPoolSources(const Layer* maxPoolLayer);
    Vector<torch::nn::Linear> getLinearLayers()
    {
        return _linearLayers;
    }
    Vector<Layer::Type> getLayersOrder() const
    {
        return _layersOrder;
    }
    Vector<Layer::Type> getLayersOrder()
    {
        return _layersOrder;
    }

    unsigned getNumberOfLayers() const
    {
        return _numberOfLayers;
    }

private:
    const NetworkLevelReasoner *_networkLevelReasoner;
    Vector<unsigned> _layerSizes;
    Vector<std::shared_ptr<CustomRelu>> _reluLayers;
    Vector<std::shared_ptr<CustomMaxPool>> _maxPoolLayers;
    Vector<torch::nn::Linear> _linearLayers;
    Vector<Layer::Type> _layersOrder;
    unsigned _numberOfLayers;
};
} // namespace NLR
#endif // _CustomDNN_h_
#endif