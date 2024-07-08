#ifndef PGD_H
#define PGD_H

#include "CustomDNN.h"
#undef Warning
#include <torch/torch.h>

constexpr float DEFAULT_ALPHA = 0.01;
constexpr unsigned DEFAULT_NUM_ITER = 100;
constexpr unsigned DEFAULT_NUM_RESTARTS = 50;
constexpr torch::DeviceType DEFAULT_DEVICE_TYPE = torch::kCPU;

class PGDAttack {
public:
  PGDAttack(CustomDNNImpl &model, const Map<unsigned, double> &lowerBounds,
            const Map<unsigned, double> &upperBounds);
  PGDAttack(CustomDNNImpl &model, const Map<unsigned, double> &lowerBounds,
            const Map<unsigned, double> &upperBounds, double alpha, unsigned num_iter,
            unsigned num_restarts, torch::Device device);

  bool displayAdversarialExample();

private:
  CustomDNNImpl &model;
  const Map<unsigned, double> &lowerBounds;
  const Map<unsigned, double> &upperBounds;
  torch::Device device;
  torch::Tensor epsilon;
  torch::Tensor originalInput;
  double alpha;
  unsigned num_iter;
  unsigned num_restarts;
  unsigned inputSize;

  torch::Tensor findDelta(int originalPred);
  std::pair<torch::Tensor, torch::Tensor> generateSampleAndEpsilon();
};

#endif // PGD_H
