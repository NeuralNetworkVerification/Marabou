//
// Created by User on 7/23/2025.
//

#include "AlphaCrown.h"
#include "MStringf.h"
#include "NetworkLevelReasoner.h"
#include "Layer.h"

namespace NLR {
AlphaCrown::AlphaCrown( LayerOwner *layerOwner )
    : _layerOwner( layerOwner )
{
    _network = new CustomDNN( dynamic_cast<NetworkLevelReasoner *>( _layerOwner ) );
    _network->getInputBounds( _lbInput, _ubInput );
    _inputSize = _lbInput.size( 0 );
    _network->getLinearLayers().end() ;
    _linearLayers = _network->getLinearLayers().getContainer();
    _layersOrder = _network->getLayersOrder().getContainer();

    unsigned linearIndex = 0;
    for ( unsigned i = 0; i < _network->getNumberOfLayers(); i++ )
    {
        if (_layersOrder[i] != Layer::WEIGHTED_SUM) continue;
        //const Layer *layer = _layerOwner->getLayer( i );
        auto linearLayer = _linearLayers[linearIndex];
        auto whights = linearLayer->weight;
        auto bias = linearLayer->bias;
        _positiveWeights.insert( {i,torch::where( whights >= 0,whights,
                                                    torch::zeros_like(
                                                        whights ) ).to(torch::kFloat32)} );
        _negativeWeights.insert( {i,torch::where( whights <= 0,whights,
                                                    torch::zeros_like(
                                                        whights ) ).to(torch::kFloat32)} );
        _biases.insert( {i,bias.to(torch::kFloat32)} );
        linearIndex += 1;
    }
}

torch::Tensor AlphaCrown::createSymbolicVariablesMatrix()
{
    // Create the identity matrix and the zero matrix
    auto eye_tensor = torch::eye(_inputSize, torch::kFloat32);   // Ensure float32
    auto zero_tensor = torch::zeros({_inputSize, 1}, torch::kFloat32); // Ensure float32

    // Concatenate the two tensors horizontally (along dim=1)
    return torch::cat({eye_tensor, zero_tensor}, 1);  // Will be of type float32
}

torch::Tensor AlphaCrown::lower_ReLU_relaxation( const torch::Tensor &u, const torch::Tensor &l )
{
    torch::Tensor mult;
    mult = torch::where( u - l == 0, torch::tensor( 1.0 ), u / ( u - l ) );
    mult = torch::where( l >= 0, torch::tensor( 1.0 ), mult );
    mult = torch::where( u <= 0, torch::tensor( 0.0 ), mult );
    return mult.to(torch::kFloat32);
}

std::tuple<torch::Tensor, torch::Tensor> AlphaCrown::upper_ReLU_relaxation( const torch::Tensor &u,
                                                                            const torch::Tensor &l )
{
    torch::Tensor mult = torch::where( u - l == 0, torch::tensor( 1.0 ), u / ( u - l ) );
    mult = torch::where( l >= 0, torch::tensor( 1.0 ), mult );
    mult = torch::where( u <= 0, torch::tensor( 0.0 ), mult );

    torch::Tensor add = torch::where( u - l == 0, torch::tensor( 0.0 ), -l * mult );
    add = torch::where( l >= 0, torch::tensor( 0.0 ), add );

    return std::make_tuple( mult.to(torch::kFloat32), add.to(torch::kFloat32) );
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

void AlphaCrown::relaxReluLayer(unsigned layerNumber, torch::Tensor
                                                           &EQ_up, torch::Tensor &EQ_low){

    auto u_values_EQ_up = AlphaCrown::getMaxOfSymbolicVariables(EQ_up);
    auto l_values_EQ_up = AlphaCrown::getMinOfSymbolicVariables(EQ_low);
    auto [upperRelaxationSlope, upperRelaxationIntercept] =
        AlphaCrown::upper_ReLU_relaxation(l_values_EQ_up, u_values_EQ_up);

    auto u_values_EQ_low = AlphaCrown::getMaxOfSymbolicVariables(EQ_up);
    auto l_values_EQ_low = AlphaCrown::getMinOfSymbolicVariables(EQ_low);
    auto alphaSlope = AlphaCrown::lower_ReLU_relaxation(l_values_EQ_low,
                                                         u_values_EQ_low);

    EQ_up = EQ_up * upperRelaxationSlope.unsqueeze( 1 );
    EQ_up = AlphaCrown::addVecToLastColumnValue( EQ_up, upperRelaxationIntercept );
    EQ_low = EQ_low * alphaSlope.unsqueeze( 1 );

    _upperRelaxationSlopes.insert({layerNumber, upperRelaxationSlope} );
    // back but insert to dict
    _upperRelaxationIntercepts.insert({layerNumber, upperRelaxationIntercept}  );
    _indexAlphaSlopeMap.insert( {layerNumber, _initialAlphaSlopes.size()} );
    _initialAlphaSlopes.push_back( alphaSlope );

}



void AlphaCrown::findBounds()
{
    torch::Tensor EQ_up = createSymbolicVariablesMatrix();
    torch::Tensor EQ_low = createSymbolicVariablesMatrix();

    for ( unsigned i = 0; i < _network->getNumberOfLayers(); i++ ){
        Layer::Type layerType = _layersOrder[i];
        switch (layerType)
        {
        case Layer::INPUT:
            break;
        case Layer::WEIGHTED_SUM:
            computeWeightedSumLayer(i, EQ_up, EQ_low);
            break;
        case Layer::RELU:
            relaxReluLayer(i, EQ_up, EQ_low);
            break;
        default:
            AlphaCrown::log ( "Unsupported layer type\n");
            throw MarabouError( MarabouError::DEBUGGING_ERROR );
        }
    }
}


std::tuple<torch::Tensor, torch::Tensor> AlphaCrown::computeBounds
    (std::vector<torch::Tensor> &alphaSlopes)
{
    torch::Tensor EQ_up = createSymbolicVariablesMatrix();
    torch::Tensor EQ_low = createSymbolicVariablesMatrix();
    for ( unsigned i = 0; i < _network->getNumberOfLayers(); i++ )
    {
        auto layerType = _layersOrder[i];
        switch (layerType)
        {
        case Layer::INPUT:
            break;
        case Layer::WEIGHTED_SUM:
            computeWeightedSumLayer (i, EQ_up, EQ_low);
            break;
        case Layer::RELU:
            computeReluLayer (i, EQ_up, EQ_low, alphaSlopes);
            break;
        default:
            log ("Unsupported layer type\n");
            throw MarabouError (MarabouError::DEBUGGING_ERROR);
        }
    }
    auto outputUpBound = getMaxOfSymbolicVariables(EQ_up);
    auto outputLowBound = getMinOfSymbolicVariables(EQ_low);
    return std::make_tuple(outputUpBound, outputLowBound);

}


void AlphaCrown::computeWeightedSumLayer(unsigned i, torch::Tensor &EQ_up, torch::Tensor &EQ_low){
    //auto linearLayer = _linearLayers[i];
    auto Wi_positive = _positiveWeights[i];
    auto Wi_negative = _negativeWeights[i];
    auto Bi = _biases[i];

    auto EQ_up_afterLayer = Wi_positive.mm( EQ_up ) + Wi_negative.mm( EQ_low );
    EQ_up_afterLayer =
        AlphaCrown::addVecToLastColumnValue( EQ_up_afterLayer, Bi );


    auto EQ_low_afterLayer = Wi_positive.mm( EQ_low ) + Wi_negative.mm( EQ_up );
    EQ_low_afterLayer =
        AlphaCrown::addVecToLastColumnValue(EQ_low_afterLayer, Bi );

    EQ_up = EQ_up_afterLayer;
    EQ_low = EQ_low_afterLayer;

}


void AlphaCrown::computeReluLayer(unsigned layerNumber, torch::Tensor
                                                             &EQ_up, torch::Tensor &EQ_low, std::vector<torch::Tensor> &alphaSlopes){
    EQ_up = EQ_up * _upperRelaxationSlopes[layerNumber].unsqueeze( 1 ); //
    EQ_up = AlphaCrown::addVecToLastColumnValue( EQ_up, _upperRelaxationIntercepts[layerNumber] );
    unsigned indexInAlpha = _indexAlphaSlopeMap[layerNumber];
    EQ_low = EQ_low * alphaSlopes[indexInAlpha].unsqueeze( 1 );
}




void AlphaCrown::updateBounds(std::vector<torch::Tensor> &alphaSlopes){
    torch::Tensor EQ_up = createSymbolicVariablesMatrix();
    torch::Tensor EQ_low = createSymbolicVariablesMatrix();


    for ( unsigned i = 0; i < _network->getNumberOfLayers(); i++ )
    {
        auto layerType = _layersOrder[i];
        switch (layerType)
        {
        case Layer::INPUT:
            break;
        case Layer::WEIGHTED_SUM:
            computeWeightedSumLayer (i, EQ_up, EQ_low);
            break;
        case Layer::RELU:
            computeReluLayer (i, EQ_up, EQ_low, alphaSlopes);
            break;
        default:
            log ("Unsupported layer type\n");
            throw MarabouError (MarabouError::DEBUGGING_ERROR);
        }
        auto upBound = getMaxOfSymbolicVariables(EQ_up);
        auto lowBound = getMinOfSymbolicVariables(EQ_low);
        updateBoundsOfLayer(i, upBound, lowBound);
    }
}

void AlphaCrown::updateBoundsOfLayer(unsigned layerIndex, torch::Tensor &upBounds, torch::Tensor &lowBounds)
{

    Layer * layer = _layerOwner->getLayerIndexToLayer()[layerIndex];
    //TODO it should be: Layer *layer = _layerOwner->getLayer(layerIndex); if we added non const getter

    for (int j = 0; j < upBounds.size(0); j++)
    {
        if ( layer->neuronEliminated( j ) ) continue;
        double lb_val = lowBounds[j].item<double>();
        if ( layer->getLb( j ) < lb_val )
        {
            log( Stringf( "Neuron %u_%u lower-bound updated from  %f to %f",
                          layerIndex,
                          j,
                          layer->getLb( j ),
                          lb_val ) );

            std::cout << "Neuron " << layerIndex << "_" << j
                      << " lower-bound updated from " << layer->getLb(j)
                      << " to " << lb_val << std::endl;
            layer->setLb( j, lb_val );
            _layerOwner->receiveTighterBound(
                Tightening( layer->neuronToVariable( j ), lb_val, Tightening::LB ) );
        }


        auto ub_val = upBounds[j].item<double>();
        if ( layer->getUb( j ) > ub_val )
        {
            log( Stringf( "Neuron %u_%u upper-bound updated from  %f to %f",
                          layerIndex,
                          j,
                          layer->getUb( j ),
                          ub_val ) );
            std::cout << "Neuron " << layerIndex << "_" << j
                      << " upper-bound updated from " << layer->getUb(j)
                      << " to " << ub_val << std::endl;

            layer->setUb( j, ub_val );
            _layerOwner->receiveTighterBound(
                Tightening( layer->neuronToVariable( j ), ub_val, Tightening::UB ) );
        }

    }
}


void AlphaCrown::optimizeBounds( int loops )
{
    std::vector<torch::Tensor> alphaSlopesForUpBound;
    std::vector<torch::Tensor> alphaSlopesForLowBound;
    for ( auto &tensor : _initialAlphaSlopes )
    {
        alphaSlopesForUpBound.push_back( tensor.detach().clone().requires_grad_(true) );
        alphaSlopesForLowBound.push_back( tensor.detach().clone().requires_grad_(true) );
    }
    AlphaCrown::GDloop( loops, "max", alphaSlopesForUpBound );
    AlphaCrown::GDloop( loops, "min", alphaSlopesForLowBound );
    updateBounds( alphaSlopesForUpBound );
    updateBounds( alphaSlopesForLowBound);
    std::cout << "AlphaCrown run completed." << std::endl;
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
        auto loss = ( val_to_opt == "max" ) ? max_val.sum() : -min_val.sum();
        loss.backward(torch::Tensor(), true);

        optimizer.step();

        for ( auto &tensor : alphaSlopes )
        {
            tensor.clamp( 0, 1 );
        }

        log( Stringf( "Optimization loop %d completed", i + 1 ) );
        std::cout << "std Optimization loop completed " << i+1 << std::endl;
    }
}


void AlphaCrown::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyAnalysis: %s\n", message.ascii() );
}


} // namespace NLR