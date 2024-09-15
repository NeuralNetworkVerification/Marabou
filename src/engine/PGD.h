#ifndef __PGD_h__
#define __PGD_h__

#include "CustomDNN.h"

#undef Warning
#include <torch/torch.h>

constexpr unsigned DEFAULT_NUM_ITER = 50;
constexpr unsigned DEFAULT_NUM_RESTARTS = 4;
constexpr unsigned INPUT = 0;
constexpr unsigned OUTPUT = 1;
constexpr unsigned RANGE = 1000.0f;

class PGDAttack
{
public:
    PGDAttack( NLR::NetworkLevelReasoner *networkLevelReasoner );
    bool displayAdversarialExample();
    double getAssignment( int index );


private:
    NLR::NetworkLevelReasoner *networkLevelReasoner;
    torch::Device device;
    torch::Tensor epsilon;
    torch::Tensor InputExample;
    CustomDNNImpl model;
    unsigned iters;
    unsigned restarts;
    unsigned inputSize;
    std::pair<Vector<double>, Vector<double>> inputBounds;
    std::pair<Vector<double>, Vector<double>> outputBounds;
    Map<unsigned, double> assignments;
    double *_adversarialInput;
    double *_adversarialOutput;

    std::pair<torch::Tensor, torch::Tensor> _findAdvExample();
    std::pair<torch::Tensor, torch::Tensor> _generateSampleAndEpsilon();
    static bool _isWithinBounds( const torch::Tensor &sample,
                                 const std::pair<Vector<double>, Vector<double>> &bounds );
    void _getBounds( std::pair<Vector<double>, Vector<double>> &bounds, signed type ) const;
    torch::Tensor _calculateLoss( const torch::Tensor &predictions );
    static torch::Device _getDevice();
};


#endif // __PGD_h__
