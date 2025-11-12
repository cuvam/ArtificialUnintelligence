#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "neural_network.h"

int main() {
    // Seed random number generator
    srand(time(NULL));

    printf("=== XOR Neural Network Example ===\n\n");
    printf("Training a neural network to learn the XOR function\n");
    printf("Architecture: 2 -> 4 -> 1 (2 input, 4 hidden, 1 output)\n\n");

    // Create training data for XOR
    // XOR truth table:
    // 0 XOR 0 = 0
    // 0 XOR 1 = 1
    // 1 XOR 0 = 1
    // 1 XOR 1 = 0

    int num_samples = 4;
    Matrix** inputs = (Matrix**)malloc(num_samples * sizeof(Matrix*));
    Matrix** targets = (Matrix**)malloc(num_samples * sizeof(Matrix*));

    // Input: [0, 0], Target: [0]
    inputs[0] = matrix_create(2, 1);
    matrix_set(inputs[0], 0, 0, 0.0);
    matrix_set(inputs[0], 1, 0, 0.0);
    targets[0] = matrix_create(1, 1);
    matrix_set(targets[0], 0, 0, 0.0);

    // Input: [0, 1], Target: [1]
    inputs[1] = matrix_create(2, 1);
    matrix_set(inputs[1], 0, 0, 0.0);
    matrix_set(inputs[1], 1, 0, 1.0);
    targets[1] = matrix_create(1, 1);
    matrix_set(targets[1], 0, 0, 1.0);

    // Input: [1, 0], Target: [1]
    inputs[2] = matrix_create(2, 1);
    matrix_set(inputs[2], 0, 0, 1.0);
    matrix_set(inputs[2], 1, 0, 0.0);
    targets[2] = matrix_create(1, 1);
    matrix_set(targets[2], 0, 0, 1.0);

    // Input: [1, 1], Target: [0]
    inputs[3] = matrix_create(2, 1);
    matrix_set(inputs[3], 0, 0, 1.0);
    matrix_set(inputs[3], 1, 0, 1.0);
    targets[3] = matrix_create(1, 1);
    matrix_set(targets[3], 0, 0, 0.0);

    // Create neural network
    // Architecture: 2 inputs -> 4 hidden neurons -> 1 output
    NeuralNetwork* nn = nn_create(2);
    nn_add_layer(nn, 0, 2, 4, ACTIVATION_SIGMOID);  // Hidden layer
    nn_add_layer(nn, 1, 4, 1, ACTIVATION_SIGMOID);  // Output layer
    nn->learning_rate = 0.5;

    printf("Training for 1000 epochs...\n\n");

    // Train the network
    nn_train(nn, inputs, targets, num_samples, 1000);

    printf("\n=== Testing the trained network ===\n\n");

    // Test the network
    for (int i = 0; i < num_samples; i++) {
        Matrix* output = nn_forward(nn, inputs[i]);
        printf("Input: [%.0f, %.0f] -> Output: %.4f (Target: %.0f)\n",
               matrix_get(inputs[i], 0, 0),
               matrix_get(inputs[i], 1, 0),
               matrix_get(output, 0, 0),
               matrix_get(targets[i], 0, 0));
    }

    printf("\n=== Results Analysis ===\n");
    printf("The network should output values close to 0 or 1.\n");
    printf("Typically, values < 0.5 are classified as 0, and >= 0.5 as 1.\n\n");

    // Cleanup
    for (int i = 0; i < num_samples; i++) {
        matrix_free(inputs[i]);
        matrix_free(targets[i]);
    }
    free(inputs);
    free(targets);
    nn_free(nn);

    return 0;
}
