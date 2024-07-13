#ifndef PGD_H
#define PGD_H

#include "CustomDNN.h"
#undef Warning
#include <torch/torch.h>

constexpr float DEFAULT_ALPHA = 0.01;
constexpr float PANELTY = 0.1;
constexpr unsigned DEFAULT_NUM_ITER = 100;
constexpr unsigned DEFAULT_NUM_RESTARTS = 100;
constexpr unsigned INPUT = 0;
constexpr unsigned OUTPUT = 1;
constexpr unsigned ATTACK_EPSILON = 10.0f;

class PGDAttack {
public:
  PGDAttack(CustomDNNImpl &model, InputQuery &inputQuery);
  PGDAttack( CustomDNNImpl &model,
             InputQuery &inputQuery,
             double alpha,
             unsigned num_iter,
             unsigned num_restarts,
             torch::Device device,
             double lambdaPenalty );

  bool displayAdversarialExample();

private:
  CustomDNNImpl &model;
  InputQuery &inputQuery;
  torch::Device device;
  torch::Tensor epsilon;
  torch::Tensor originalInput;
  double alpha;
  unsigned num_iter;
  unsigned num_restarts;
  unsigned inputSize;
  double lambdaPenalty;
  std::pair<Vector<double>, Vector<double>> inputBounds;
  std::pair<Vector<double>, Vector<double>> outputBounds;

  torch::Tensor _findDelta();
  std::pair<torch::Tensor, torch::Tensor> _generateSampleAndEpsilon();
  bool _isWithinBounds( const torch::Tensor &sample, unsigned type );
  std::pair<Vector<double>, Vector<double>> _getBounds(unsigned type ) const;
  torch::Tensor _calculateLoss( const torch::Tensor &predictions );
  static torch::Device _getDevice();

};



#endif // PGD_H
