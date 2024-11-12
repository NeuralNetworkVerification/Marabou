#include "PGD.h"

#include "InputQuery.h"

#include <iostream>

PGDAttack::PGDAttack( NLR::NetworkLevelReasoner *networkLevelReasoner )
    : networkLevelReasoner( networkLevelReasoner )
    , _device( torch::kCPU )
    , _model( CustomDNN( networkLevelReasoner ) )
    , _iters( GlobalConfiguration::PGD_DEFAULT_NUM_ITER )
    , _restarts( GlobalConfiguration::PGD_NUM_RESTARTS )
    , _adversarialInput( nullptr )
    , _adversarialOutput( nullptr )
{
    _inputSize = _model.getLayerSizes().first();
    getBounds( _inputBounds, GlobalConfiguration::PdgBoundType::PGD_INPUT );
    getBounds( _outputBounds, GlobalConfiguration::PdgBoundType::PGD_OUTPUT );
    std::pair<torch::Tensor, torch::Tensor> variables = generateSampleAndEpsilon();
    _inputExample = variables.first;
    _epsilon = variables.second;
}

torch::Device PGDAttack::getDevice()
{
    if ( torch::cuda::is_available() )
    {
        PGD_LOG( "CUDA Device available. Using GPU" );
        return { torch::kCUDA };
    }
    else
    {
        PGD_LOG( "CUDA Device not available. Using CPU" );
        return { torch::kCPU };
    }
}

void PGDAttack::getBounds( std::pair<Vector<double>, Vector<double>> &bounds,
                           const signed type ) const
{
    unsigned layerIndex = type == GlobalConfiguration::PdgBoundType::PGD_INPUT
                            ? 0
                            : networkLevelReasoner->getNumberOfLayers() - 1;
    const NLR::Layer *layer = networkLevelReasoner->getLayer( layerIndex );

    for ( unsigned neuron = 0; neuron < layer->getSize(); ++neuron )
    {
        bounds.first.append( layer->getLb( neuron ) );
        bounds.second.append( layer->getUb( neuron ) );
    }
}

std::pair<torch::Tensor, torch::Tensor> PGDAttack::generateSampleAndEpsilon()
{
    Vector<double> sample( _inputSize, 0.0f );
    Vector<double> epsilons( _inputSize, 0.0f );

    for ( unsigned i = 0; i < _inputSize; i++ )
    {
        double lower = _inputBounds.first.get( i );
        double upper = _inputBounds.second.get( i );

        if ( std::isinf( lower ) && std::isinf( upper ) )
        {
            sample[i] = 0.0f;
            epsilons[i] = std::numeric_limits<double>::infinity();
        }
        else if ( std::isinf( lower ) )
        {
            sample[i] = upper - GlobalConfiguration::PGD_INPUT_RANGE;
            epsilons[i] = GlobalConfiguration::PGD_INPUT_RANGE;
        }
        else if ( std::isinf( upper ) )
        {
            sample[i] = lower + GlobalConfiguration::PGD_INPUT_RANGE;
            epsilons[i] = GlobalConfiguration::PGD_INPUT_RANGE;
        }
        else
        {
            // Both bounds are finite
            sample[i] = lower + ( upper - lower ) / 2.0f;
            epsilons[i] = ( upper - lower ) / 2.0f;
        }
    }

    return { torch::tensor( sample.getContainer() ).unsqueeze( 0 ).to( _device ),
             torch::tensor( epsilons.getContainer() ).to( _device ) };
}

bool PGDAttack::isWithinBounds( const torch::Tensor &sample,
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


torch::Tensor PGDAttack::calculateLoss( const torch::Tensor &predictions )
{
    torch::Tensor lowerBoundTensor =
        torch::tensor( _outputBounds.first.data(), torch::kFloat32 ).to( _device );
    torch::Tensor upperBoundTensor =
        torch::tensor( _outputBounds.second.data(), torch::kFloat32 ).to( _device );

    // Compute the penalty: High loss if predictions are outside the bounds
    torch::Tensor ubViolation =
        torch::sum( torch::square( torch::relu( predictions - upperBoundTensor ) ) ).to( _device );
    torch::Tensor lbViolation =
        torch::sum( torch::square( torch::relu( lowerBoundTensor - predictions ) ) ).to( _device );
    return torch::sum( ubViolation + lbViolation ).to( _device );
}

std::pair<torch::Tensor, torch::Tensor> PGDAttack::findAdvExample()
{
    double timeoutForAttack = ( Options::get()->getInt( Options::ATTACK_TIMEOUT ) == 0
                                    ? FloatUtils::infinity()
                                    : Options::get()->getInt( Options::ATTACK_TIMEOUT ) );
    PGD_LOG( Stringf( "Adversarial attack timeout set to %f\n", timeoutForAttack ).ascii() );
    PGD_LOG( Stringf( "Adversarial attack start time: %f\n", TimeUtils::now() ).ascii() );
    timespec startTime = TimeUtils::sampleMicro();

    torch::Tensor currentExample = _inputExample;
    torch::Tensor currentPrediction;
    torch::Tensor lowerBoundTensor =
        torch::tensor( _inputBounds.first.data(), torch::kFloat32 ).to( _device );
    torch::Tensor upperBoundTensor =
        torch::tensor( _inputBounds.second.data(), torch::kFloat32 ).to( _device );
    torch::Tensor delta = torch::zeros( _inputSize ).to( _device ).requires_grad_( true );

    for ( unsigned i = 0; i < _restarts; ++i )
    {
        unsigned long timePassed = TimeUtils::timePassed( startTime, TimeUtils::sampleMicro() );
        if ( static_cast<long double>( timePassed ) / MICROSECONDS_TO_SECONDS > timeoutForAttack )
        {
            throw MarabouError( MarabouError::TIMEOUT, "Attack failed due to timeout" );
        }
        torch::optim::Adam optimizer( { delta }, torch::optim::AdamOptions() );
        for ( unsigned j = 0; j < _iters; ++j )
        {
            currentExample = currentExample + delta;
            currentPrediction = _model.forward( currentExample );
            if ( isWithinBounds( currentPrediction, _outputBounds ) )
            {
                return { currentExample, currentPrediction };
            }
            optimizer.zero_grad();
            torch::Tensor loss = calculateLoss( currentPrediction );
            loss.backward();
            optimizer.step();
            delta.data() = delta.data().clamp( -_epsilon, _epsilon );
        }
        delta = ( torch::rand( _inputSize ) * 2 - 1 )
                    .mul( _epsilon )
                    .to( _device )
                    .requires_grad_( true );
    }
    return { currentExample, currentPrediction };
}

double PGDAttack::getAssignment( int index )
{
    return _assignments[index];
}

bool PGDAttack::runAttack()
{
    PGD_LOG( "-----Starting PGD attack-----" );
    auto adversarial = findAdvExample();
    torch::Tensor advInput = adversarial.first.to( torch::kDouble );
    torch::Tensor advPred = adversarial.second.to( torch::kDouble );

    bool isFooled =
        isWithinBounds( advInput, _inputBounds ) && isWithinBounds( advPred, _outputBounds );

    auto *example = advInput.data_ptr<double>();
    auto *prediction = advPred.data_ptr<double>();
    size_t outputSize = advPred.size( 1 );
    ASSERT( advInput.size( 1 ) == _inputSize && outputSize == _model.getLayerSizes().last() );

    if ( isFooled )
    {
        _adversarialInput = new double[advInput.size( 1 )];
        _adversarialOutput = new double[advPred.size( 1 )];

        std::copy( example, example + _inputSize, _adversarialInput );
        std::copy( prediction, prediction + outputSize, _adversarialOutput );
    }
    PGD_LOG( "Input Lower Bounds : \n" );
    for ( auto &bound : _inputBounds.first.getContainer() )
        PGD_LOG( Stringf( "%.3lf", bound ).ascii() );
    PGD_LOG( "Input Upper Bounds : \n" );
    for ( auto &bound : _inputBounds.second.getContainer() )
        PGD_LOG( Stringf( "%.3lf", bound ).ascii() );

    PGD_LOG( "Adversarial Input:\n" );
    std::cout << "Adversarial Input: \n";
    for ( int i = 0; i < advInput.numel(); ++i )
    {
        PGD_LOG( "x%u=%.3lf", i, example[i] );
    }
    PGD_LOG( "Output Lower Bounds : \n" );
    for ( auto &bound : _outputBounds.first.getContainer() )
        PGD_LOG( Stringf( "%.3lf", bound ).ascii() );
    PGD_LOG( "Output Upper Bounds : \n" );
    for ( auto &bound : _outputBounds.second.getContainer() )
        PGD_LOG( Stringf( "%.3lf", bound ).ascii() );

    std::cout << "Adversarial Prediction: \n";
    for ( int i = 0; i < advPred.numel(); ++i )
    {
        PGD_LOG( "y%u=%.3lf", i, prediction[i] );
    }


    if ( isFooled )
    {
        PGD_LOG( "Model fooled: Yes \n ------ PGD Attack Succeed ------\n" );
    }
    else
        PGD_LOG( "Model fooled: No \n ------ PGD Attack Failed ------\n" );


    if ( _adversarialInput )
        networkLevelReasoner->concretizeInputAssignment( _assignments, _adversarialInput );

    return isFooled;
}
