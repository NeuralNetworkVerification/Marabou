#include "NetworkLevelReasoner.h"
#include "CustomDNN.h"
#ifdef BUILD_TORCH
namespace NLR {
CustomRelu::CustomRelu( const NetworkLevelReasoner *nlr, unsigned layerIndex )
    : _networkLevelReasoner( nlr )
    , _reluLayerIndex( layerIndex )
{
}

torch::Tensor CustomRelu::forward( torch::Tensor x ) const
{
    return CustomReluFunction::apply( x, _networkLevelReasoner, _reluLayerIndex );
}

CustomMaxPool::CustomMaxPool( const NetworkLevelReasoner *nlr, unsigned layerIndex )
    : _networkLevelReasoner( nlr )
    , _maxLayerIndex( layerIndex )
{
}

torch::Tensor CustomMaxPool::forward( torch::Tensor x ) const
{
    return CustomMaxPoolFunction::apply( x, _networkLevelReasoner, _maxLayerIndex );
}

void CustomDNN::setWeightsAndBiases( torch::nn::Linear &linearLayer,
                                     const Layer *layer,
                                     unsigned sourceLayer,
                                     unsigned inputSize,
                                     unsigned outputSize )
{
    Vector<Vector<float>> layerWeights( outputSize, Vector<float>( inputSize ) );
    Vector<float> layerBiases( outputSize );

    // Fetch weights and biases from networkLevelReasoner
    for ( unsigned j = 0; j < outputSize; j++ )
    {
        for ( unsigned k = 0; k < inputSize; k++ )
        {
            double weight_value = layer->getWeight( sourceLayer, k, j );
            layerWeights[j][k] = static_cast<float>( weight_value );
        }
        double bias_value = layer->getBias( j );
        layerBiases[j] = static_cast<float>( bias_value );
    }

    Vector<float> flattenedWeights;
    for ( const auto &weight : layerWeights )
    {
        for ( const auto &w : weight )
        {
            flattenedWeights.append( w );
        }
    }

    torch::Tensor weightTensor = torch::tensor( flattenedWeights.getContainer(), torch::kFloat )
                                     .view( { outputSize, inputSize } );
    torch::Tensor biasTensor = torch::tensor( layerBiases.getContainer(), torch::kFloat );

    torch::NoGradGuard no_grad;
    linearLayer->weight.set_( weightTensor );
    linearLayer->bias.set_( biasTensor );
}

void CustomDNN::weightedSum( unsigned i, const Layer *layer )
{
    unsigned sourceLayer = i - 1;
    const Layer *prevLayer = _networkLevelReasoner->getLayer( sourceLayer );
    unsigned inputSize = prevLayer->getSize();
    unsigned outputSize = layer->getSize();

    if ( outputSize > 0 )
    {
        auto linearLayer = torch::nn::Linear( torch::nn::LinearOptions( inputSize, outputSize ) );
        _linearLayers.append( linearLayer );

        setWeightsAndBiases( linearLayer, layer, sourceLayer, inputSize, outputSize );

        register_module( "linear" + std::to_string( i ), linearLayer );
    }
}


CustomDNN::CustomDNN( const NetworkLevelReasoner *nlr )
{
    CUSTOM_DNN_LOG( "----- Construct Custom Network -----" );
    _networkLevelReasoner = nlr;
    _numberOfLayers = _networkLevelReasoner->getNumberOfLayers();
    for ( unsigned i = 0; i < _numberOfLayers; i++ )
    {
        const Layer *layer = _networkLevelReasoner->getLayer( i );
        _layerSizes.append( layer->getSize() );
        Layer::Type layerType = layer->getLayerType();
        _layersOrder.append( layerType );
        switch ( layerType )
        {
        case Layer::INPUT:
            break;
        case Layer::WEIGHTED_SUM:
            weightedSum( i, layer );
            break;
        case Layer::RELU:
        {
            auto reluLayer = std::make_shared<CustomRelu>( _networkLevelReasoner, i );
            _reluLayers.append( reluLayer );
            register_module( "ReLU" + std::to_string( i ), reluLayer );
            break;
        }
        case Layer::MAX:
        {
            auto maxPoolLayer = std::make_shared<CustomMaxPool>( _networkLevelReasoner, i );
            _maxPoolLayers.append( maxPoolLayer );
            register_module( "maxPool" + std::to_string( i ), maxPoolLayer );
            break;
        }
        default:
            CUSTOM_DNN_LOG( "Unsupported layer type\n" );
            throw MarabouError( MarabouError::DEBUGGING_ERROR );
        }
    }
}

torch::Tensor CustomDNN::forward( torch::Tensor x )
{
    unsigned linearIndex = 0;
    unsigned reluIndex = 0;
    unsigned maxPoolIndex = 0;
    for ( unsigned i = 0; i < _numberOfLayers; i++ )
    {
        const Layer::Type layerType = _layersOrder[i];
        switch ( layerType )
        {
        case Layer::INPUT:
            break;
        case Layer::WEIGHTED_SUM:
            x = _linearLayers[linearIndex]->forward( x );
            linearIndex++;
            break;
        case Layer::RELU:
            x = _reluLayers[reluIndex]->forward( x );
            reluIndex++;
            break;
        case Layer::MAX:
            x = _maxPoolLayers[maxPoolIndex]->forward( x );
            maxPoolIndex++;
            break;
        default:
            CUSTOM_DNN_LOG( "Unsupported layer type\n" );
            throw MarabouError( MarabouError::DEBUGGING_ERROR );
            break;
        }
    }
    return x;
}

torch::Tensor CustomReluFunction::forward( torch::autograd::AutogradContext *ctx,
                                           torch::Tensor x,
                                           const NetworkLevelReasoner *nlr,
                                           unsigned int layerIndex )
{
    ctx->save_for_backward( { x } );

    const Layer *layer = nlr->getLayer( layerIndex );
    torch::Tensor reluOutputs = torch::zeros( { 1, layer->getSize() } );
    torch::Tensor reluGradients = torch::zeros( { 1, layer->getSize() } );

    for ( unsigned neuron = 0; neuron < layer->getSize(); ++neuron )
    {
        auto sources = layer->getActivationSources( neuron );
        ASSERT( sources.size() == 1 );
        const NeuronIndex &sourceNeuron = sources.back();
        int index = static_cast<int>( sourceNeuron._neuron );
        reluOutputs.index_put_( { 0, static_cast<int>( neuron ) },
                                torch::clamp_min( x.index( { 0, index } ), 0 ) );
        reluGradients.index_put_( { 0, static_cast<int>( neuron ) }, x.index( { 0, index } ) > 0 );
    }

    ctx->saved_data["reluGradients"] = reluGradients;

    return reluOutputs;
}

std::vector<torch::Tensor> CustomReluFunction::backward( torch::autograd::AutogradContext *ctx,
                                                         std::vector<torch::Tensor> grad_output )
{
    auto saved = ctx->get_saved_variables();
    auto input = saved[0];

    auto reluGradients = ctx->saved_data["reluGradients"].toTensor();
    auto grad_input = grad_output[0] * reluGradients[0];

    return { grad_input, torch::Tensor(), torch::Tensor() };
}

torch::Tensor CustomMaxPoolFunction::forward( torch::autograd::AutogradContext *ctx,
                                              torch::Tensor x,
                                              const NetworkLevelReasoner *nlr,
                                              unsigned int layerIndex )
{
    ctx->save_for_backward( { x } );

    const Layer *layer = nlr->getLayer( layerIndex );
    torch::Tensor maxOutputs = torch::zeros( { 1, layer->getSize() } );
    torch::Tensor argMaxOutputs = torch::zeros( { 1, layer->getSize() }, torch::kInt64 );

    for ( unsigned neuron = 0; neuron < layer->getSize(); ++neuron )
    {
        auto sources = layer->getActivationSources( neuron );
        torch::Tensor sourceValues = torch::zeros( sources.size(), torch::kFloat );
        torch::Tensor sourceIndices = torch::zeros( sources.size() );

        for ( int i = sources.size() - 1; i >= 0; --i )
        {
            const NeuronIndex &activationNeuron = sources.back();
            int index = static_cast<int>( activationNeuron._neuron );
            sources.popBack();
            sourceValues.index_put_( { i }, x.index( { 0, index } ) );
            sourceIndices.index_put_( { i }, index );
        }

        maxOutputs.index_put_( { 0, static_cast<int>( neuron ) }, torch::max( sourceValues ) );
        argMaxOutputs.index_put_( { 0, static_cast<int>( neuron ) },
                                  sourceIndices.index( { torch::argmax( sourceValues ) } ) );
    }

    ctx->saved_data["argMaxOutputs"] = argMaxOutputs;

    return maxOutputs;
}

std::vector<torch::Tensor> CustomMaxPoolFunction::backward( torch::autograd::AutogradContext *ctx,
                                                            std::vector<torch::Tensor> grad_output )
{
    auto saved = ctx->get_saved_variables();
    auto input = saved[0];

    auto grad_input = torch::zeros_like( input );

    auto indices = ctx->saved_data["argMaxOutputs"].toTensor();

    grad_input[0].index_add_( 0, indices.flatten(), grad_output[0].flatten() );

    return { grad_input, torch::Tensor(), torch::Tensor() };
}

const Vector<unsigned> &CustomDNN::getLayerSizes() const
{
    return _layerSizes;
}

torch::Tensor CustomDNN::getLayerWeights(unsigned layerIndex) const {
    if (_layersOrder[layerIndex] == Layer::WEIGHTED_SUM) {
        auto linearLayer = _linearLayers[layerIndex];
        return linearLayer->weight; // Returning weights of the corresponding linear layer
    }
    throw std::runtime_error("Requested weights for a non-weighted sum layer.");
}

torch::Tensor CustomDNN::getLayerBias(unsigned layerIndex) const {
    if (_layersOrder[layerIndex] == Layer::WEIGHTED_SUM) {
        auto linearLayer = _linearLayers[layerIndex];
        return linearLayer->bias; // Returning bias of the corresponding linear layer
    }
    throw std::runtime_error("Requested bias for a non-weighted sum layer.");
}

void CustomDNN::getInputBounds(torch::Tensor &lbTensor, torch::Tensor &ubTensor) const
{
    const Layer *layer = _networkLevelReasoner->getLayer(0);
    unsigned size = layer->getSize();

    std::vector<double> lowerBounds;
    std::vector<double> upperBounds;
    lowerBounds.reserve(size);
    upperBounds.reserve(size);

    for (unsigned neuron = 0; neuron < size; ++neuron)
    {
        lowerBounds.push_back(layer->getLb(neuron));
        upperBounds.push_back(layer->getUb(neuron));
    }

    lbTensor = torch::tensor(lowerBounds, torch::kDouble);
    ubTensor = torch::tensor(upperBounds, torch::kDouble);
}



std::vector<List<NeuronIndex>> CustomDNN::getMaxPoolSources(const Layer* maxPoolLayer) {
    std::vector<List<NeuronIndex>> sources;
    unsigned size = maxPoolLayer->getSize();
    for (unsigned neuron = 0; neuron < size; ++neuron) {

        sources.push_back(maxPoolLayer->getActivationSources(neuron));
    }
    return sources;
}

}

#endif