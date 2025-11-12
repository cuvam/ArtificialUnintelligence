# ML Framework in C

A lightweight machine learning framework implemented in C, featuring matrix operations, neural networks, and backpropagation.

## Features

### Matrix Operations Library (`matrix.h`, `matrix.c`)
- Matrix creation, destruction, and copying
- Basic operations: addition, subtraction, multiplication
- Scalar multiplication
- Transpose
- Hadamard (element-wise) product
- Element access and modification
- Random initialization

### Neural Network Framework (`neural_network.h`, `neural_network.c`)
- Multi-layer perceptron (MLP) architecture
- Activation functions:
  - Sigmoid
  - Tanh
  - ReLU
  - Linear
- Forward propagation
- Backpropagation with gradient descent
- Mean Squared Error (MSE) loss function
- Flexible layer configuration

## Architecture

```
Input Layer → Hidden Layer(s) → Output Layer
     ↓              ↓                 ↓
  Matrix      Activation         Activation
Operations    Functions          Functions
```

## Building the Examples

```bash
make all
```

This will compile:
- `xor_example` - XOR classification problem
- `regression_example` - Function approximation (f(x) = x²)

## Running the Examples

### XOR Example
```bash
./xor_example
```

Trains a neural network to learn the XOR logical operation:
- Architecture: 2 → 4 → 1
- Activation: Sigmoid
- Training: 1000 epochs

### Regression Example
```bash
./regression_example
```

Trains a neural network to approximate f(x) = x²:
- Architecture: 1 → 8 → 8 → 1
- Activation: Tanh (hidden), Linear (output)
- Training: 2000 epochs

## Usage

### Creating a Neural Network

```c
#include "neural_network.h"

// Create a network with 2 layers
NeuralNetwork* nn = nn_create(2);

// Add hidden layer: 2 inputs → 4 neurons, sigmoid activation
nn_add_layer(nn, 0, 2, 4, ACTIVATION_SIGMOID);

// Add output layer: 4 inputs → 1 neuron, sigmoid activation
nn_add_layer(nn, 1, 4, 1, ACTIVATION_SIGMOID);

// Set learning rate
nn->learning_rate = 0.5;
```

### Training the Network

```c
// Prepare training data
Matrix** inputs = ...; // Array of input matrices
Matrix** targets = ...; // Array of target matrices
int num_samples = 4;
int epochs = 1000;

// Train
nn_train(nn, inputs, targets, num_samples, epochs);
```

### Making Predictions

```c
Matrix* input = matrix_create(2, 1);
matrix_set(input, 0, 0, 1.0);
matrix_set(input, 1, 0, 0.0);

Matrix* output = nn_forward(nn, input);
double prediction = matrix_get(output, 0, 0);
```

### Cleanup

```c
nn_free(nn);
matrix_free(input);
```

## API Reference

### Matrix Operations
- `Matrix* matrix_create(int rows, int cols)` - Create new matrix
- `void matrix_free(Matrix* m)` - Free matrix memory
- `Matrix* matrix_multiply(Matrix* a, Matrix* b)` - Matrix multiplication
- `Matrix* matrix_add(Matrix* a, Matrix* b)` - Element-wise addition
- `void matrix_print(Matrix* m)` - Print matrix

### Neural Network
- `NeuralNetwork* nn_create(int num_layers)` - Create network
- `void nn_add_layer(...)` - Add layer to network
- `Matrix* nn_forward(NeuralNetwork* nn, Matrix* input)` - Forward pass
- `void nn_train(...)` - Train network
- `void nn_free(NeuralNetwork* nn)` - Free network

## Implementation Details

- **Weight Initialization**: Xavier/He initialization for better convergence
- **Optimization**: Stochastic Gradient Descent (SGD)
- **Loss Function**: Mean Squared Error (MSE)
- **Backpropagation**: Full implementation with gradient computation

## Limitations

- Currently supports only fully connected (dense) layers
- Single optimization algorithm (SGD)
- No regularization (L1/L2)
- No dropout or batch normalization
- No GPU acceleration

## Future Enhancements

- Additional optimizers (Adam, RMSprop)
- Convolutional layers
- Regularization techniques
- Cross-entropy loss
- Mini-batch training
- Model serialization

## Clean Up

```bash
make clean
```

Removes all compiled objects and executables.
