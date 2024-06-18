#ifndef PGD_H
#define PGD_H

#include <torch/torch.h>  

#ifdef LOG
#undef LOG
#endif

#include "CustomDNN.h"

namespace attack {

torch::Tensor findDelta(CustomDNNImpl& model, const torch::Tensor& X, int y, float epsilon, float alpha, int num_iter, torch::Device device);
bool displayAdversarialExample(CustomDNNImpl& model, const torch::Tensor& input, int target, torch::Device device);

} // namespace attack

#endif // PGD_H
