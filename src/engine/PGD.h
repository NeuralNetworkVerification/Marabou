#ifndef __PGD_h__
#define __PGD_h__

#include "CustomDNN.h"
#include "GlobalConfiguration.h"

#undef Warning
#include <torch/torch.h>

constexpr unsigned DEFAULT_NUM_ITER = 100;
constexpr unsigned DEFAULT_NUM_RESTARTS = 3;
#define PGD_LOG( x, ... ) MARABOU_LOG( GlobalConfiguration::CUSTOM_DNN_LOGGING, "PGD: %s\n", x )

/*
  PGDAttack class implements the Projected Gradient Descent method for generating
  adversarial examples for the network.
  The model aim to find valid input to the network that result in outputs within output bounds
  (SAT).
*/
class PGDAttack
{
public:
    PGDAttack( NLR::NetworkLevelReasoner *networkLevelReasoner );
    /*
    Search for an adversarial example. Returns true if successful.
  */
    bool hasAdversarialExample();


private:
    NLR::NetworkLevelReasoner *networkLevelReasoner;
    torch::Device _device;
    /*
     The maximum permissible perturbation for each variable in the input vector, Defining
     the scope of allowable changes to the input during the attack.
     */
    torch::Tensor _epsilon;
    torch::Tensor _inputExample;
    CustomDNN _model;
    unsigned _iters;
    unsigned _restarts;
    unsigned _inputSize;
    std::pair<Vector<double>, Vector<double>> _inputBounds;
    std::pair<Vector<double>, Vector<double>> _outputBounds;
    Map<unsigned, double> _assignments;
    double *_adversarialInput;
    double *_adversarialOutput;
    double getAssignment( int index );
    /*
     Iteratively generates and refines adversarial examples, aiming to discover inputs that lead to
     predictions outside the defined bounds, using gradient-based optimization.
     */
    std::pair<torch::Tensor, torch::Tensor> findAdvExample();
    /*
    Generates a valid sample input and epsilon tensor for initializing the attack,
   */
    std::pair<torch::Tensor, torch::Tensor> generateSampleAndEpsilon();

    /*
      Checks if a given sample is within the valid bounds
    */
    static bool isWithinBounds( const torch::Tensor &sample,
                                const std::pair<Vector<double>, Vector<double>> &bounds );
    /*
     Get the bounds of each neuron in the network defining in the nlr.
     */
    void getBounds( std::pair<Vector<double>, Vector<double>> &bounds, signed type ) const;
    /*
     Calculates the loss based on how far the network's outputs deviate from its bounds.
     Assigns a higher loss to predictions that fall outside bounds.

  */
    torch::Tensor calculateLoss( const torch::Tensor &predictions );
    static torch::Device getDevice();
};


#endif // __PGD_h__
