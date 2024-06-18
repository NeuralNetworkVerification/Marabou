#include "PGD.h"
#include <iostream>

namespace attack {

torch::Tensor findDelta(CustomDNNImpl& model, const torch::Tensor& X, int y, float epsilon, float alpha, int num_iter, torch::Device device) {
    torch::Tensor delta = torch::zeros_like(X).uniform_(-epsilon, epsilon).to(device);
    delta.set_requires_grad(true);  

    torch::Tensor target = torch::tensor({y}, torch::kInt64).to(device); 

    torch::optim::SGD optimizer(model.parameters(), torch::optim::SGDOptions(0.01).momentum(0.9));

    for (unsigned i = 0; i < num_iter; ++i) {
        optimizer.zero_grad();
        if (delta.grad().defined()) {
            delta.grad().zero_();
        }

        torch::Tensor pred = model.forward(X + delta);
        torch::Tensor loss = torch::nn::functional::cross_entropy(pred, target);
        loss.backward();

        delta = (delta + alpha * delta.grad().sign()).clamp(-epsilon, epsilon).detach();
        delta.set_requires_grad(true); 
    }

    return delta;
}

bool displayAdversarialExample(CustomDNNImpl& model, const torch::Tensor& input, int target, torch::Device device) {
    std::cout << "starting the attack" << std::endl;

    auto x = input.to(device);

    float epsilon = 0.1;
    float alpha = 0.01;
    int num_iter = 40;
    torch::Tensor adversarial_delta = findDelta(model, x, target, epsilon, alpha, num_iter, device);
    torch::Tensor adversarial_x = x + adversarial_delta;

    auto original_pred = model.forward(x).argmax(1);
    auto adversarial_pred = model.forward(adversarial_x).argmax(1);
    bool is_fooled = original_pred.item<int>() != adversarial_pred.item<int>();

    std::cout << "Original Prediction: " << original_pred.item<int>() << "\n";
    std::cout << "Adversarial Prediction: " << adversarial_pred.item<int>() << "\n";
    std::cout << "Model fooled: " << (is_fooled ? "Yes" : "No") << std::endl;

    return is_fooled;
}

} // namespace attack
