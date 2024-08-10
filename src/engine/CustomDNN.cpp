#include "CustomDNN.h"

#include "NetworkLevelReasoner.h"
#include "Vector.h"

#include <TimeUtils.h>


CustomMaxPool::CustomMaxPool(const NLR::NetworkLevelReasoner* nlr, unsigned layerIndex)
    : networkLevelReasoner(nlr), maxLayerIndex(layerIndex) {}

torch::Tensor CustomMaxPool::forward( torch::Tensor x ) const
{
    std::cout << "start time: " << TimeUtils::now().ascii() << std::endl;
    fflush( stdout );

    const NLR::Layer *layer = networkLevelReasoner->getLayer( maxLayerIndex );
    torch::Tensor maxOutputs = torch::zeros( { 1, layer->getSize() } );

    for ( unsigned neuron = 0; neuron < layer->getSize(); ++neuron )
    {
        auto sources = layer->getActivationSources( neuron );
        torch::Tensor maxSource = torch::zeros( sources.size(), torch::kFloat );

        for ( int i = sources.size() - 1; i >= 0; --i )
        {
            const NLR::NeuronIndex &activationNeuron = sources.back();
            sources.popBack();
            int index = static_cast<int>( activationNeuron._neuron );
            maxSource[i] = x.index( { 0, index } );
        }
        maxOutputs.index_put_( { 0, static_cast<int>( neuron ) }, torch::max( maxSource ) );
    }

    std::cout << "end time: " << TimeUtils::now().ascii() << std::endl;
    fflush( stdout );

    return maxOutputs;
}

void CustomDNNImpl::weightedSum( unsigned i, const NLR::Layer *layer)
{
    unsigned sourceLayer = i - 1;
    const NLR::Layer *prevLayer = networkLevelReasoner->getLayer( sourceLayer );
    unsigned inputSize = prevLayer->getSize();
    unsigned outputSize = layer->getSize();

    if ( outputSize > 0 )
    {
        auto linearLayer = torch::nn::Linear( torch::nn::LinearOptions( inputSize, outputSize ) );
        linearLayers.append( linearLayer );
        register_module( "linear" + std::to_string( i ), linearLayer );

        Vector<Vector<float>> layerWeights( outputSize, Vector<float>( inputSize ) );
        Vector<float> layerBiases( layer->getSize() );

        // Fetch weights and biases from the networkLevelReasoner
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
                                         .view( { layer->getSize(), prevLayer->getSize() } );
        torch::Tensor biasTensor = torch::tensor( layerBiases.getContainer(), torch::kFloat );
        torch::NoGradGuard no_grad;
        linearLayer->weight.set_( weightTensor );
        linearLayer->bias.set_( biasTensor );
    }
}
CustomDNNImpl::CustomDNNImpl( const NLR::NetworkLevelReasoner *nlr )
{
    std::cout << "----- Construct Custom Network -----" << std::endl;
    networkLevelReasoner = nlr;
    numberOfLayers = networkLevelReasoner->getNumberOfLayers();
    for ( unsigned i = 0; i < numberOfLayers; i++ )
    {
        const NLR::Layer *layer = networkLevelReasoner->getLayer( i );
        layerSizes.append( layer->getSize() );
        NLR::Layer::Type layerType = layer->getLayerType();
        layersOrder.append( layerType );
        switch (layerType)
        {
        case NLR::Layer::INPUT:
            break;
        case NLR::Layer::WEIGHTED_SUM:
            weightedSum( i, layer );
            break;
        case NLR::Layer::RELU:
        {
            auto reluLayer = torch::nn::ReLU( torch::nn::ReLUOptions() );
            reluLayers.append( reluLayer );
            register_module( "relu" + std::to_string( i ), reluLayer );
            break;
        }
        case NLR::Layer::LEAKY_RELU:
        {
            auto reluLayer = torch::nn::LeakyReLU( torch::nn::LeakyReLUOptions() );
            leakyReluLayers.append( reluLayer );
            register_module( "leaky_relu" + std::to_string( i ), reluLayer );
            break;
        }
        case NLR::Layer::MAX:
        {
            auto customMaxPoolLayer = std::make_shared<CustomMaxPool>(networkLevelReasoner, i);
            customMaxPoolLayers.append(customMaxPoolLayer);
            register_module("custom_max_pool" + std::to_string(i), customMaxPoolLayer);
            break;
        }
        case NLR::Layer::SIGMOID:
        {
            auto sigmoidLayer = torch::nn::Sigmoid();
            sigmoidLayers.append(sigmoidLayer);
            register_module("sigmoid" + std::to_string(i), sigmoidLayer);
            break;
        }
        default:
            std::cerr << "Unsupported layer type: " << layerType << std::endl;
            break;
        }
    }
}



// torch::Tensor CustomDNNImpl::customMaxPool( unsigned maxLayerIndex, torch::Tensor x ) const
// {
//     std::cout << "start time: " << TimeUtils::now().ascii() << std::endl;
//     fflush( stdout );
//     const NLR::Layer *layer = networkLevelReasoner->getLayer( maxLayerIndex );
//     torch::Tensor maxOutputs = torch::zeros( { 1, layer->getSize() } );
//
//     torch::Tensor maxSource;
//
//     for ( unsigned neuron = 0; neuron < layer->getSize(); ++neuron )
//     {
//         auto sources = layer->getActivationSources( neuron );
//         maxSource = torch::zeros( sources.size(), torch::kFloat );
//
//         for ( int i = sources.size() - 1; i >= 0; --i )
//         {
//             const NLR::NeuronIndex &activationNeuron = sources.back();
//             sources.popBack();
//             int index = static_cast<int>( activationNeuron._neuron );
//             maxSource[i] = x.index( { 0, index } );
//         }
//         maxOutputs.index_put_( { 0, static_cast<int>( neuron ) }, torch::max( maxSource ) );
//     }
//     std::cout << "end time: " << TimeUtils::now().ascii() << std::endl;
//     fflush( stdout );
//
//
//     return maxOutputs;
// }

torch::Tensor CustomDNNImpl::forward(torch::Tensor x)
{
    unsigned linearIndex = 0;
    unsigned reluIndex = 0;
    unsigned maxPoolIndex = 0;
    unsigned leakyReluIndex = 0;
    unsigned sigmoidIndex = 0;
    for (unsigned i = 0; i < numberOfLayers; i++)
    {
        NLR::Layer::Type layerType = layersOrder[i];
        switch (layerType)
        {
        case NLR::Layer::INPUT:
            break;
        case NLR::Layer::WEIGHTED_SUM:
            x = linearLayers[linearIndex]->forward(x);
            linearIndex++;
            break;
        case NLR::Layer::RELU:
            x = reluLayers[reluIndex]->forward( x );
            reluIndex++;
            break;
        case NLR::Layer::MAX:
            x = customMaxPoolLayers[maxPoolIndex]->forward( x );
            maxPoolIndex++;
            break;
        case NLR::Layer::LEAKY_RELU:
            x = leakyReluLayers[leakyReluIndex]->forward(x);
            leakyReluIndex++;
            break;
        case NLR::Layer::SIGMOID:
            x = sigmoidLayers[sigmoidIndex]->forward(x);
            sigmoidIndex++;
            break;
        default:
            std::cerr << "Unsupported activation type: " << activations[i] << std::endl;

        }
    }
    return x;
}



// torch::Tensor CustomDNNImpl::forward( torch::Tensor x )
// {
//     unsigned linearIndex = 0;
//     unsigned activationIndex = 0;
//     unsigned maxPoolNum = 0;
//     for ( unsigned i = 0; i < numberOfLayers; i++ )
//     {
//         NLR::Layer::Type layerType = layersOrder[i];
//         switch ( layerType )
//         {
//         case NLR::Layer::INPUT:
//             break;
//         case NLR::Layer::WEIGHTED_SUM:
//             x = linearLayers[linearIndex]->forward( x );
//             linearIndex++;
//
//             break;
//
//         case ACTIVATION:
//         {
//             NLR::Layer::Type activationType = activations[activationIndex];
//             switch ( activationType )
//             {
//             case NLR::Layer::RELU:
//                 x = torch::relu( x );
//                 activationIndex++;
//
//                 break;
//
//             case NLR::Layer::LEAKY_RELU:
//                 x = torch::leaky_relu( x );
//                 activationIndex++;
//                 break;
//
//             case NLR::Layer::SIGMOID:
//                 x = torch::sigmoid( x );
//                 activationIndex++;
//                 break;
//
//             case NLR::Layer::ABSOLUTE_VALUE:
//                 x = torch::abs( x );
//                 activationIndex++;
//                 break;
//
//             case NLR::Layer::MAX:
//             {
//                 std::cout << x.dim() << std::endl;
//                 std::cout << "dim 1: " << x.size( 0 ) << std::endl;
//                 std::cout << "dim 2: " << x.size( 1 ) << std::endl;
//                 int kernel_size = 0;
//                 // x = customMaxPool( maxLayerIndices[maxPoolNum], x );
//                 if ( x.size( 1 ) == 33856 )
//                     kernel_size = 2881;
//                 else if (x.size(1) == 5184)
//                     kernel_size = 1089;
//                 else if (x.size( 1 ) == 256)
//                     kernel_size = 1;
//                 // Apply the max pooling operation
//                 torch::nn::MaxPool1d max_pool1d(
//                     torch::nn::MaxPool1dOptions( kernel_size ).stride( 4 ) );
//
//                 x = max_pool1d( x );
//                 x = torch::max_pool1d( x, 4, 4);
//                 std::cout << "end max pool" << x.size( 1 ) << std::endl;
//                 activationIndex++;
//                 maxPoolNum++;
//                 break;
//             }
//
//             case NLR::Layer::SIGN:
//                 x = torch::sign( x );
//                 activationIndex++;
//                 break;
//
//             case NLR::Layer::ROUND:
//                 x = torch::round( x );
//                 activationIndex++;
//                 break;
//
//             case NLR::Layer::SOFTMAX:
//                 x = torch::softmax( x, 1 );
//                 activationIndex++;
//                 break;
//
//             default:
//                 std::cerr << "Unsupported activation type: " << activations[i] << std::endl;
//             }
//         }
//         }
//     }
//     return x;
