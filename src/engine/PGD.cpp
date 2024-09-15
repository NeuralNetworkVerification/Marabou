#include "PGD.h"
#include "InputQuery.h"

#include <iostream>

PGDAttack::PGDAttack( NLR::NetworkLevelReasoner *networkLevelReasoner )
    : networkLevelReasoner( networkLevelReasoner )
    , device( torch::kCPU )
    , model( CustomDNNImpl( networkLevelReasoner ) )
    , iters( DEFAULT_NUM_ITER )
    , restarts( DEFAULT_NUM_RESTARTS )
    , _adversarialInput( nullptr )
    , _adversarialOutput( nullptr )
{
    inputSize = model.layerSizes.first();
    _getBounds( inputBounds, INPUT );
    _getBounds( outputBounds, OUTPUT );
    std::pair<torch::Tensor, torch::Tensor> variables = _generateSampleAndEpsilon();
    InputExample = variables.first;
    epsilon = variables.second;
}

torch::Device PGDAttack::_getDevice()
{
    if ( torch::cuda::is_available() )
    {
        std::cout << "CUDA is available. Using GPU." << std::endl;
        return { torch::kCUDA };
    }
    else
    {
        std::cout << "CUDA is not available. Using CPU." << std::endl;
        return { torch::kCPU };
    }
}

void PGDAttack::_getBounds( std::pair<Vector<double>, Vector<double>> &bounds,
                            const signed type ) const
{
    unsigned layerIndex = type == INPUT ? 0 : networkLevelReasoner->getNumberOfLayers() - 1;
    const NLR::Layer *layer = networkLevelReasoner->getLayer( layerIndex );

    for ( unsigned neuron = 0; neuron < layer->getSize(); ++neuron )
    {
        bounds.first.append( layer->getLb( neuron ) );
        bounds.second.append( layer->getUb( neuron ) );
    }
}

std::pair<torch::Tensor, torch::Tensor> PGDAttack::_generateSampleAndEpsilon()
{
    Vector<double> sample( inputSize, 0.0f );
    Vector<double> epsilons( inputSize, 0.0f );

    for ( unsigned i = 0; i < inputSize; i++ )
    {
        double lower = inputBounds.first.get( i );
        double upper = inputBounds.second.get( i );

        if ( std::isinf( lower ) && std::isinf( upper ) )
        {
            sample[i] = 0.0f;
            epsilons[i] = std::numeric_limits<double>::infinity();
        }
        else if ( std::isinf( lower ) )
        {
            sample[i] = upper - RANGE;
            epsilons[i] = RANGE;
        }
        else if ( std::isinf( upper ) )
        {
            sample[i] = lower + RANGE;
            epsilons[i] = RANGE;
        }
        else
        {
            // Both bounds are finite
            sample[i] = lower + ( upper - lower ) / 2.0f;
            epsilons[i] = ( upper - lower ) / 2.0f;
        }
    }

    return { torch::tensor( sample.getContainer() ).unsqueeze( 0 ).to( device ),
             torch::tensor( epsilons.getContainer() ).to( device ) };
}

bool PGDAttack::_isWithinBounds( const torch::Tensor &sample,
                                 const std::pair<Vector<double>, Vector<double>> &bounds )
{
    torch::Tensor flatInput = sample.view( { -1 } );
    if ( flatInput.numel() != bounds.first.size() || flatInput.numel() != bounds.second.size() )
    {
        throw std::runtime_error( "Mismatch in sizes of input and bounds" );
    }

    for ( int64_t i = 0; i < flatInput.size( 0 ); i++ )
    {
        auto value = flatInput[i].item<double>();
        double lower = bounds.first.get( i );
        double upper = bounds.second.get( i );

        if ( std::isinf( lower ) && std::isinf( upper ) )
        {
            // If both bounds are infinite, any value is acceptable
            continue;
        }
        else if ( std::isinf( lower ) )
        {
            // Only check upper bound
            if ( value > upper )
                return false;
        }
        else if ( std::isinf( upper ) )
        {
            // Only check lower bound
            if ( value < lower )
                return false;
        }
        else
        {
            // Check both bounds
            if ( value < lower || value > upper )
                return false;
        }
    }

    return true;
}


torch::Tensor PGDAttack::_calculateLoss( const torch::Tensor &predictions )
{
    torch::Tensor lowerBoundTensor =
        torch::tensor( outputBounds.first.data(), torch::kFloat32 ).to( device );
    torch::Tensor upperBoundTensor =
        torch::tensor( outputBounds.second.data(), torch::kFloat32 ).to( device );

    // Compute the penalty: We want high loss if predictions are outside the bounds
    torch::Tensor ubViolation =
        torch::sum( torch::square( torch::relu( predictions - upperBoundTensor ) ) ).to( device );
    torch::Tensor lbViolation =
        torch::sum( torch::square( torch::relu( lowerBoundTensor - predictions ) ) ).to( device );
    return torch::sum( ubViolation + lbViolation ).to( device );
}


std::pair<torch::Tensor, torch::Tensor> PGDAttack::_findAdvExample()
{
    torch::Tensor currentExample;
    torch::Tensor currentPrediction;

    for ( unsigned i = 0; i < restarts; ++i )
    {
        torch::Tensor delta = torch::rand( inputSize ).mul( epsilon ).to( device );
        delta.set_requires_grad( true );
        torch::optim::Adam optimizer( { delta }, torch::optim::AdamOptions() );

        for ( unsigned j = 0; j < iters; ++j )
        {
            currentExample = InputExample + delta;
            currentPrediction = model.forward( currentExample );
            torch::Tensor loss = _calculateLoss( currentPrediction );
            optimizer.zero_grad();
            loss.backward();
            optimizer.step();
            delta.data() = delta.data().clamp( -epsilon, epsilon );
        }

        currentExample = InputExample + delta;
        currentPrediction = model.forward( currentExample );
        torch::Tensor currentLoss = _calculateLoss( currentPrediction );
        if ( _isWithinBounds( currentPrediction, outputBounds ) )
        {
            return { currentExample, currentPrediction };
        }
    }

    return { currentExample, currentPrediction };
    ;
}

double PGDAttack::getAssignment(  int index )
{
    return assignments[index];
}

bool PGDAttack::displayAdversarialExample()
{
    std::cout << "-----Starting PGD attack-----" << std::endl;
    auto adversarial = _findAdvExample();
    torch::Tensor advInput = adversarial.first;
    torch::Tensor advPred = adversarial.second;

    bool isFooled =
        _isWithinBounds( advInput, inputBounds ) && _isWithinBounds( advPred, outputBounds );

    auto *example = advInput.data_ptr<double>();
    auto *prediction = advPred.data_ptr<double>();
    size_t outputSize = advPred.size( 1 );
    ASSERT( advInput.size( 1 ) == inputSize && outputSize == model.layerSizes.last() );

    if ( isFooled )
    {
        _adversarialInput = new double[advInput.size( 1 )];
        _adversarialOutput = new double[advPred.size( 1 )];

        std::copy( example, example + inputSize, _adversarialInput );
        std::copy( prediction, prediction + outputSize, _adversarialOutput );
    }

    std::cout << "Input Lower Bounds : " << inputBounds.first.getContainer() << "\n";
    std::cout << "Input Upper Bounds : " << inputBounds.second.getContainer() << "\n";
    std::cout << "Adversarial Input: \n";
    for ( int i = 0; i < advInput.numel(); ++i )
    {
        std::cout << "x" << i << " = " << example[i] << "\n";
    }
    std::cout << "Output Lower Bounds : " << outputBounds.first.getContainer() << "\n";
    std::cout << "Output Upper Bounds : " << outputBounds.second.getContainer() << "\n";
    std::cout << "Adversarial Prediction: \n";
    for ( int i = 0; i < advPred.numel(); ++i )
    {
        std::cout << "y" << i << " = " << prediction[i] << "\n";
    }
    std::cout << "Model fooled: "
              << ( isFooled ? "Yes \n ------ PGD Attack Succeed ------"
                            : "No \n ------ PGD Attack Failed ------" )
              << "\n\n";

    if ( _adversarialInput )
        networkLevelReasoner->concretizeInputAssignment( assignments, _adversarialInput );

    return isFooled;
}
