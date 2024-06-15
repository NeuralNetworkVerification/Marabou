#include "pgd_attack.h"
#include <torch/torch.h>
#include "read_net.h"

PGD_Linf_Rand::PGD_Linf_Rand(CustomDNN model, torch::Device device)
    : model(std::move(model)), device(device),
      optimizer(this->model->parameters(), torch::optim::SGDOptions(0.01).momentum(0.9)) {}

torch::Tensor PGD_Linf_Rand::pgd_linf_rand(const torch::Tensor& X, int y, float epsilon, float alpha, int num_iter) {
    torch::Tensor delta = torch::zeros_like(X).uniform_(-epsilon, epsilon).to(device);
    delta.set_requires_grad(true);  

    torch::Tensor target = torch::tensor({y}, torch::kInt64).to(device); 

    for (int i = 0; i < num_iter; ++i) {
        // Clear gradients
        optimizer.zero_grad();
        if (delta.grad().defined()) {
            delta.grad().zero_();
        }

        torch::Tensor pred = model->forward(X + delta);
        torch::Tensor loss = torch::nn::functional::cross_entropy(pred, target);
        loss.backward();

        // Update delta
        delta = (delta + alpha * delta.grad().sign()).clamp(-epsilon, epsilon).detach();
        delta.set_requires_grad(true); 
    }

    return delta;
}

bool PGD_Linf_Rand::display_adversarial_example(const torch::Tensor& input, int target) {
    std::cout<<"starting the attack"<< std::endl;

    auto x = input.to(device);

    float epsilon = 0.1;
    float alpha = 0.01;
    int num_iter = 40;
    torch::Tensor adversarial_delta = pgd_linf_rand(x, target, epsilon, alpha, num_iter);
    torch::Tensor adversarial_x = x + adversarial_delta;

    // Check if the model is fooled
    auto original_pred = model->forward(x).argmax(1);
    auto adversarial_pred = model->forward(adversarial_x).argmax(1);
    bool is_fooled = original_pred.item<int>() != adversarial_pred.item<int>();

    std::cout << "Original Prediction: " << original_pred.item<int>() << "\n";
    std::cout << "Adversarial Prediction: " << adversarial_pred.item<int>() << "\n";
    std::cout << "Model fooled: " << (is_fooled ? "Yes" : "No") << std::endl;

    return is_fooled;
}

bool main(std::string filename) {
    // filepath "/home/maya-swisa/Documents/Lab/PGD_Linf_Rand/example_dnn_model.nnet";
    std::vector<int> layers;
    std::vector<std::string> activations;
    std::vector<std::vector<std::vector<float>>> weights;
    std::vector<std::vector<float>> biases;

    load_nnet(filename, layers, activations, weights, biases);

    CustomDNN model(layers, activations, weights, biases);

    torch::Device device(torch::kCPU);
    
    torch::Tensor input = torch::tensor({{0.1300, -0.6401,  0.3475, -0.0579, -0.9246,  0.7567,  0.2141, -1.1153, 0.8332,  0.0851}}); 
    int target = 0;  

    PGD_Linf_Rand pgd_attack(model, device);

    // Display adversarial example
    return display_adversarial_example(input, target);

}


