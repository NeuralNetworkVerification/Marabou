#include "PGD.h"
#include <iostream>

// Constructor
PGDAttack::PGDAttack(CustomDNNImpl &model, const Map<unsigned, double> &lowerBounds,
                     const Map<unsigned, double> &upperBounds)
    : model(model), lowerBounds(lowerBounds), upperBounds(upperBounds), device(DEFAULT_DEVICE_TYPE),
      alpha(DEFAULT_ALPHA), num_iter(DEFAULT_NUM_ITER), num_restarts(DEFAULT_NUM_RESTARTS) {
    inputSize = model.layerSizes.first();
    std::pair<torch::Tensor, torch::Tensor> variables = generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;

}


PGDAttack::PGDAttack(CustomDNNImpl &model, const Map<unsigned, double> &lowerBounds,
                     const Map<unsigned, double> &upperBounds, double alpha, unsigned num_iter,
                     unsigned num_restarts, torch::Device device)
    : model(model), lowerBounds(lowerBounds), upperBounds(upperBounds), device(device),
      alpha(alpha), num_iter(num_iter), num_restarts(num_restarts){
    inputSize = model.layerSizes.first();
    std::pair<torch::Tensor, torch::Tensor> variables = generateSampleAndEpsilon();
    originalInput = variables.first;
    epsilon = variables.second;
}


std::pair<torch::Tensor, torch::Tensor> PGDAttack::generateSampleAndEpsilon() {
    Vector<double> sample(inputSize, 0.0f);
    Vector<double> epsilons(inputSize, 0.0f);
    for (unsigned j = 0; j < inputSize; ++j) {
        double lowerBound = lowerBounds.exists(j) ? lowerBounds.at(j) : -std::numeric_limits<double>::infinity();
        double upperBound = upperBounds.exists(j) ? upperBounds.at(j) : std::numeric_limits<double>::infinity();
        sample[j] = lowerBound + (upperBound - lowerBound) / 2.0f;
        epsilons[j] = (upperBound - lowerBound) / 2.0f;
    }
    return {torch::tensor(sample.getContainer()).unsqueeze(0).to(device),
            torch::tensor(epsilons.getContainer()).to(device)};
}

torch::Tensor PGDAttack::findDelta(int originalPred) {
    torch::Tensor max_delta = torch::zeros_like(originalInput).to(device);
    torch::Tensor target = torch::tensor({ originalPred }, torch::kInt64).to(device);
    torch::Tensor max_loss = torch::tensor(-std::numeric_limits<float>::infinity()).to(device);

    for (unsigned i = 0; i < num_restarts; i++) {
        torch::Tensor delta = torch::zeros_like(originalInput).uniform_(-1, 1).mul(epsilon).to(device);
        delta.set_requires_grad(true);

        for (unsigned j = 0; j < num_iter; j++) {
            if (delta.grad().defined()) {
                delta.grad().zero_();
            }

            torch::Tensor pred = model.forward(originalInput + delta);
            torch::Tensor loss = torch::nn::functional::cross_entropy(pred, target);
            loss.backward();

            torch::Tensor alpha_tensor = torch::tensor(alpha).to(device);
            // Update delta
            delta.data().add_(alpha_tensor * delta.grad().sign()).clamp_(-epsilon, epsilon);
        }
        torch::Tensor current_loss = torch::nn::functional::cross_entropy(model.forward(originalInput + delta), target);
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

    int original_pred = model.forward(originalInput).argmax(1).item<int>();

    torch::Tensor adversarial_delta = findDelta(original_pred);
    torch::Tensor adversarial_x = originalInput + adversarial_delta;

    auto adversarial_pred = model.forward(adversarial_x).argmax(1);
    bool is_fooled = original_pred != adversarial_pred.item<int>();

    std::cout << "Original Prediction: " << original_pred
              << ", Adversarial Prediction: " << adversarial_pred.item<int>()
              << ", Model fooled: " << (is_fooled ? "Yes" : "No") << "\n";

    if (is_fooled) {
        std::cout << "Found a successful adversarial example:\n";
        std::cout << "Original Input: " << originalInput << "\n";
        std::cout << "Adversarial Example: " << adversarial_x << "\n";
        std::cout << "Epsilon used: " << adversarial_delta.abs().max().item<float>() << "\n";
        return true;
    }

    std::cout << "------ PGD attack failed ------" << "\n\n";
    return false;
}