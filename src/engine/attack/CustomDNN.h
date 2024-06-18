#ifndef CUSTOM_DNN_H
#define CUSTOM_DNN_H

#include <torch/torch.h> 

#ifdef LOG
#undef LOG
#endif


#include <vector>
#include <string>
#include "NetworkLevelReasoner.h"

namespace attack {

class CustomDNNImpl : public torch::nn::Module {
public:
    std::vector<int> layerSizes;
    std::vector<std::string> activations;
    std::vector<std::vector<std::vector<float>>> weights;
    std::vector<std::vector<float>> biases;
    std::vector<torch::nn::Linear> linearLayers;

    CustomDNNImpl(const std::vector<int>& layer_sizes, const std::vector<std::string>& activationFunctions, 
                  const std::vector<std::vector<std::vector<float>>>& weights, const std::vector<std::vector<float>>& biases);
    CustomDNNImpl(const NLR::NetworkLevelReasoner& nlr);

    torch::Tensor forward(torch::Tensor x);
};

TORCH_MODULE(CustomDNN);

} 

#endif // CUSTOM_DNN_H