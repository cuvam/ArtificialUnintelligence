#ifndef NEURAL_NETWORK_H
#define NEURAL_NETWORK_H

#include "matrix.h"

// Activation function types
typedef enum {
    ACTIVATION_SIGMOID,
    ACTIVATION_TANH,
    ACTIVATION_RELU,
    ACTIVATION_LINEAR
} ActivationType;

// Layer structure
typedef struct {
    int input_size;
    int output_size;
    Matrix* weights;
    Matrix* biases;
    ActivationType activation;

    // Cache for backpropagation
    Matrix* input;
    Matrix* z;  // Pre-activation
    Matrix* a;  // Post-activation (output)
    Matrix* dW; // Weight gradients
    Matrix* db; // Bias gradients
    Matrix* delta; // Error term
} Layer;

// Neural Network structure
typedef struct {
    int num_layers;
    Layer** layers;
    double learning_rate;
} NeuralNetwork;

// Activation functions and their derivatives
double sigmoid(double x);
double sigmoid_derivative(double x);
double tanh_activation(double x);
double tanh_derivative(double x);
double relu(double x);
double relu_derivative(double x);
double linear(double x);
double linear_derivative(double x);

// Apply activation functions to matrices
void apply_activation(Matrix* m, ActivationType type);
void apply_activation_derivative(Matrix* m, ActivationType type);

// Layer operations
Layer* layer_create(int input_size, int output_size, ActivationType activation);
void layer_free(Layer* layer);
Matrix* layer_forward(Layer* layer, Matrix* input);

// Neural Network operations
NeuralNetwork* nn_create(int num_layers);
void nn_free(NeuralNetwork* nn);
void nn_add_layer(NeuralNetwork* nn, int index, int input_size, int output_size, ActivationType activation);
Matrix* nn_forward(NeuralNetwork* nn, Matrix* input);
void nn_backward(NeuralNetwork* nn, Matrix* input, Matrix* target);
void nn_update_weights(NeuralNetwork* nn);

// Loss functions
double mse_loss(Matrix* predicted, Matrix* target);
Matrix* mse_loss_derivative(Matrix* predicted, Matrix* target);

// Training
void nn_train(NeuralNetwork* nn, Matrix** inputs, Matrix** targets, int num_samples, int epochs);

#endif
