#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "neural_network.h"

// Function to convert a number to binary and store in matrix starting at given row
void number_to_binary(Matrix* m, int row_start, int number, int bits) {
    for (int i = 0; i < bits; i++) {
        matrix_set(m, row_start + i, 0, (number >> i) & 1 ? 1.0 : 0.0);
    }
}

// Function to convert binary matrix to number
int binary_to_number(Matrix* m, int row_start, int bits) {
    int result = 0;
    for (int i = 0; i < bits; i++) {
        if (matrix_get(m, row_start + i, 0) >= 0.5) {
            result |= (1 << i);
        }
    }
    return result;
}

// Generate training data for 8-bit addition
void generate_training_data(Matrix*** inputs, Matrix*** targets, int* num_samples) {
    // Generate a diverse set of training examples
    // We'll use a subset of all possible combinations to keep training manageable
    // Focus on: small numbers, medium numbers, large numbers, and edge cases

    int capacity = 1000;
    *inputs = (Matrix**)malloc(capacity * sizeof(Matrix*));
    *targets = (Matrix**)malloc(capacity * sizeof(Matrix*));
    int count = 0;

    srand(42); // Fixed seed for reproducibility

    // Add systematic examples for small numbers (0-31)
    for (int a = 0; a < 32 && count < capacity; a++) {
        for (int b = 0; b < 32 && count < capacity; b += 4) {
            int sum = a + b;
            int carry = sum >> 8;
            sum = sum & 0xFF;

            (*inputs)[count] = matrix_create(16, 1);
            (*targets)[count] = matrix_create(9, 1);

            number_to_binary((*inputs)[count], 0, a, 8);
            number_to_binary((*inputs)[count], 8, b, 8);
            number_to_binary((*targets)[count], 0, sum, 8);
            matrix_set((*targets)[count], 8, 0, carry ? 1.0 : 0.0);

            count++;
        }
    }

    // Add examples around powers of 2 and their neighbors
    int special_values[] = {0, 1, 2, 4, 8, 16, 32, 64, 127, 128, 255};
    int num_special = 11;
    for (int i = 0; i < num_special && count < capacity; i++) {
        for (int j = 0; j < num_special && count < capacity; j++) {
            int a = special_values[i];
            int b = special_values[j];
            int sum = a + b;
            int carry = sum >> 8;
            sum = sum & 0xFF;

            (*inputs)[count] = matrix_create(16, 1);
            (*targets)[count] = matrix_create(9, 1);

            number_to_binary((*inputs)[count], 0, a, 8);
            number_to_binary((*inputs)[count], 8, b, 8);
            number_to_binary((*targets)[count], 0, sum, 8);
            matrix_set((*targets)[count], 8, 0, carry ? 1.0 : 0.0);

            count++;
        }
    }

    // Add random examples for better coverage
    for (int i = 0; i < 500 && count < capacity; i++) {
        int a = rand() % 256;
        int b = rand() % 256;
        int sum = a + b;
        int carry = sum >> 8;
        sum = sum & 0xFF;

        (*inputs)[count] = matrix_create(16, 1);
        (*targets)[count] = matrix_create(9, 1);

        number_to_binary((*inputs)[count], 0, a, 8);
        number_to_binary((*inputs)[count], 8, b, 8);
        number_to_binary((*targets)[count], 0, sum, 8);
        matrix_set((*targets)[count], 8, 0, carry ? 1.0 : 0.0);

        count++;
    }

    *num_samples = count;
}

int main() {
    srand(time(NULL));

    printf("=== 8-Bit Adder Neural Network ===\n\n");
    printf("Training a neural network to perform 8-bit binary addition\n");
    printf("Input: Two 8-bit numbers (16 bits total)\n");
    printf("Output: 8-bit sum + 1 carry bit (9 bits total)\n");
    printf("Architecture: 16 -> 32 -> 16 -> 9\n\n");

    // Generate training data
    Matrix** inputs;
    Matrix** targets;
    int num_samples;

    printf("Generating training data...\n");
    generate_training_data(&inputs, &targets, &num_samples);
    printf("Generated %d training examples\n\n", num_samples);

    // Create neural network
    // Architecture: 16 inputs -> 32 hidden -> 16 hidden -> 9 outputs
    NeuralNetwork* nn = nn_create(3);
    nn_add_layer(nn, 0, 16, 32, ACTIVATION_SIGMOID);  // First hidden layer
    nn_add_layer(nn, 1, 32, 16, ACTIVATION_SIGMOID);  // Second hidden layer
    nn_add_layer(nn, 2, 16, 9, ACTIVATION_SIGMOID);   // Output layer
    nn->learning_rate = 0.3;

    printf("Training for 5000 epochs...\n");
    printf("This may take a moment...\n\n");

    // Train the network
    nn_train(nn, inputs, targets, num_samples, 5000);

    printf("\n=== Testing the trained 8-bit adder ===\n\n");

    // Test cases
    int test_cases[][2] = {
        {0, 0},
        {1, 1},
        {5, 3},
        {15, 15},
        {128, 64},
        {200, 100},
        {255, 0},
        {255, 1},
        {127, 128},
        {255, 255}
    };
    int num_tests = 10;

    int correct = 0;
    for (int i = 0; i < num_tests; i++) {
        int a = test_cases[i][0];
        int b = test_cases[i][1];
        int expected_sum = (a + b) & 0xFF;
        int expected_carry = (a + b) >> 8;

        Matrix* input = matrix_create(16, 1);
        number_to_binary(input, 0, a, 8);
        number_to_binary(input, 8, b, 8);

        Matrix* output = nn_forward(nn, input);

        int predicted_sum = binary_to_number(output, 0, 8);
        int predicted_carry = matrix_get(output, 8, 0) >= 0.5 ? 1 : 0;

        int is_correct = (predicted_sum == expected_sum && predicted_carry == expected_carry);
        if (is_correct) correct++;

        printf("Test %2d: %3d + %3d = %3d (carry: %d)\n",
               i + 1, a, b, expected_sum, expected_carry);
        printf("         Predicted: %3d (carry: %d) %s\n",
               predicted_sum, predicted_carry, is_correct ? "✓" : "✗");

        // Show bit-by-bit output for first few tests
        if (i < 3) {
            printf("         Output bits: ");
            for (int bit = 0; bit < 9; bit++) {
                printf("%.2f ", matrix_get(output, bit, 0));
            }
            printf("\n");
        }
        printf("\n");

        matrix_free(input);
    }

    printf("=== Results ===\n");
    printf("Accuracy: %d/%d correct (%.1f%%)\n\n", correct, num_tests, (correct * 100.0) / num_tests);

    // Additional random tests
    printf("=== Additional Random Tests ===\n");
    srand(time(NULL));
    int random_correct = 0;
    int num_random = 20;

    for (int i = 0; i < num_random; i++) {
        int a = rand() % 256;
        int b = rand() % 256;
        int expected_sum = (a + b) & 0xFF;
        int expected_carry = (a + b) >> 8;

        Matrix* input = matrix_create(16, 1);
        number_to_binary(input, 0, a, 8);
        number_to_binary(input, 8, b, 8);

        Matrix* output = nn_forward(nn, input);

        int predicted_sum = binary_to_number(output, 0, 8);
        int predicted_carry = matrix_get(output, 8, 0) >= 0.5 ? 1 : 0;

        int is_correct = (predicted_sum == expected_sum && predicted_carry == expected_carry);
        if (is_correct) random_correct++;

        if (i < 5 || !is_correct) {
            printf("%3d + %3d = %3d (c:%d) | Predicted: %3d (c:%d) %s\n",
                   a, b, expected_sum, expected_carry,
                   predicted_sum, predicted_carry,
                   is_correct ? "✓" : "✗");
        }

        matrix_free(input);
    }

    printf("\nRandom test accuracy: %d/%d correct (%.1f%%)\n\n",
           random_correct, num_random, (random_correct * 100.0) / num_random);

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
