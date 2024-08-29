#ifndef PGD_H
#define PGD_H

#include "CustomDNN.h"
#undef Warning
#include <torch/torch.h>

constexpr float LR = 0.05;
constexpr unsigned DEFAULT_NUM_ITER = 350;
constexpr unsigned DEFAULT_NUM_RESTARTS = 2;
constexpr unsigned INPUT = 0;
constexpr unsigned OUTPUT = 1;
constexpr unsigned RANGE = 1000.0f;

class PGDAttack {
public:
  PGDAttack(CustomDNNImpl &model, const NLR::NetworkLevelReasoner* networkLevelReasoner);
  PGDAttack( CustomDNNImpl &model,
             const NLR::NetworkLevelReasoner* networkLevelReasoner,
             unsigned num_iter,
             unsigned num_restarts,
             torch::Device device);

  bool displayAdversarialExample();

private:
  CustomDNNImpl &model;
  const NLR::NetworkLevelReasoner* networkLevelReasoner;
  torch::Device device;
  torch::Tensor epsilon;
  torch::Tensor InputExample;
  unsigned iters;
  unsigned restarts;
  unsigned inputSize;
  std::pair<Vector<double>, Vector<double>> inputBounds;
  std::pair<Vector<double>, Vector<double>> outputBounds;

  std::pair <torch::Tensor, torch::Tensor> _findAdvExample();
  std::pair<torch::Tensor, torch::Tensor> _generateSampleAndEpsilon();
  static bool _isWithinBounds(const torch::Tensor& sample,
                                          const std::pair<Vector<double>, Vector<double>> &bounds);
  void _getBounds(std::pair<Vector<double>, Vector<double>> &bounds, signed type ) const;
  torch::Tensor _calculateLoss( const torch::Tensor &predictions );
  static torch::Device _getDevice();

};



#endif // PGD_H
