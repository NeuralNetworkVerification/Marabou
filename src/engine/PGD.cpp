#include "PGD.h"
#include <InputQuery.h>
#include <iostream>

PGDAttack::PGDAttack(CustomDNNImpl &model, const NLR::NetworkLevelReasoner* networkLevelReasoner)
    : model(model),  networkLevelReasoner( networkLevelReasoner ), device(torch::kCPU),
      learningRate(LR), num_iter(DEFAULT_NUM_ITER), num_restarts(DEFAULT_NUM_RESTARTS), penalty( PANELTY ) {
    inputSize = model.layerSizes.first();
    inputBounds = _getBounds(INPUT);
    outputBounds = _getBounds(OUTPUT);
    std::pair<torch::Tensor, torch::Tensor> variables = _generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}

PGDAttack::PGDAttack(CustomDNNImpl &model,
    const NLR::NetworkLevelReasoner* networkLevelReasoner, double learningRate, unsigned num_iter,
                     unsigned num_restarts, torch::Device device, double penalty)
    : model(model),  networkLevelReasoner( networkLevelReasoner ), device(device),
      learningRate(learningRate), num_iter(num_iter), num_restarts(num_restarts), penalty( penalty ) {
    inputSize = model.layerSizes.first();
    inputBounds = _getBounds(INPUT);
    outputBounds = _getBounds(OUTPUT);
    std::pair<torch::Tensor, torch::Tensor> variables = _generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}

torch::Device PGDAttack::_getDevice() {
    if (torch::cuda::is_available()) {
        std::cout << "CUDA is available. Using GPU." << std::endl;
        return {torch::kCUDA};
    } else {
        std::cout << "CUDA is not available. Using CPU." << std::endl;
        return {torch::kCPU};
    }
}

std::pair<Vector<double>, Vector<double>> PGDAttack::_getBounds(unsigned type ) const
{
    Vector<double> upperBounds;
    Vector<double> lowerBounds;
    unsigned layerIndex = type == INPUT ? 0 : networkLevelReasoner->getNumberOfLayers() -1;
    const NLR::Layer *layer = networkLevelReasoner->getLayer( layerIndex );

    for (unsigned neuron = 0 ; neuron < layer->getSize(); ++neuron) {
        double lowerBound = layer->getLb(neuron);
        double upperBound = layer->getUb(neuron);

        lowerBounds.append( lowerBound );
        upperBounds.append( upperBound );
    }
    return {lowerBounds, upperBounds};
}

std::pair<torch::Tensor, torch::Tensor> PGDAttack::_generateSampleAndEpsilon() {
    Vector<double> sample(inputSize, 0.0f);
    Vector<double> epsilons(inputSize, 0.0f);
    Vector<double> lowerBounds = inputBounds.first;
    Vector<double> upperBounds = inputBounds.second;

    for (unsigned i = 0; i < inputSize; i++) {
        double lower = lowerBounds.get(i);
        double upper = upperBounds.get(i);

        if (std::isinf(lower) && std::isinf(upper)) {
            // Both bounds are infinite
            sample[i] = 0.0f;
            epsilons[i] = std::numeric_limits<double>::infinity();
        } else if (std::isinf(lower)) {
            // Only lower bound is infinite
            sample[i] = upper - ATTACK_EPSILON;
            epsilons[i] = ATTACK_EPSILON;
        } else if (std::isinf(upper)) {
            // Only upper bound is infinite
            sample[i] = lower + ATTACK_EPSILON;
            epsilons[i] = ATTACK_EPSILON;
        } else {
            // Both bounds are finite
            sample[i] = lower + (upper - lower) / 2.0f;
            epsilons[i] = (upper - lower) / 2.0f;
        }
    }

    return {torch::tensor(sample.getContainer()).unsqueeze(0).to(device),
            torch::tensor(epsilons.getContainer()).to(device)};
}

bool PGDAttack::_isWithinBounds(const torch::Tensor& sample, unsigned type) {
    std::pair<Vector<double>, Vector<double>> bounds = _getBounds(type);
    Vector<double> lowerBounds = bounds.first;
    Vector<double> upperBounds = bounds.second;

    torch::Tensor flatInput = sample.view({-1});
    if (flatInput.numel() != lowerBounds.size() || flatInput.numel() != upperBounds.size()) {
        throw std::runtime_error("Mismatch in sizes of input and bounds");
    }

    for (int64_t i = 0; i < flatInput.size(0); i++) {
        double value = flatInput[i].item<double>();
        double lower = lowerBounds.get(i);
        double upper = upperBounds.get(i);

        if (std::isinf(lower) && std::isinf(upper)) {
            // If both bounds are infinite, any value is acceptable
            continue;
        } else if (std::isinf(lower)) {
            // Only check upper bound
            if (value > upper) return false;
        } else if (std::isinf(upper)) {
            // Only check lower bound
            if (value < lower) return false;
        } else {
            // Check both bounds
            if (value < lower || value > upper) return false;
        }
    }

    return true;
}


torch::Tensor PGDAttack::_calculateLoss(const torch::Tensor& predictions) {
    torch::Tensor lowerBoundTensor = torch::tensor(outputBounds.first.data(), torch::kFloat32).to(device);
    torch::Tensor upperBoundTensor = torch::tensor(outputBounds.second.data(), torch::kFloat32).to(device);

    // Compute the penalty: We want high loss if predictions are outside the bounds
    torch::Tensor total_violation = torch::add(
        torch::relu(predictions - upperBoundTensor),
        torch::relu(lowerBoundTensor - predictions));

    torch::Tensor smoothed_violation = torch::log1p(total_violation) * penalty;
    return torch::sum(smoothed_violation).pow( 2 );;
}


torch::Tensor PGDAttack::_findDelta() {
    torch::Tensor bestDelta = torch::zeros_like(originalInput).to(device);
    torch::Tensor minLoss = torch::tensor(std::numeric_limits<double>::infinity()).to(device);

    for (unsigned i = 0; i < num_restarts; i++) {
        torch::Tensor delta = torch::rand(inputSize).to(device);
        delta = delta.mul(epsilon);
        delta.set_requires_grad(true);

        torch::optim::Adam optimizer({delta},
            torch::optim::AdamOptions(learningRate));

        for (unsigned j = 0; j < num_iter; j++) {
            optimizer.zero_grad();

            torch::Tensor prediction = model.forward(originalInput + delta);
            torch::Tensor loss = _calculateLoss(prediction);
            loss.backward();
            optimizer.step();
            // Project delta back to epsilon-ball
            delta.data() = delta.data().clamp(-epsilon, epsilon);
        }

        torch::Tensor currentPrediction = model.forward(originalInput + delta);
        torch::Tensor currentLoss = _calculateLoss(currentPrediction);
        if (_isWithinBounds( currentPrediction, OUTPUT ))
        {
            return delta;
        }
        if (currentLoss.item<double>() < minLoss.item<double>()) {
            minLoss = currentLoss;
            bestDelta = delta.detach().clone();
        }

        learningRate *=2;
    }

    return bestDelta;
}

bool PGDAttack::displayAdversarialExample() {
    std::cout << "-----Starting PGD attack-----" << std::endl;
    model.eval();

    auto original_pred = model.forward(originalInput);

    torch::Tensor adv_delta = _findDelta();
    torch::Tensor advInput = originalInput + adv_delta;

    auto advPred = model.forward(advInput);

    bool isFooled = _isWithinBounds( advInput, INPUT ) &&
                    _isWithinBounds( advPred, OUTPUT );

    std::cout << "Input Lower Bounds : " << inputBounds.first.getContainer()<< "\n";
    std::cout << "Input Upper Bounds : " << inputBounds.second.getContainer() << "\n";
    std::cout << "Adversarial Input: \n";
    auto* example = advInput.data_ptr<float>();
    for (int i = 0; i < advInput.numel(); ++i) {
        std::cout << "x" << i << " = "<<  example[i] << "\n";
    }
    std::cout << "Output Lower Bounds : " << outputBounds.first.getContainer() << "\n";
    std::cout << "Output Upper Bounds : " << outputBounds.second.getContainer() << "\n";
    std::cout << "Adversarial Prediction: \n";
    auto* prediction = advPred.data_ptr<float>();
    for (int i = 0; i < advPred.numel(); ++i) {
        std::cout << "y" << i << " = "<<  prediction[i] << "\n";
    }
    std::cout << "Model fooled: " << (isFooled ? "Yes \n ------ PGD Attack Succeed ------" :
                                "No \n ------ PGD Attack Failed ------") << "\n\n";

    return isFooled;
}
