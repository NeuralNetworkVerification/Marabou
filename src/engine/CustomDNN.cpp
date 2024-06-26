#include "CustomDNN.h"

#include "NetworkLevelReasoner.h"

CustomDNNImpl::CustomDNNImpl( const std::vector<int> &layerSizes,
                              const std::vector<std::string> &activationFunctions,
                              const std::vector<std::vector<std::vector<float>>> &weights,
                              const std::vector<std::vector<float>> &biases )
{
    for ( size_t i = 0; i < layerSizes.size() - 1; ++i )
    {
        auto layer = register_module( "fc" + std::to_string( i ),
                                      torch::nn::Linear( layerSizes[i], layerSizes[i + 1] ) );
        linearLayers.push_back( layer );
        activations.push_back( activationFunctions[i] );

        std::vector<float> flattened_weights;
        for ( const auto &weight : weights[i] )
        {
            flattened_weights.insert( flattened_weights.end(), weight.begin(), weight.end() );
        }


        torch::Tensor weightTensor = torch::tensor( flattened_weights, torch::kFloat );
        weightTensor = weightTensor.view( { layerSizes[i + 1], layerSizes[i] } );

        torch::Tensor biasTensor = torch::tensor( biases[i], torch::kFloat );

        layer->weight.data().copy_( weightTensor );
        layer->bias.data().copy_( biasTensor );
    }
}


CustomDNNImpl::CustomDNNImpl( NLR::NetworkLevelReasoner &nlr )
{
    std::vector<int> layerSizes;
    std::vector<std::string> activationFunctions;
    std::vector<std::vector<std::vector<float>>> weights;
    std::vector<std::vector<float>> biases;

    int numLayers = nlr.getNumberOfLayers();
    for ( int i = 0; i < numLayers; ++i )
    {
        const NLR::Layer *layer = nlr.getLayer( i );
        layerSizes.push_back( layer->getSize() );

        switch ( layer->getLayerType() )
        {
        case NLR::Layer::RELU:
            activationFunctions.push_back( "relu" );
            break;
        case NLR::Layer::SIGMOID:
            activationFunctions.push_back( "sigmoid" );
            break;
        default:
            activationFunctions.push_back( "none" );
            break;
        }

        if ( i < numLayers - 1 )
        {
            const NLR::Layer *nextLayer = nlr.getLayer( i + 1 );
            std::vector<std::vector<float>> layerWeights( nextLayer->getSize(),
                                                          std::vector<float>( layer->getSize() ) );
            std::vector<float> layerBiases( layer->getSize() );

            for ( unsigned j = 0; j < nextLayer->getSize(); j++ )
            {
                for ( unsigned k = 0; k < layer->getSize(); ++k )
                {
                    layerWeights[j][k] =
                        static_cast<float>( nlr.getLayer( i )->getWeight( i, k, j ) );
                }
            }
            for ( unsigned j = 0; j < nextLayer->getSize(); ++j )
            {
                layerBiases[j] = static_cast<float>( layer->getBias( j ) );
            }

            weights.push_back( layerWeights );
            biases.push_back( layerBiases );
        }
    }

    for ( size_t i = 0; i < layerSizes.size() - 1; ++i )
    {
        linearLayers.push_back(
            register_module( "linear" + std::to_string( i ),
                             torch::nn::Linear( layerSizes[i], layerSizes[i + 1] ) ) );
        std::vector<float> flattenedWeights;
        for ( const auto &weight : weights[i] )
        {
            flattenedWeights.insert( flattenedWeights.end(), weight.begin(), weight.end() );
        }

        torch::Tensor weightTensor = torch::tensor( flattenedWeights, torch::kFloat )
                                         .view( { layerSizes[i + 1], layerSizes[i] } );
        torch::Tensor biasTensor = torch::tensor( biases[i], torch::kFloat );
        auto &linear = linearLayers.back();
        linear->weight.data().copy_( weightTensor );
        linear->bias.data().copy_( biasTensor );
    }
}
torch::Tensor CustomDNNImpl::forward( torch::Tensor x )
{
    for ( size_t i = 0; i < linearLayers.size(); ++i )
    {
        x = linearLayers[i]->forward( x );
        if ( activations[i] == "ReLU" )
        {
            x = torch::relu( x );
        }
        else if ( activations[i] == "Sigmoid" )
        {
            x = torch::sigmoid( x );
        }
        else if ( activations[i] == "Tanh" )
        {
            x = torch::tanh( x );
        }
        else if ( activations[i] == "Softmax" )
        {
            x = torch::softmax( x, 1 );
        }
    }
    return x;
}