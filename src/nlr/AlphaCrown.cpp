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
    _nlr = dynamic_cast<NetworkLevelReasoner *>( layerOwner );
    _network = new CustomDNN( _nlr );
    _network->getInputBounds( _lbInput, _ubInput );
    _inputSize = _lbInput.size( 0 );
    _linearLayers = _network->getLinearLayers().getContainer();
    _layersOrder = _network->getLayersOrder().getContainer();

    unsigned linearIndex = 0;
    for ( unsigned i = 0; i < _network->getNumberOfLayers(); i++ )
    {
        if (_layersOrder[i] == Layer::WEIGHTED_SUM)
        {
            // const Layer *layer = _layerOwner->getLayer( i );
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
        if (_layersOrder[i] == Layer::MAX)
        {
            _maxPoolSources.insert({i, _network->getMaxPoolSources(_nlr->getLayer( i ) )});
        }
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

void AlphaCrown::relaxMaxPoolLayer(unsigned layerNumber,
                                    torch::Tensor &EQ_up,
                                    torch::Tensor &EQ_low)
{
    std::cout << "Relaxing MaxPool layer number: " << layerNumber << std::endl;
    const auto &groups = _maxPoolSources[layerNumber];
    TORCH_CHECK(!groups.empty(), "MaxPool layer has no groups");

    const auto cols = EQ_up.size(1);
    auto next_EQ_up  = torch::zeros({ (long)groups.size(), cols }, torch::kFloat32);
    auto next_EQ_low = torch::zeros({ (long)groups.size(), cols }, torch::kFloat32);

    std::vector<long>  upIdx;   upIdx.reserve(groups.size());
    std::vector<long>  loIdx;   loIdx.reserve(groups.size());
    std::vector<float> slopes;  slopes.reserve(groups.size());
    std::vector<float> ints;    ints.reserve(groups.size());

    for (size_t k = 0; k < groups.size(); ++k)
    {
        // Get per-neuron relaxation parameters & indices
        auto R = relaxMaxNeuron(groups, k, EQ_up, EQ_low);

        // Build next rows:
        // Upper: slope * EQ_up[R.idx_up]  (+ intercept on last column)
        auto up_row = EQ_up.index({ (long)R.idx_up, torch::indexing::Slice() }) * R.slope;
        auto bvec   = torch::full({1}, R.intercept, torch::kFloat32);
        up_row      = AlphaCrown::addVecToLastColumnValue(up_row, bvec);

        // Lower: copy EQ_low[R.idx_low]
        auto low_row = EQ_low.index({ (long)R.idx_low, torch::indexing::Slice() }).clone();

        next_EQ_up.index_put_ ( { (long)k, torch::indexing::Slice() }, up_row );
        next_EQ_low.index_put_( { (long)k, torch::indexing::Slice() }, low_row );

        // Persist
        upIdx.push_back(R.idx_up);
        loIdx.push_back(R.idx_low);
        slopes.push_back(R.slope);
        ints.push_back(R.intercept);
    }


    _maxUpperChoice[layerNumber] = torch::from_blob(
                                       upIdx.data(), {(long)upIdx.size()}, torch::TensorOptions().dtype(torch::kLong)).clone();
    _maxLowerChoice[layerNumber] = torch::from_blob(
                                       loIdx.data(), {(long)loIdx.size()}, torch::TensorOptions().dtype(torch::kLong)).clone();
    _upperRelaxationSlopes[layerNumber] =
        torch::from_blob(slopes.data(), {(long)slopes.size()}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
    _upperRelaxationIntercepts[layerNumber] =
        torch::from_blob(ints.data(), {(long)ints.size()}, torch::TensorOptions().dtype(torch::kFloat32)).clone();

    // Advance EQs
    EQ_up  = next_EQ_up;
    EQ_low = next_EQ_low;
}




std::pair<torch::Tensor, torch::Tensor>
AlphaCrown::boundsFromEQ(const torch::Tensor &EQ, const std::vector<long> &rows)
{
    TORCH_CHECK(!rows.empty(), "boundsFromEQ: empty rows");
    auto idx = torch::from_blob(const_cast<long*>(rows.data()),
                                 {(long)rows.size()},
                                 torch::TensorOptions().dtype(torch::kLong)).clone();
    auto sub = EQ.index({ idx, torch::indexing::Slice() });  // |S| x (n+1)
    auto U = getMaxOfSymbolicVariables(sub);  // |S|
    auto L = getMinOfSymbolicVariables(sub);  // |S|
    return {U, L};
}



AlphaCrown::MaxRelaxResult AlphaCrown::relaxMaxNeuron(const std::vector<List<NeuronIndex>> &groups,
                                                       size_t k,
                                                       const torch::Tensor &EQ_up,
                                                       const torch::Tensor &EQ_low)
{
    constexpr double EPS = 1e-12;

    // Collect absolute previous-layer row indices for output k
    std::vector<long> srcRows; srcRows.reserve(16);
    const auto &srcList = groups[k];
    for (const auto &ni : srcList) {
        srcRows.push_back((long)ni._neuron);
    }
    TORCH_CHECK(!srcRows.empty(), "MaxPool group has no sources");


    auto [U_low, L_low] = boundsFromEQ(EQ_low, srcRows);
    auto M_low = (U_low + L_low) / 2.0;
    long j_rel_low = torch::argmax(M_low).item<long>();
    long idx_low_abs = srcRows[(size_t)j_rel_low];


    auto [U_up, L_up] = boundsFromEQ(EQ_up, srcRows);

    // i = argmax U_up, j = second argmax U_up (or i if single source)
    int64_t kTop = std::min<int64_t>(2, U_up.size(0));
    auto top2    = torch::topk(U_up, kTop, /dim=/0, /largest=/true, /sorted=/true);
    auto Uidxs   = std::get<1>(top2);
    long i_rel   = Uidxs[0].item<long>();
    long j_rel2  = (kTop > 1) ? Uidxs[1].item<long>() : Uidxs[0].item<long>();

    double li = L_up[i_rel].item<double>();
    double ui = U_up[i_rel].item<double>();
    double uj = U_up[j_rel2].item<double>();

    // Case 1: (li == max(L_up)) ∧ (li >= uj)
    auto Lmax_pair = torch::max(L_up, /dim=/0);
    long l_arg     = std::get<1>(Lmax_pair).item<long>();
    bool case1     = (i_rel == l_arg) && (li + EPS >= uj);

    float slope, intercept;
    if (case1 || (ui - li) <= EPS) {
        // Case 1 (or degenerate): y ≤ x_i  → a=1, intercept=0
        slope = 1.0f;
        intercept = 0.0f;
    } else {
        // Case 2: a=(ui-uj)/(ui-li), b=uj  → store as (a*xi + (b - a*li))
        double a = (ui - uj) / (ui - li);
        if (a < 0.0) a = 0.0;
        if (a > 1.0) a = 1.0;
        slope = (float)a;
        intercept = (float)(uj - a * li);   // this is what you ADD to last column
    }

    long idx_up_abs = srcRows[(size_t)i_rel];
    return MaxRelaxResult{ idx_up_abs, idx_low_abs, slope, intercept };
}


void AlphaCrown::computeMaxPoolLayer(unsigned layerNumber,
                                      torch::Tensor &EQ_up,
                                      torch::Tensor &EQ_low)
{
    auto idxUp = _maxUpperChoice.at(layerNumber);                     // int64 [m]
    auto idxLo = _maxLowerChoice.at(layerNumber);                     // int64 [m]
    auto a     = _upperRelaxationSlopes.at(layerNumber).to(torch::kFloat32);     // [m]
    auto b     = _upperRelaxationIntercepts.at(layerNumber).to(torch::kFloat32); // [m]

    // Select rows from current EQs
    auto up_sel  = EQ_up.index ({ idxUp, torch::indexing::Slice() });   // m x (n+1)
    auto low_sel = EQ_low.index({ idxLo, torch::indexing::Slice() });

    // Upper: scale + add intercept on last column
    auto next_up = up_sel * a.unsqueeze(1);
    next_up = AlphaCrown::addVecToLastColumnValue(next_up, b);

    // Lower: copy chosen rows
    auto next_low = low_sel.clone();

    EQ_up  = next_up;
    EQ_low = next_low;
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
        case Layer::MAX:
        {
            relaxMaxPoolLayer( i, EQ_up, EQ_low );
            break;
        }
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
        case Layer::MAX:
            computeMaxPoolLayer( i, EQ_up, EQ_low );
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
        case Layer::MAX:
            computeMaxPoolLayer( i, EQ_up, EQ_low );
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


    std::cout << "Starting AlphaCrown run with " << loops << " optimization loops." << std::endl;
    std::vector<torch::Tensor> alphaSlopesForUpBound;
    std::vector<torch::Tensor> alphaSlopesForLowBound;
    for ( auto &tensor : _initialAlphaSlopes )
    {
        alphaSlopesForUpBound.push_back( tensor.detach().clone().requires_grad_(true) );
        alphaSlopesForLowBound.push_back( tensor.detach().clone().requires_grad_(true) );
    }
    GDloop( loops, "max", alphaSlopesForUpBound );
    GDloop( loops, "min", alphaSlopesForLowBound );
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


torch::Tensor AlphaCrown::addVecToLastColumnValue(const torch::Tensor &matrix,
                                                   const torch::Tensor &vec)
{
    auto result = matrix.clone();
    if (result.dim() == 2)
    {
        // add 'vec' per row to last column
        result.slice(1, result.size(1) - 1, result.size(1)) += vec.unsqueeze(1);
    }
    else if (result.dim() == 1)
    {
        // add scalar to last entry (the constant term)
        TORCH_CHECK(vec.numel() == 1, "1-D addVec expects scalar vec");
        result.index_put_({ result.size(0) - 1 },
                           result.index({ result.size(0) - 1 }) + vec.item<float>());
    }
    else
    {
        TORCH_CHECK(false, "addVecToLastColumnValue expects 1-D or 2-D tensor");
    }
    return result;
}



void AlphaCrown::log( const String &message )
{
    if ( GlobalConfiguration::NETWORK_LEVEL_REASONER_LOGGING )
        printf( "DeepPolyAnalysis: %s\n", message.ascii() );
}


} // namespace NLR