#ifndef __CWATTACK_H__
#define __CWATTACK_H__
#ifdef BUILD_TORCH

#include "CustomDNN.h"
#include "InputQuery.h"
#include "Options.h"

#include <TimeUtils.h>
#include <cmath>
#include <torch/torch.h>

#define CW_LOG( x, ... ) MARABOU_LOG( GlobalConfiguration::CW_LOGGING, "CW: %s\n", x )

/**
  CWAttack implements the Carlini–Wagner L2 adversarial attack,
  optimizing min ||δ||_2^2 + c * specLoss(x0 + δ) with Adam and restarts.
*/
class CWAttack
{
public:
    enum {
        MICROSECONDS_TO_SECONDS = 1000000
    };
    CWAttack( NLR::NetworkLevelReasoner *networkLevelReasoner );
    ~CWAttack();

    /**
      Runs the CW attack. Returns true if a valid adversarial example is found.
    */
    bool runAttack();
    double getAssignment( int index );

private:
    NLR::NetworkLevelReasoner *networkLevelReasoner;
    torch::Device _device;
    std::unique_ptr<NLR::CustomDNN> _model;

    unsigned _inputSize;
    unsigned _iters;
    unsigned _restarts;
    double _specLossWeight;

    std::pair<Vector<double>, Vector<double>> _inputBounds;
    std::pair<Vector<double>, Vector<double>> _outputBounds;
    torch::Tensor _inputLb;
    torch::Tensor _inputUb;
    torch::Tensor _x0;

    Map<unsigned, double> _assignments;
    double *_adversarialInput;
    double *_adversarialOutput;

    void getBounds( std::pair<Vector<double>, Vector<double>> &bounds, signed type ) const;
    std::pair<torch::Tensor, torch::Tensor> generateSampleAndEpsilon();
    torch::Tensor calculateLoss( const torch::Tensor &predictions );
    std::pair<torch::Tensor, torch::Tensor> findAdvExample();
    static bool isWithinBounds( const torch::Tensor &sample,
                                const std::pair<Vector<double>, Vector<double>> &bounds );
    static void printValue( double value );
};

#endif // BUILD_TORCH
#endif // __CWATTACK_H__
