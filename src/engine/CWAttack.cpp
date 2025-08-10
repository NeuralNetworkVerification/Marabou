#include "CWAttack.h"

#ifdef BUILD_TORCH

CWAttack::CWAttack( NLR::NetworkLevelReasoner *networkLevelReasoner )
    : networkLevelReasoner( networkLevelReasoner )
    , _device( torch::cuda::is_available() ? torch::kCUDA : torch::kCPU )
    , _model( std::make_unique<NLR::CustomDNN>( networkLevelReasoner ) )
    , _iters( GlobalConfiguration::CW_DEFAULT_ITERS )
    , _restarts( GlobalConfiguration::CW_NUM_RESTARTS )
    , _specLossWeight( 1e-2 )
    , _adversarialInput( nullptr )
    , _adversarialOutput( nullptr )
{
    _inputSize = _model->getLayerSizes().first();
    getBounds( _inputBounds, GlobalConfiguration::PdgBoundType::ATTACK_INPUT );
    getBounds( _outputBounds, GlobalConfiguration::PdgBoundType::ATTACK_OUTPUT );

    _inputLb = torch::tensor( _inputBounds.first.getContainer(), torch::kFloat32 ).to( _device );
    _inputUb = torch::tensor( _inputBounds.second.getContainer(), torch::kFloat32 ).to( _device );

    auto vars = generateSampleAndEpsilon();
    _x0 = vars.first;
}

CWAttack::~CWAttack()
{
    if ( _adversarialInput )
        delete[] _adversarialInput;
    if ( _adversarialOutput )
        delete[] _adversarialOutput;
}

bool CWAttack::runAttack()
{
    CW_LOG( "-----Starting CW attack-----" );
    auto adversarial = findAdvExample();
    auto advInput = adversarial.first.to( torch::kDouble );
    auto advPred = adversarial.second.to( torch::kDouble );

    bool isFooled =
        isWithinBounds( advInput, _inputBounds ) && isWithinBounds( advPred, _outputBounds );

    auto inputPtr = advInput.data_ptr<double>();
    auto predPtr = advPred.data_ptr<double>();
    size_t outSize = advPred.size( 1 );

    if ( isFooled )
    {
        _adversarialInput = new double[_inputSize];
        _adversarialOutput = new double[outSize];
        std::copy( inputPtr, inputPtr + _inputSize, _adversarialInput );
        std::copy( predPtr, predPtr + outSize, _adversarialOutput );
    }
    CW_LOG( "Input Lower Bounds : " );
    for ( auto &bound : _inputBounds.first.getContainer() )
        printValue( bound );
    CW_LOG( "Input Upper Bounds : " );
    for ( auto &bound : _inputBounds.second.getContainer() )
        printValue( bound );

    CW_LOG( "Adversarial Input:" );
    for ( int i = 0; i < advInput.numel(); ++i )
    {
        CW_LOG( Stringf( "x%u=%.3lf", i, inputPtr[i] ).ascii() );
    }
    CW_LOG( "Output Lower Bounds : " );
    for ( auto &bound : _outputBounds.first.getContainer() )
        printValue( bound );
    CW_LOG( "Output Upper Bounds : " );
    for ( auto &bound : _outputBounds.second.getContainer() )
        printValue( bound );

    CW_LOG( "Adversarial Prediction: " );
    for ( int i = 0; i < advPred.numel(); ++i )
    {
        CW_LOG( Stringf( "y%u=%.3lf", i, predPtr[i] ).ascii() );
    }


    if ( isFooled )
    {
        CW_LOG( "Model fooled: Yes \n ------ CW Attack Succeed ------\n" );
    }
    else
        CW_LOG( "Model fooled: No \n ------ CW Attack Failed ------\n" );
    // Concretize assignments if attack succeeded
    if ( _adversarialInput )
        networkLevelReasoner->concretizeInputAssignment( _assignments, _adversarialInput );
    return isFooled;
}


void CWAttack::getBounds( std::pair<Vector<double>, Vector<double>> &bounds, signed type ) const
{
    unsigned layerIndex = type == GlobalConfiguration::PdgBoundType::ATTACK_INPUT
                            ? 0
                            : networkLevelReasoner->getNumberOfLayers() - 1;
    const NLR::Layer *layer = networkLevelReasoner->getLayer( layerIndex );
    for ( unsigned i = 0; i < layer->getSize(); ++i )
    {
        bounds.first.append( layer->getLb( i ) );
        bounds.second.append( layer->getUb( i ) );
    }
}

std::pair<torch::Tensor, torch::Tensor> CWAttack::generateSampleAndEpsilon()
{
    Vector<double> sample( _inputSize, 0.0 ), eps( _inputSize, 0.0 );
    for ( unsigned i = 0; i < _inputSize; ++i )
    {
        double lo = _inputBounds.first.get( i ), hi = _inputBounds.second.get( i );
        if ( std::isfinite( lo ) && std::isfinite( hi ) )
        {
            sample[i] = 0.5 * ( lo + hi );
            eps[i] = 0.5 * ( hi - lo );
        }
        else
        {
            sample[i] = 0.0;
            eps[i] = GlobalConfiguration::ATTACK_INPUT_RANGE;
        }
    }
    auto s = torch::tensor( sample.getContainer(), torch::kFloat32 ).unsqueeze( 0 ).to( _device );
    auto e = torch::tensor( eps.getContainer(), torch::kFloat32 ).to( _device );
    return { s, e };
}

torch::Tensor CWAttack::calculateLoss( const torch::Tensor &pred )
{
    auto lb = torch::tensor( _outputBounds.first.data(), torch::kFloat32 ).to( _device );
    auto ub = torch::tensor( _outputBounds.second.data(), torch::kFloat32 ).to( _device );
    auto ubv = torch::sum( torch::square( torch::relu( pred - ub ) ) );
    auto lbv = torch::sum( torch::square( torch::relu( lb - pred ) ) );
    return ( ubv + lbv ).to( _device );
}

std::pair<torch::Tensor, torch::Tensor> CWAttack::findAdvExample()
{
    torch::Tensor bestAdv, bestPred;
    double bestL2 = std::numeric_limits<double>::infinity();
    double timeoutForAttack = ( Options::get()->getInt( Options::ATTACK_TIMEOUT ) == 0
                                    ? FloatUtils::infinity()
                                    : Options::get()->getInt( Options::ATTACK_TIMEOUT ) );
    CW_LOG( Stringf( "Adversarial attack timeout set to %f\n", timeoutForAttack ).ascii() );
    timespec startTime = TimeUtils::sampleMicro();
    torch::Tensor advExample;
    for ( unsigned r = 0; r < _restarts; ++r )
    {
        unsigned long timePassed = TimeUtils::timePassed( startTime, TimeUtils::sampleMicro() );
        if ( static_cast<long double>( timePassed ) / MICROSECONDS_TO_SECONDS > timeoutForAttack )
        {
            throw MarabouError( MarabouError::TIMEOUT, "Attack failed due to timeout" );
        }
        torch::Tensor delta = torch::zeros_like( _x0, torch::requires_grad() ).to( _device );
        torch::optim::Adam optimizer( { delta },
                                      torch::optim::AdamOptions( GlobalConfiguration::CW_LR ) );

        for ( unsigned it = 0; it < _iters; ++it )
        {
            torch::Tensor prevExample = advExample;
            advExample = ( _x0 + delta ).clamp( _inputLb, _inputUb );
            // Skip the equality check on the first iteration
            if ( ( it > 0 && prevExample.defined() && advExample.equal( prevExample ) ) ||
                 !isWithinBounds( advExample, _inputBounds ) )
                break;
            auto pred = _model->forward( advExample );
            auto specLoss = calculateLoss( pred );
            auto l2norm = torch::sum( torch::pow( advExample - _x0, 2 ) );
            auto loss = l2norm + _specLossWeight * specLoss;

            optimizer.zero_grad();
            loss.backward();
            optimizer.step();

            if ( specLoss.item<double>() == 0.0 )
            {
                double curL2 = l2norm.item<double>();
                if ( curL2 < bestL2 )
                {
                    bestL2 = curL2;
                    bestAdv = advExample.detach();
                    bestPred = pred.detach();
                }
            }
        }
    }

    if ( !bestAdv.defined() )
    {
        bestAdv = ( _x0 + torch::zeros_like( _x0 ) ).clamp( _inputLb, _inputUb );
        bestPred = _model->forward( bestAdv );
    }

    return { bestAdv, bestPred };
}

bool CWAttack::isWithinBounds( const torch::Tensor &sample,
                               const std::pair<Vector<double>, Vector<double>> &bounds )
{
    torch::Tensor flatInput = sample.view( { -1 } );
    if ( flatInput.numel() != (int)bounds.first.size() ||
         flatInput.numel() != (int)bounds.second.size() )
        throw std::runtime_error( "Mismatch in sizes of input and bounds" );

    for ( int64_t i = 0; i < flatInput.size( 0 ); ++i )
    {
        double v = flatInput[i].item<double>();
        double lo = bounds.first.get( i ), hi = bounds.second.get( i );
        if ( std::isinf( lo ) && std::isinf( hi ) )
            continue;
        if ( std::isinf( lo ) )
        {
            if ( v > hi )
                return false;
        }
        else if ( std::isinf( hi ) )
        {
            if ( v < lo )
                return false;
        }
        else if ( v < lo || v > hi )
            return false;
    }
    return true;
}

double CWAttack::getAssignment( int index )
{
    return _assignments[index];
}

void CWAttack::printValue( double value )
{
    if ( std::isinf( value ) )
    {
        if ( value < 0 )
        {
            CW_LOG( "-inf" );
        }
        else
        {
            CW_LOG( "inf" );
        }
    }
    else if ( std::isnan( value ) )
    {
        CW_LOG( "nan" );
    }
    else
    {
        CW_LOG( Stringf( "%.3lf", value ).ascii() );
    }
}

#endif // BUILD_TORCH
