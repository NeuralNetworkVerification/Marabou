#ifndef READ_NET_H
#define READ_NET_H

#include <torch/torch.h>
#include <vector>
#include <string>

std::vector<std::string> split_line(const std::string& line);

void load_nnet(const std::string& filename, std::vector<int>& layers, std::vector<std::string>& activations, 
               std::vector<std::vector<std::vector<float>>>& weights, std::vector<std::vector<float>>& biases);

struct CustomDNN : torch::nn::Module {
    std::vector<torch::nn::Linear> linear_layers;
    std::vector<std::string> activations;

    CustomDNN(const std::vector<int>& layer_sizes, const std::vector<std::string>& activation_functions, 
                  const std::vector<std::vector<std::vector<float>>>& weights, const std::vector<std::vector<float>>& biases);

    torch::Tensor forward(torch::Tensor x);
};
TORCH_MODULE(CustomDNN);

#endif // READ_NET_H
