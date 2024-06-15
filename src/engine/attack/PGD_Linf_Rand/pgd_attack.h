#ifndef PGD_ATTACK_H
#define PGD_ATTACK_H

#include <torch/torch.h>
#include "read_net.h"

class PGD_Linf_Rand {
public:
    PGD_Linf_Rand(CustomDNN model, torch::Device device);
    bool main(std::string filename);

private:
    CustomDNN model;
    torch::Device device;
    torch::optim::SGD optimizer;
    torch::Tensor pgd_linf_rand(const torch::Tensor& X, int y, float epsilon, float alpha, int num_iter);
    bool display_adversarial_example(const torch::Tensor& input, int target);
};

#endif // PGD_ATTACK_H
