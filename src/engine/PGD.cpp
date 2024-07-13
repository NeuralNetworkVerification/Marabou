#include "PGD.h"
#include <InputQuery.h>
#include <iostream>

PGDAttack::PGDAttack(CustomDNNImpl &model, InputQuery &inputQuery)
    : model(model), inputQuery(inputQuery), device(torch::kCPU),
      alpha(DEFAULT_ALPHA), num_iter(DEFAULT_NUM_ITER), num_restarts(DEFAULT_NUM_RESTARTS), lambdaPenalty( PANELTY ) {
    inputSize = model.layerSizes.first();
    inputBounds = _getBounds(INPUT);
    outputBounds = _getBounds(OUTPUT);
    std::pair<torch::Tensor, torch::Tensor> variables = _generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}

PGDAttack::PGDAttack(CustomDNNImpl &model, InputQuery &inputQuery, double alpha, unsigned num_iter,
                     unsigned num_restarts, torch::Device device, double lambdaPenalty)
    : model(model), inputQuery(inputQuery), device(device),
      alpha(alpha), num_iter(num_iter), num_restarts(num_restarts), lambdaPenalty( lambdaPenalty ) {
    inputSize = model.layerSizes.first();
    std::pair<Vector<double>, Vector<double>> inputBounds = _getBounds(INPUT);
    std::pair<Vector<double>, Vector<double>> outputBounds = _getBounds(OUTPUT);
    std::pair<torch::Tensor, torch::Tensor> variables = _generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}

torch::Device PGDAttack::getDevice() {
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

    List<unsigned> variables = type == INPUT ? inputQuery.getInputVariables() : inputQuery.getOutputVariables();

    for (auto it = variables.begin(); it != variables.end(); ++it) {
        double upperBound = inputQuery.getUpperBounds().exists(*it)
                                ? inputQuery.getUpperBounds().get(*it)
                                : std::numeric_limits<double>::infinity();
        double lowerBound = inputQuery.getLowerBounds().exists(*it)
                                ? inputQuery.getLowerBounds().get(*it)
                                : -std::numeric_limits<double>::infinity();

        upperBounds.append(upperBound);
        lowerBounds.append(lowerBound);
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
    // Convert output bounds to tensors
    torch::Tensor lowerBoundTensor = torch::tensor(outputBounds.first.data(), torch::kFloat32).to(device);
    torch::Tensor upperBoundTensor = torch::tensor(outputBounds.second.data(), torch::kFloat32).to(device);

    // Compute the penalty: We want to penalize if predictions are inside the bounds
    torch::Tensor penalty = torch::add(
        - torch::relu(predictions - upperBoundTensor), // Penalize if pred > upperBound
        - torch::relu(lowerBoundTensor - predictions)  // Penalize if pred < lowerBound
    );

    return torch::sum(penalty);
}


torch::Tensor PGDAttack::_findDelta() {
    torch::Tensor max_delta = torch::zeros_like(originalInput).to(device);
    torch::Tensor max_loss = torch::tensor(-std::numeric_limits<float>::infinity()).to(device);

    for (unsigned i = 0; i < num_restarts; i++) {
        torch::Tensor delta = torch::rand(inputSize).to(device);
        delta = delta.mul(epsilon);
        delta.set_requires_grad(true);

        torch::optim::Adam optimizer({delta}, torch::optim::AdamOptions(alpha));

        for (unsigned j = 0; j < num_iter; j++) {
            optimizer.zero_grad();

            torch::Tensor pred = model.forward(originalInput + delta);
            torch::Tensor loss = _calculateLoss(pred);

            // Negate the loss for maximization
            (-loss).backward();

            // Step the optimizer
            optimizer.step();

            // Project delta back to epsilon-ball
            delta.data() = delta.data().clamp(-epsilon, epsilon);
        }

        torch::Tensor currentPrediction = model.forward(originalInput + delta);
        torch::Tensor current_loss = _calculateLoss(currentPrediction);
        if (current_loss.item<float>() > max_loss.item<float>()) {
            max_loss = current_loss;
            max_delta = delta.detach().clone();
        }
    }

    return max_delta;
}

bool PGDAttack::displayAdversarialExample() {
    std::cout << "-----Starting PGD attack-----" << std::endl;
    model.eval();

    auto original_pred = model.forward(originalInput);

    torch::Tensor adv_delta = _findDelta();
    torch::Tensor adv_example = originalInput + adv_delta;

    auto adv_pred = model.forward(adv_example);

    auto loss = _calculateLoss(adv_pred);


    bool isFooled = _isWithinBounds( adv_example, INPUT ) &&  !_isWithinBounds( adv_pred, OUTPUT );

    std::cout << "Input Lower Bounds : " << inputBounds.first.getContainer() << "\n";
    std::cout << "Input Upper Bounds : " << inputBounds.second.getContainer() << "\n\n";
    std::cout << "Original Input: " << originalInput << "\n";
    std::cout << "Adversarial Example: " << adv_example << "\n\n";
    std::cout << "Output Lower Bounds : " << outputBounds.first.getContainer() << "\n";
    std::cout << "Output Lower Bounds : " << outputBounds.second.getContainer() << "\n";
    std::cout << "Original Prediction: " << original_pred << "\n";
    std::cout << ", Adversarial Prediction: " << adv_pred << "\n\n";
    std::cout << "Model fooled: " << (isFooled ? "Yes \n ------ PGD Attack Succeed ------" :
                                "No \n ------ PGD Attack Failed ------") << "\n";

    return isFooled;
}
