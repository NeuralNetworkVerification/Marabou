#include "CustomDNN.h"
#include "Vector.h"
#include "NetworkLevelReasoner.h"

CustomDNNImpl::CustomDNNImpl(const NLR::NetworkLevelReasoner* nlr) {

    std::cout << "----- Construct Custom Network -----" << std::endl;
    networkLevelReasoner = nlr;
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
                   layerType == NLR::Layer::SIGN ||
                   layerType == NLR::Layer::ROUND || layerType == NLR::Layer::SOFTMAX) {
            layersOrder.append( ACTIVATION );
            activations.append(layerType);
        }
        else if(layerType == NLR::Layer::MAX)
        {
            maxLayerIndices.append( i );
            layersOrder.append( ACTIVATION );
            activations.append(layerType);
        }else if (layerType == NLR::Layer::INPUT) {
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




torch::Tensor CustomDNNImpl::customMaxPool(unsigned maxLayerIndex, torch::Tensor *x)
{
    const NLR::Layer *layer = networkLevelReasoner->getLayer(maxLayerIndex);
    Vector<float> newX;
    for (unsigned neuron = 0; neuron < layer->getSize(); ++neuron)
    {
        float maxVal = -std::numeric_limits<float>::infinity();
        for (const NLR::NeuronIndex activationNeuron : layer->getActivationSources(neuron))
        {
            int index = static_cast<int>(activationNeuron._neuron);
            maxVal = (std::max(maxVal, x->index({0, index}).item<float>()));
        }
        newX.append(maxVal);
    }

    unsigned batchSize = x->size(0);
    unsigned newSize = static_cast<unsigned>(newX.size());
    return torch::tensor(newX.getContainer(), torch::kFloat).view({batchSize, newSize});
}



torch::Tensor CustomDNNImpl::forward(torch::Tensor x) {
    unsigned linearIndex = 0;
    unsigned activationIndex = 0;
    unsigned maxPoolNum = 0;
    for ( unsigned i = 0; i < numberOfLayers; i++ )
    {
        Type layerType = layersOrder[i];
        switch (layerType)
        {
        case INPUT:
            // std::cout << "input layer " << i << ":  shape: " << x.sizes() << std::endl;
            break;
        case LINEAR:
            // std::cout << "linear Layer " << i << ":  shape: " << x.sizes() << std::endl;
            x = linearLayers[linearIndex]->forward( x );
            linearIndex ++;
            // std::cout << " after linear Layer " << i << ":  shape: " << x.sizes() << std::endl;

            break;

        case ACTIVATION:
            {
            NLR::Layer::Type activationType = activations[activationIndex];
            switch ( activationType )
            {
            case NLR::Layer::RELU:
                // std::cout << "Relu Layer " << i << ":  shape: " << x.sizes() << std::endl;
                x = torch::relu( x );
                activationIndex ++;
                // std::cout << "after Relu Layer " << i << ":  shape: " << x.sizes() << std::endl;

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
            {
                // std::cout << "max Layer " << i << ":  shape: " << x.sizes() << std::endl;
                unsigned maxLayerIndex = maxLayerIndices[maxPoolNum];
                x = customMaxPool(maxLayerIndex, &x);
                activationIndex ++;
                maxPoolNum ++;
                // std::cout << "after max Layer " << i << ":  shape: " << x.sizes() << std::endl;
                break;
            }

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