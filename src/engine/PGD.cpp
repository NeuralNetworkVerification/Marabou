#include "PGD.h"
#include <InputQuery.h>
#include <iostream>

// Constructor
PGDAttack::PGDAttack(CustomDNNImpl &model, InputQuery &inputQuery)
    : model(model), inputQuery(inputQuery), device(DEFAULT_DEVICE_TYPE),
      alpha(DEFAULT_ALPHA), num_iter(DEFAULT_NUM_ITER), num_restarts(DEFAULT_NUM_RESTARTS), lambdaPenalty( PANELTY ) {
    inputSize = model.layerSizes.first();
    inputBounds = getBounds(INPUT);
    outputBounds = getBounds(OUTPUT);
    std::pair<torch::Tensor, torch::Tensor> variables = generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}

PGDAttack::PGDAttack(CustomDNNImpl &model, InputQuery &inputQuery, double alpha, unsigned num_iter,
                     unsigned num_restarts, torch::Device device, double lambdaPenalty)
    : model(model), inputQuery(inputQuery), device(device),
      alpha(alpha), num_iter(num_iter), num_restarts(num_restarts), lambdaPenalty( lambdaPenalty ) {
    inputSize = model.layerSizes.first();
    std::pair<Vector<double>, Vector<double>> inputBounds = getBounds(INPUT);
    std::pair<Vector<double>, Vector<double>> outputBounds = getBounds(OUTPUT);
    std::pair<torch::Tensor, torch::Tensor> variables = generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}

std::pair<Vector<double>, Vector<double>> PGDAttack::getBounds(unsigned type) {
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

std::pair<torch::Tensor, torch::Tensor> PGDAttack::generateSampleAndEpsilon() {
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
            epsilons[i] = std::numeric_limits<double>::infinity();   // todo choose a default value
        } else if (std::isinf(lower)) {
            // Only lower bound is infinite
            sample[i] = upper - 1.0f;  // todo choose a value slightly below upper bound
            epsilons[i] = 1.0f; // todo change to the same value as the decrease in upper
        } else if (std::isinf(upper)) {
            // Only upper bound is infinite
            sample[i] = lower + 1.0f;  // todo choose a value above lower bound
            epsilons[i] = 1.0f; // todo change to the same value as the increase in upper
        } else {
            // Both bounds are finite
            sample[i] = lower + (upper - lower) / 2.0f;
            epsilons[i] = (upper - lower) / 2.0f;
        }
    }

    return {torch::tensor(sample.getContainer()).unsqueeze(0).to(device),
            torch::tensor(epsilons.getContainer()).to(device)};
}

bool PGDAttack::isWithinBounds(const torch::Tensor& sample, unsigned type) {
    std::pair<Vector<double>, Vector<double>> bounds = getBounds(type);
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


torch::Tensor PGDAttack::calculateLoss(const torch::Tensor& predictions) {
    // Convert output bounds to tensors
    torch::Tensor lowerBoundTensor = torch::tensor(outputBounds.first.data(), torch::kFloat32).to(device);
    torch::Tensor upperBoundTensor = torch::tensor(outputBounds.second.data(), torch::kFloat32).to(device);

    // Compute the penalty: We want to penalize if predictions are inside the bounds
    torch::Tensor penalty = torch::add(
        torch::relu(predictions - upperBoundTensor), // Penalize if pred > upperBound
        torch::relu(lowerBoundTensor - predictions)  // Penalize if pred < lowerBound
    );

    return torch::sum(penalty);
}


torch::Tensor PGDAttack::findDelta() {
    torch::Tensor max_delta = torch::zeros_like(originalInput).to(device);
    torch::Tensor max_loss = torch::tensor(-std::numeric_limits<float>::infinity()).to(device);
    std::cout << "-----Starting num restarts-----" << std::endl;

    for (unsigned i = 0; i < num_restarts; i++) {
        torch::Tensor delta = torch::zeros_like(originalInput).uniform_(-1, 1).mul(epsilon).to(device);
        delta.set_requires_grad(true);

        torch::optim::Adam optimizer({delta}, torch::optim::AdamOptions(alpha));

        for (unsigned j = 0; j < num_iter; j++) {
            optimizer.zero_grad();

            torch::Tensor pred = model.forward(originalInput + delta);
            torch::Tensor loss = calculateLoss(pred);

            // Negate the loss for maximization
            (-loss).backward();

            // Step the optimizer
            optimizer.step();

            // Project delta back to epsilon-ball
            delta.data() = delta.data().clamp(-epsilon, epsilon);
        }

        torch::Tensor currentPrediction = model.forward(originalInput + delta);
        torch::Tensor current_loss = calculateLoss(currentPrediction);
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

    torch::Tensor adv_delta = findDelta();
    torch::Tensor adv_example = originalInput + adv_delta;

    auto adv_pred = model.forward(adv_example);

    auto loss = calculateLoss(adv_pred);

    std::cout << "-----displayAdversarialExample line 128-----" << std::endl;

    bool is_fooled = isWithinBounds( adv_example, INPUT ) &&  !isWithinBounds( adv_pred, OUTPUT );

    std::cout << "Original Prediction: " << original_pred
              << ", Adversarial Prediction: " << adv_pred
              << ", Model fooled: " << (is_fooled ? "Yes" : "No") << "\n";

    if (is_fooled) {
        std::cout << "Found a successful adversarial example:\n";
        std::cout << "Original Input: " << originalInput << "\n";
        std::cout << "Adversarial Example: " << adv_example << "\n";
        std::cout << "Epsilon used: " << adv_delta.abs().max().item<float>() << "\n";
        return true;
    }

    std::cout << "------ PGD attack failed ------"
              << "\n\n";
    return false;
}
