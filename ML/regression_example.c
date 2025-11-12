#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "neural_network.h"

int main() {
    // Seed random number generator
    srand(time(NULL));

    printf("=== Regression Neural Network Example ===\n\n");
    printf("Training a neural network to learn f(x) = x^2\n");
    printf("Architecture: 1 -> 8 -> 8 -> 1 (1 input, 2 hidden layers with 8 neurons, 1 output)\n\n");

    // Create training data for f(x) = x^2
    int num_samples = 20;
    Matrix** inputs = (Matrix**)malloc(num_samples * sizeof(Matrix*));
    Matrix** targets = (Matrix**)malloc(num_samples * sizeof(Matrix*));

    printf("Generating training data:\n");
    for (int i = 0; i < num_samples; i++) {
        // Generate x values from -1 to 1
        double x = -1.0 + (2.0 * i) / (num_samples - 1);
        double y = x * x; // Target: x^2

        inputs[i] = matrix_create(1, 1);
        matrix_set(inputs[i], 0, 0, x);

        targets[i] = matrix_create(1, 1);
        matrix_set(targets[i], 0, 0, y);

        if (i % 5 == 0) {
            printf("  x = %.2f, y = %.4f\n", x, y);
        }
    }

    // Create neural network
    NeuralNetwork* nn = nn_create(3);
    nn_add_layer(nn, 0, 1, 8, ACTIVATION_TANH);     // Hidden layer 1
    nn_add_layer(nn, 1, 8, 8, ACTIVATION_TANH);     // Hidden layer 2
    nn_add_layer(nn, 2, 8, 1, ACTIVATION_LINEAR);   // Output layer
    nn->learning_rate = 0.01;

    printf("\nTraining for 2000 epochs...\n\n");

    // Train the network
    nn_train(nn, inputs, targets, num_samples, 2000);

    printf("\n=== Testing the trained network ===\n\n");

    // Test the network on training data
    printf("Testing on training data:\n");
    double total_error = 0.0;
    for (int i = 0; i < num_samples; i += 4) {
        Matrix* output = nn_forward(nn, inputs[i]);
        double x = matrix_get(inputs[i], 0, 0);
        double predicted = matrix_get(output, 0, 0);
        double actual = matrix_get(targets[i], 0, 0);
        double error = fabs(predicted - actual);
        total_error += error;

        printf("  x = %.2f: Predicted = %.4f, Actual = %.4f, Error = %.4f\n",
               x, predicted, actual, error);
    }

    printf("\n=== Testing on new data ===\n\n");
    printf("Testing on unseen data:\n");

    // Test on new data points
    double test_x[] = {-0.75, -0.25, 0.25, 0.75};
    int num_tests = 4;

    for (int i = 0; i < num_tests; i++) {
        Matrix* test_input = matrix_create(1, 1);
        matrix_set(test_input, 0, 0, test_x[i]);

        Matrix* output = nn_forward(nn, test_input);
        double predicted = matrix_get(output, 0, 0);
        double actual = test_x[i] * test_x[i];
        double error = fabs(predicted - actual);

        printf("  x = %.2f: Predicted = %.4f, Actual = %.4f, Error = %.4f\n",
               test_x[i], predicted, actual, error);

        matrix_free(test_input);
    }

    printf("\n=== Results Analysis ===\n");
    printf("The network learns to approximate the quadratic function f(x) = x^2.\n");
    printf("Errors should be small, indicating successful function approximation.\n\n");

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
