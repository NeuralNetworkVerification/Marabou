#include "read_net.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> split_line(const std::string& line) {
    std::istringstream input_stream(line);
    std::vector<std::string> tokens;
    std::string token;
    while (input_stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

void load_nnet(const std::string& filename, std::vector<int>& layers, std::vector<std::string>& activations, 
               std::vector<std::vector<std::vector<float>>>& weights, std::vector<std::vector<float>>& biases) {
    std::ifstream input_file(filename);
    if (!input_file) {
        std::cerr << "Could not open file: " << filename << std::endl;
        exit(1);
    }

    std::string line;
    std::getline(input_file, line);
    int num_layers = std::stoi(line);

    std::getline(input_file, line);
    std::vector<std::string> layer_sizes_str = split_line(line);
    for (const auto& size_str : layer_sizes_str) {
        layers.push_back(std::stoi(size_str));
    }

    std::getline(input_file, line);
    activations = split_line(line);

    // Skip input mins, max, means, ranges
    for (int i = 0; i < 6; ++i) {
        std::getline(input_file, line);
    }

    for (int i = 0; i < num_layers; ++i) { 
        std::vector<std::vector<float>> layer_weights;
        int weight_next_layer = layers[i + 1]; 
        int weight_cur_layer = layers[i];    

        for (int j = 0; j < weight_next_layer; ++j) {
            std::getline(input_file, line);
            std::vector<std::string> weights_next_str = split_line(line);
            if (weights_next_str.size() != weight_cur_layer) {
                std::cerr << "Error: Expected " << weight_cur_layer << " columns but got " << weights_next_str.size() << std::endl;
                exit(1);
            }
            std::vector<float> weights_next;
            for (const auto& weight_str : weights_next_str) {
                weights_next.push_back(std::stof(weight_str));
            }
            layer_weights.push_back(weights_next);
        }
        weights.push_back(layer_weights);

        std::getline(input_file, line);
        std::vector<std::string> biases_next_str = split_line(line);
        if (biases_next_str.size() != weight_next_layer) {
            std::cerr << "Error: Expected " << weight_next_layer << " biases but got " << biases_next_str.size() << std::endl;
            exit(1);
        }
        std::vector<float> layer_biases;
        for (const auto& bias_str : biases_next_str) {
            layer_biases.push_back(std::stof(bias_str));
        }
        biases.push_back(layer_biases);
    }
    input_file.close();
    std::cout<<"done loading the network!"<< std::endl;
}

CustomDNN::CustomDNN(const std::vector<int>& layer_sizes, const std::vector<std::string>& activation_functions, 
                             const std::vector<std::vector<std::vector<float>>>& weights, const std::vector<std::vector<float>>& biases) {
    for (size_t i = 0; i < layer_sizes.size() - 1; ++i) {
        auto layer = register_module("fc" + std::to_string(i), torch::nn::Linear(layer_sizes[i], layer_sizes[i+1]));
        linear_layers.push_back(layer);
        activations.push_back(activation_functions[i]);
        
        std::vector<float> flattened_weights;
        for (const auto& row : weights[i]) {
            flattened_weights.insert(flattened_weights.end(), row.begin(), row.end());
        }

        torch::Tensor weight_tensor = torch::tensor(flattened_weights, torch::kFloat).view({layer_sizes[i+1], layer_sizes[i]});
        torch::Tensor bias_tensor = torch::tensor(biases[i], torch::kFloat);

        layer->weight.data().copy_(weight_tensor);
        layer->bias.data().copy_(bias_tensor);
    }
}

torch::Tensor CustomDNN::forward(torch::Tensor x) {
    for (size_t i = 0; i < linear_layers.size(); ++i) {
        x = linear_layers[i]->forward(x);
        if (activations[i] == "ReLU") {
            x = torch::relu(x);
        } else if (activations[i] == "Sigmoid") {
            x = torch::sigmoid(x);
        } else if (activations[i] == "Tanh") {
            x = torch::tanh(x);
        } else if (activations[i] == "Softmax") {
            x = torch::softmax(x, 1);
        }
    }
    return x;
}
