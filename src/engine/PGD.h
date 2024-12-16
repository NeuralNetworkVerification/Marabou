#ifdef BUILD_TORCH
#ifndef __PGD_h__
#define __PGD_h__
#undef Warning
#include <torch/torch.h>
#include "CustomDNN.h"
#include "Options.h"

#include <TimeUtils.h>
#define PGD_LOG( x, ... ) LOG( GlobalConfiguration::CUSTOM_DNN_LOGGING, "PGD: %s\n", x )



/*
  PGDAttack class implements the Projected Gradient Descent method for generating
  adversarial examples for the network.
  The model aim to find valid input to the network that result in outputs within output bounds
  (SAT).
*/
class PGDAttack
{
public:
    enum {
        MICROSECONDS_TO_SECONDS = 1000000,
    };
    PGDAttack( NLR::NetworkLevelReasoner *networkLevelReasoner );
    ~PGDAttack();

    /*
    Search for an adversarial example. Returns true if successful.
  */
    bool runAttack();
    double getAssignment( int index );


private:
    NLR::NetworkLevelReasoner *networkLevelReasoner;
    torch::Device _device;
    /*
     The maximum permissible perturbation for each variable in the input vector, Defining
     the scope of allowable changes to the input during the attack.
     */
    torch::Tensor _epsilon;
    torch::Tensor _inputExample;
    std::unique_ptr<CustomDNN> _model;
    unsigned _iters;
    unsigned _restarts;
    unsigned _inputSize;
    std::pair<Vector<double>, Vector<double>> _inputBounds;
    std::pair<Vector<double>, Vector<double>> _outputBounds;
    Map<unsigned, double> _assignments;
    double *_adversarialInput;
    double *_adversarialOutput;

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
    static void printValue( double value );
};


#endif
#endif