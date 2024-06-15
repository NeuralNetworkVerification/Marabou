import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset



def generate_nonlinear_data(num_samples, input_size):
    x = torch.randn(num_samples, input_size)
    y = torch.zeros(num_samples, dtype=torch.long)
    for i in range(num_samples):
        if torch.sum(x[i] ** 2) > input_size * 0.5:  
            y[i] = 1
    return x, y


def train_model(model, dataloader, criterion, optimizer, num_epochs):
    for epoch in range(num_epochs):
        running_loss = 0.0
        for inputs, labels in dataloader:
            optimizer.zero_grad()
            outputs = model(inputs)
            loss = criterion(outputs, labels)
            loss.backward()
            optimizer.step()
            running_loss += loss.item()

        avg_loss = running_loss / len(dataloader)
        print(f'Epoch [{epoch + 1}/{num_epochs}], Loss: {avg_loss:.4f}')

    print("Training complete")


class example_DNN(nn.Module):
    def __init__(self, input_size, num_classes):
        super(example_DNN, self).__init__()
        self.fc1 = nn.Linear(input_size, 128)
        self.relu1 = nn.ReLU()
        self.fc2 = nn.Linear(128, 64)
        self.relu2 = nn.ReLU()
        self.fc3 = nn.Linear(64, 32)
        self.relu3 = nn.ReLU()
        self.fc4 = nn.Linear(32, num_classes)

    def forward(self, x):
        out = self.fc1(x)
        out = self.relu1(out)
        out = self.fc2(out)
        out = self.relu2(out)
        out = self.fc3(out)
        out = self.relu3(out)
        out = self.fc4(out)
        return out

def save_to_nnet(model, input_size, output_size, filename, input_mins=None, input_maxs=None, input_means=None, input_ranges=None, output_mins=None, output_maxs=None):
    layers = [input_size]
    biases = []
    weights = []
    activations = []

    if input_mins is None:
        input_mins = [0.0] * input_size
    if input_maxs is None:
        input_maxs = [1.0] * input_size
    if input_means is None:
        input_means = [0.0] * input_size
    if input_ranges is None:
        input_ranges = [1.0] * input_size
    if output_mins is None:
        output_mins = [-0.1] * output_size
    if output_maxs is None:
        output_maxs = [1.0] * output_size

    # Mapping from PyTorch activation functions to .nnet format
    activation_map = {
        nn.ReLU: 'ReLU',
        nn.Sigmoid: 'Sigmoid',
        nn.Tanh: 'Tanh',
        nn.Identity: 'Linear',  # no activation
        nn.Softmax: 'Softmax', 
    }

    for layer in model.children():
        if isinstance(layer, nn.Linear):
            layers.append(layer.out_features)
            biases.append(layer.bias.detach().numpy())
            weights.append(layer.weight.detach().numpy())
            activations.append('Linear')  
        elif type(layer) in activation_map:
            activations[-1] = activation_map[type(layer)]
    
    with open(filename, 'w') as f:
        # Write the header information
        f.write(f"{len(layers) - 1}\n")  # NumLayers
        f.write(' '.join(map(str, layers)) + '\n') 
        f.write(' '.join(activations) + '\n')  
        f.write(' '.join(map(str, input_mins)) + '\n')  
        f.write(' '.join(map(str, input_maxs)) + '\n')  
        f.write(' '.join(map(str, input_means)) + '\n')  
        f.write(' '.join(map(str, input_ranges)) + '\n')
        f.write(' '.join(map(str, output_mins)) + '\n')  
        f.write(' '.join(map(str, output_maxs)) + '\n')  

        # Write weights and biases for each layer
        for w, b in zip(weights, biases):
            for row in w:  # Weights
                f.write(' '.join(map(str, row)) + '\n')
            f.write(' '.join(map(str, b)) + '\n')  # Biases

if __name__ == "__main__":
    input_size = 10
    num_classes = 2
    num_samples = 100000
    num_epochs = 20
    batch_size = 32

    x_train, y_train = generate_nonlinear_data(num_samples, input_size)
    dataset = TensorDataset(x_train, y_train)
    dataloader = DataLoader(dataset, batch_size=batch_size, shuffle=True)

    model = example_DNN(input_size, num_classes)
    criterion = nn.CrossEntropyLoss()
    optimizer = optim.Adam(model.parameters(), lr=0.001)
    train_model(model, dataloader, criterion, optimizer, num_epochs)

    example_index = torch.randint(0, num_samples, (1,)).item()
    example_input = x_train[example_index]
    true_label = y_train[example_index]
    model.eval()

    # predict
    with torch.no_grad():
        example_output = model(example_input.unsqueeze(0))
        predicted_label = torch.argmax(example_output, dim=1).item()

    print(f'Example input: {example_input}')
    print(f'True label: {true_label}')
    print(f'Predicted label: {predicted_label}')

    # Save to .nnet file 
    save_to_nnet(model, input_size, num_classes, 'example_dnn_model.nnet')
    print('Model saved as example_dnn_model.nnet')
    input_size = 10  # Assuming input size is 10 as in the example
    num_samples = 1  # Generate only one sample
    x, y = generate_nonlinear_data(num_samples, input_size)
    print(f"sample: {x}, true label: {y}")
