//
// Created by User on 7/23/2025.
//

#include "AlphaCrown.h"

#include "MStringf.h"

namespace NLR {
AlphaCrown::AlphaCrown( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
{
    _network = new CustomDNN( static_cast<NetworkLevelReasoner *>(_layerOwner) );
    _network->getInputBounds( _lbInput, _ubInput );
    _inputSize = _lbInput.size( 0 ); // TODO it that length of tensor?

    _linearLayers = std::vector<torch::nn::Linear>( _network->getLinearLayers().begin(),
                                                    _network->getLinearLayers().end() );
    for ( const auto &linearLayer : _linearLayers )
    {
        _positiveWeights.push_back( torch::where( linearLayer->weight >= 0,
                                                  linearLayer->weight,
                                                  torch::zeros_like( linearLayer->weight ) ) );
        _negativeWeights.push_back( torch::where( linearLayer->weight <= 0,
                                                  linearLayer->weight,
                                                  torch::zeros_like( linearLayer->weight ) ) );
        _biases.push_back( linearLayer->bias );
    }
}

torch::Tensor AlphaCrown::createSymbolicVariablesMatrix()
{
    return torch::cat( { torch::eye( _inputSize ), torch::zeros( { _inputSize, 1 } ) }, 1 );
}

torch::Tensor AlphaCrown::lower_ReLU_relaxation( const torch::Tensor &u, const torch::Tensor &l )
{
    torch::Tensor mult;
    mult = torch::where( u - l == 0, torch::tensor( 1.0 ), u / ( u - l ) );
    mult = torch::where( l >= 0, torch::tensor( 1.0 ), mult );
    mult = torch::where( u <= 0, torch::tensor( 0.0 ), mult );
    return mult;
}

std::tuple<torch::Tensor, torch::Tensor> AlphaCrown::upper_ReLU_relaxation( const torch::Tensor &u,
                                                                            const torch::Tensor &l )
{
    torch::Tensor mult = torch::where( u - l == 0, torch::tensor( 1.0 ), u / ( u - l ) );
    mult = torch::where( l >= 0, torch::tensor( 1.0 ), mult );
    mult = torch::where( u <= 0, torch::tensor( 0.0 ), mult );

    torch::Tensor add = torch::where( u - l == 0, torch::tensor( 0.0 ), -l * mult );
    add = torch::where( l >= 0, torch::tensor( 0.0 ), add );

    return std::make_tuple( mult, add );
}
torch::Tensor AlphaCrown::getMaxOfSymbolicVariables( const torch::Tensor &matrix )
{
    auto coefficients = matrix.index(
        { torch::indexing::Slice(), torch::indexing::Slice( torch::indexing::None, -1 ) } );
    auto free_coefficients = matrix.index( { torch::indexing::Slice(), -1 } );

    auto positive_mask = coefficients >= 0;

    torch::Tensor u_values =
        torch::sum( torch::where( positive_mask, coefficients * _ubInput, coefficients * _lbInput ),
                    1 ) +
        free_coefficients;

    return u_values;
}

torch::Tensor AlphaCrown::getMinOfSymbolicVariables( const torch::Tensor &matrix )
{
    auto coefficients = matrix.index(
        { torch::indexing::Slice(), torch::indexing::Slice( torch::indexing::None, -1 ) } );
    auto free_coefficients = matrix.index( { torch::indexing::Slice(), -1 } );

    auto positive_mask = coefficients >= 0;

    torch::Tensor l_values =
        torch::sum( torch::where( positive_mask, coefficients * _lbInput, coefficients * _ubInput ),
                    1 ) +
        free_coefficients;

    return l_values;
}


void AlphaCrown::findBounds()
{
    torch::Tensor EQ_up = createSymbolicVariablesMatrix();
    torch::Tensor EQ_low = createSymbolicVariablesMatrix();

    for ( size_t i = 0; i < _linearLayers.size(); i++ )
    {
        auto Wi_positive = _positiveWeights[i];
        auto Wi_negative = _negativeWeights[i];
        auto Bi = _biases[i];

        auto EQ_up_before_activation = Wi_positive.mm( EQ_up ) + Wi_negative.mm( EQ_low );
        EQ_up_before_activation =
            AlphaCrown::addVecToLastColumnValue( EQ_up_before_activation, Bi );

        auto EQ_low_before_activation = Wi_positive.mm( EQ_low ) + Wi_negative.mm( EQ_up );
        EQ_low_before_activation =
            AlphaCrown::addVecToLastColumnValue( EQ_low_before_activation, Bi );

        if ( i == _linearLayers.size() - 1 )
        {
            // TODO how can we know what layer it is in nlr? in order to update bounds there?
            // we should get it from cDNN

            EQ_up = EQ_up_before_activation;
            EQ_low = EQ_low_before_activation;
            break;
        } // TODO we can skip it???


        // TODO we can use u_values and l_values of EQ_up to compute upper relaxation?

        auto u_values = AlphaCrown::getMaxOfSymbolicVariables( EQ_up_before_activation );
        auto l_values = AlphaCrown::getMinOfSymbolicVariables( EQ_low_before_activation );
        auto [upperRelaxationSlope, upperRelaxationIntercept] =
            AlphaCrown::upper_ReLU_relaxation( l_values, u_values );
        auto alphaSlope = AlphaCrown::lower_ReLU_relaxation( l_values, u_values );

        EQ_up = EQ_up_before_activation * upperRelaxationSlope.unsqueeze( 1 );
        EQ_up = AlphaCrown::addVecToLastColumnValue( EQ_up, upperRelaxationIntercept );
        EQ_low = EQ_low_before_activation * alphaSlope.unsqueeze( 1 );

        _upperRelaxationSlopes.push_back( upperRelaxationSlope );
        _upperRelaxationIntercepts.push_back( upperRelaxationIntercept );
        _alphaSlopes.push_back( alphaSlope );
    }
}

std::tuple<torch::Tensor, torch::Tensor>
AlphaCrown::computeBounds( std::vector<torch::Tensor> &alphaSlopes )
{
    torch::Tensor EQ_up = createSymbolicVariablesMatrix();
    torch::Tensor EQ_low = createSymbolicVariablesMatrix();

    for ( size_t i = 0; i < _linearLayers.size(); i++ )
    {
        auto Wi_positive = _positiveWeights[i];
        auto Wi_negative = _negativeWeights[i];
        auto Bi = _biases[i];

        auto EQ_up_before_activation = Wi_positive.mm( EQ_up ) + Wi_negative.mm( EQ_low );
        EQ_up_before_activation =
            AlphaCrown::addVecToLastColumnValue( EQ_up_before_activation, Bi );

        auto EQ_low_before_activation = Wi_positive.mm( EQ_low ) + Wi_negative.mm( EQ_up );
        EQ_low_before_activation =
            AlphaCrown::addVecToLastColumnValue( EQ_low_before_activation, Bi );

        if ( i == _linearLayers.size() - 1 )
        {
            EQ_up = EQ_up_before_activation;
            EQ_low = EQ_low_before_activation;
            break;
        }

        // TODO we can improve _upperRelaxationSlopes becouse we have better bound on each  neuron
        //  in hidden layer. if so we need to use it as an argument on each iteration becose it
        //  not constant

        EQ_up = EQ_up_before_activation * _upperRelaxationSlopes[i].unsqueeze( 1 ); //
        EQ_up = AlphaCrown::addVecToLastColumnValue( EQ_up, _upperRelaxationIntercepts[i] );
        EQ_low = EQ_low_before_activation * alphaSlopes[i].unsqueeze( 1 );
    }
    auto up_bound = getMaxOfSymbolicVariables( EQ_up );
    auto low_bound = getMinOfSymbolicVariables( EQ_low );
    return std::make_tuple( up_bound, low_bound );
}

void AlphaCrown::optimizeBounds( int loops )
{
    std::vector<torch::Tensor> alphaSlopesForUpBound;
    std::vector<torch::Tensor> alphaSlopesForLowBound;
    for ( auto &tensor : _alphaSlopes )
    {
        alphaSlopesForUpBound.push_back( tensor.copy_( tensor.detach().requires_grad_( true ) ) );
        alphaSlopesForLowBound.push_back( tensor.copy_( tensor.detach().requires_grad_( true ) ) );
    }
    AlphaCrown::GDloop( loops, "max", alphaSlopesForUpBound );
    AlphaCrown::GDloop( loops, "min", alphaSlopesForLowBound );
}


void AlphaCrown::GDloop( int loops,
                         const std::string val_to_opt,
                         std::vector<torch::Tensor> &alphaSlopes )
{
    torch::optim::Adam optimizer( alphaSlopes, 0.005 );
    for ( int i = 0; i < loops; i++ )
    {
        optimizer.zero_grad();

        auto [max_val, min_val] = AlphaCrown::computeBounds( alphaSlopes );
        auto loss = ( val_to_opt == "max" ) ? max_val : -min_val;
        loss.backward();
        optimizer.step();

        for ( auto &tensor : alphaSlopes )
        {
            tensor.clamp_( 0, 1 );
        }

        log( Stringf( "Optimization loop %d completed", i + 1 ) );
    }
}


void AlphaCrown::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyAnalysis: %s\n", message.ascii() );
}
} // namespace NLR
