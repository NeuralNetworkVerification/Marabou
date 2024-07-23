#include "CustomDNN.h"
#include "Vector.h"
#include "NetworkLevelReasoner.h"

CustomDNNImpl::CustomDNNImpl(const NLR::NetworkLevelReasoner* networkLevelReasoner) {

    std::cout << "----- Construct Custom Network -----" << std::endl;

    numberOfLayers = networkLevelReasoner->getNumberOfLayers();
    for (unsigned i = 0; i < numberOfLayers; i++) {
        const NLR::Layer* layer = networkLevelReasoner->getLayer(i);
        layerSizes.append(layer->getSize());
        NLR::Layer::Type layerType = layer->getLayerType();

        if (layerType == NLR::Layer::WEIGHTED_SUM) {
            layersOrder.append( LINEAR );
            // Fully connected layer
            unsigned sourceLayer = i - 1;
            const NLR::Layer *prevLayer = networkLevelReasoner->getLayer(sourceLayer);
            unsigned inputSize = prevLayer->getSize();
            unsigned outputSize = layer->getSize();

            if (outputSize > 0) {
                auto linearLayer = torch::nn::Linear(torch::nn::LinearOptions(inputSize, outputSize));
                linearLayers.append(linearLayer);
                register_module("linear" + std::to_string(i), linearLayer);

                Vector<Vector<float>> layerWeights(outputSize, Vector<float>(inputSize));
                Vector<float> layerBiases(layer->getSize());

                // Fetch weights and biases from the networkLevelReasoner
                for (unsigned j = 0; j < outputSize; j++) {
                    for (unsigned k = 0; k < inputSize; k++) {
                        double weight_value = layer->getWeight(sourceLayer, k, j);
                        layerWeights[j][k] = static_cast<float>(weight_value);
                    }
                    double bias_value = layer->getBias(j);
                    layerBiases[j] = static_cast<float>(bias_value);
                }

                Vector<float> flattenedWeights;
                for (const auto& weight : layerWeights) {
                    for (const auto& w : weight) {
                        flattenedWeights.append(w);
                    }
                }

                torch::Tensor weightTensor = torch::tensor(flattenedWeights.getContainer(), torch::kFloat)
                    .view({ layer->getSize(), prevLayer->getSize() });
                torch::Tensor biasTensor = torch::tensor(layerBiases.getContainer(), torch::kFloat);
                torch::NoGradGuard no_grad;
                linearLayer->weight.set_(weightTensor);
                linearLayer->bias.set_(biasTensor);
            }
        // activation layers
        } else if (layerType == NLR::Layer::RELU || layerType == NLR::Layer::LEAKY_RELU ||
                   layerType == NLR::Layer::SIGMOID || layerType == NLR::Layer::ABSOLUTE_VALUE ||
                   layerType == NLR::Layer::MAX || layerType == NLR::Layer::SIGN ||
                   layerType == NLR::Layer::ROUND || layerType == NLR::Layer::SOFTMAX) {
            layersOrder.append( ACTIVATION );
            activations.append(layerType);
        } else if (layerType == NLR::Layer::INPUT) {
            // No action needed for input layer
            layersOrder.append( INPUT );
        } else {
            std::cerr << "Unsupported layer type: " << layerType << std::endl;
        }
    }
    std::cout << "Number of layers: " << linearLayers.size() + 1<< std::endl;
    std::cout << "Number of activations: " << activations.size() << std::endl;
    std::cout << "Input layer size: " << layerSizes.first() << std::endl;
    std::cout << "Output layer size: " << layerSizes.last() << std::endl;

}

torch::Tensor CustomDNNImpl::forward(torch::Tensor x) {
    unsigned linearIndex = 0;
    unsigned activationIndex = 0;
    for ( unsigned i = 1; i < numberOfLayers; i++ )
    {
        switch (layersOrder[i])
        {
        case LINEAR:

            x = linearLayers[linearIndex]->forward( x );
            linearIndex ++;
            break;

        case ACTIVATION:
            {
            switch ( activations[activationIndex] )
            {
            case NLR::Layer::RELU:
                x = torch::relu( x );
                activationIndex ++;
                break;

            case NLR::Layer::LEAKY_RELU:
                x = torch::leaky_relu( x );
                activationIndex ++;
                break;

            case NLR::Layer::SIGMOID:
                x = torch::sigmoid( x );
                activationIndex ++;
                break;

            case NLR::Layer::ABSOLUTE_VALUE:
                x = torch::abs( x );
                activationIndex ++;
                break;

            case NLR::Layer::MAX:
                x = std::get<0>( torch::max( x, 1, true ) );
                activationIndex ++;
                break;

            case NLR::Layer::SIGN:
                x = torch::sign( x );
                activationIndex ++;
                break;

            case NLR::Layer::ROUND:
                x = torch::round( x );
                activationIndex ++;
                break;

            case NLR::Layer::SOFTMAX:
                x = torch::softmax( x, 1 );
                activationIndex ++;
                break;

            default:
                std::cerr << "Unsupported activation type: " << activations[i] << std::endl;
            }
            }
        }
    }
    return x;
}