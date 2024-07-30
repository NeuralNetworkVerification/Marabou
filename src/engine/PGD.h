#ifndef PGD_H
#define PGD_H

#include "CustomDNN.h"
#undef Warning
#include <torch/torch.h>

constexpr float LR = 0.1;
constexpr float PANELTY = 10;
constexpr unsigned DEFAULT_NUM_ITER = 1000;
constexpr unsigned DEFAULT_NUM_RESTARTS = 5;
constexpr unsigned INPUT = 0;
constexpr unsigned OUTPUT = 1;
constexpr unsigned RANGE = 1000.0f;

class PGDAttack {
public:
  PGDAttack(CustomDNNImpl &model, const NLR::NetworkLevelReasoner* networkLevelReasoner);
  PGDAttack( CustomDNNImpl &model,
             const NLR::NetworkLevelReasoner* networkLevelReasoner,
             double alpha,
             unsigned num_iter,
             unsigned num_restarts,
             torch::Device device,
             double lambdaPenalty );

  bool displayAdversarialExample();

private:
  CustomDNNImpl &model;
  const NLR::NetworkLevelReasoner* networkLevelReasoner;
  torch::Device device;
  torch::Tensor epsilon;
  torch::Tensor originalInput;
  double learningRate;
  unsigned iters;
  unsigned restarts;
  unsigned inputSize;
  double penalty;
  std::pair<Vector<double>, Vector<double>> inputBounds;
  std::pair<Vector<double>, Vector<double>> outputBounds;

  std::pair <torch::Tensor, torch::Tensor> _findAdvExample();
  std::pair<torch::Tensor, torch::Tensor> _generateSampleAndEpsilon();
  bool _isWithinBounds( const torch::Tensor &sample, unsigned type );
  std::pair<Vector<double>, Vector<double>> _getBounds(unsigned type ) const;
  torch::Tensor _calculateLoss( const torch::Tensor &predictions );
  static torch::Device _getDevice();

};



#endif // PGD_H
