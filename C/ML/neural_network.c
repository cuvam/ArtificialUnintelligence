#include "neural_network.h"
#include <math.h>

// Activation functions
double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

double sigmoid_derivative(double x) {
    double s = sigmoid(x);
    return s * (1.0 - s);
}

double tanh_activation(double x) {
    return tanh(x);
}

double tanh_derivative(double x) {
    double t = tanh(x);
    return 1.0 - t * t;
}

double relu(double x) {
    return x > 0 ? x : 0;
}

double relu_derivative(double x) {
    return x > 0 ? 1.0 : 0.0;
}

double linear(double x) {
    return x;
}

double linear_derivative(double x) {
    return 1.0;
}

// Apply activation functions to matrices
void apply_activation(Matrix* m, ActivationType type) {
    switch(type) {
        case ACTIVATION_SIGMOID:
            matrix_map(m, sigmoid);
            break;
        case ACTIVATION_TANH:
            matrix_map(m, tanh_activation);
            break;
        case ACTIVATION_RELU:
            matrix_map(m, relu);
            break;
        case ACTIVATION_LINEAR:
            matrix_map(m, linear);
            break;
    }
}

void apply_activation_derivative(Matrix* m, ActivationType type) {
    switch(type) {
        case ACTIVATION_SIGMOID:
            matrix_map(m, sigmoid_derivative);
            break;
        case ACTIVATION_TANH:
            matrix_map(m, tanh_derivative);
            break;
        case ACTIVATION_RELU:
            matrix_map(m, relu_derivative);
            break;
        case ACTIVATION_LINEAR:
            matrix_map(m, linear_derivative);
            break;
    }
}

// Layer operations
Layer* layer_create(int input_size, int output_size, ActivationType activation) {
    Layer* layer = (Layer*)malloc(sizeof(Layer));
    layer->input_size = input_size;
    layer->output_size = output_size;
    layer->activation = activation;

    // Initialize weights and biases
    layer->weights = matrix_create(output_size, input_size);
    layer->biases = matrix_create(output_size, 1);

    // Xavier/He initialization
    double limit = sqrt(2.0 / input_size);
    matrix_randomize(layer->weights, -limit, limit);
    matrix_fill(layer->biases, 0.0);

    // Initialize cache
    layer->input = NULL;
    layer->z = NULL;
    layer->a = NULL;
    layer->dW = NULL;
    layer->db = NULL;
    layer->delta = NULL;

    return layer;
}

void layer_free(Layer* layer) {
    if (layer == NULL) return;
    matrix_free(layer->weights);
    matrix_free(layer->biases);
    if (layer->input) matrix_free(layer->input);
    if (layer->z) matrix_free(layer->z);
    if (layer->a) matrix_free(layer->a);
    if (layer->dW) matrix_free(layer->dW);
    if (layer->db) matrix_free(layer->db);
    if (layer->delta) matrix_free(layer->delta);
    free(layer);
}

Matrix* layer_forward(Layer* layer, Matrix* input) {
    // Save input for backpropagation
    if (layer->input) matrix_free(layer->input);
    layer->input = matrix_copy(input);

    // z = W * x + b
    Matrix* wx = matrix_multiply(layer->weights, input);
    if (layer->z) matrix_free(layer->z);
    layer->z = matrix_add(wx, layer->biases);
    matrix_free(wx);

    // a = activation(z)
    if (layer->a) matrix_free(layer->a);
    layer->a = matrix_copy(layer->z);
    apply_activation(layer->a, layer->activation);

    return layer->a;
}

// Neural Network operations
NeuralNetwork* nn_create(int num_layers) {
    NeuralNetwork* nn = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
    nn->num_layers = num_layers;
    nn->layers = (Layer**)malloc(num_layers * sizeof(Layer*));
    for (int i = 0; i < num_layers; i++) {
        nn->layers[i] = NULL;
    }
    nn->learning_rate = 0.01;
    return nn;
}

void nn_free(NeuralNetwork* nn) {
    if (nn == NULL) return;
    for (int i = 0; i < nn->num_layers; i++) {
        layer_free(nn->layers[i]);
    }
    free(nn->layers);
    free(nn);
}

void nn_add_layer(NeuralNetwork* nn, int index, int input_size, int output_size, ActivationType activation) {
    if (index >= 0 && index < nn->num_layers) {
        nn->layers[index] = layer_create(input_size, output_size, activation);
    }
}

Matrix* nn_forward(NeuralNetwork* nn, Matrix* input) {
    Matrix* current = input;

    for (int i = 0; i < nn->num_layers; i++) {
        current = layer_forward(nn->layers[i], current);
    }

    return current;
}

void nn_backward(NeuralNetwork* nn, Matrix* input, Matrix* target) {
    // Compute output error
    Layer* output_layer = nn->layers[nn->num_layers - 1];
    Matrix* output_error = matrix_subtract(output_layer->a, target);

    // Compute output layer delta
    Matrix* output_derivative = matrix_copy(output_layer->z);
    apply_activation_derivative(output_derivative, output_layer->activation);

    if (output_layer->delta) matrix_free(output_layer->delta);
    output_layer->delta = matrix_hadamard(output_error, output_derivative);
    matrix_free(output_error);
    matrix_free(output_derivative);

    // Backpropagate through hidden layers
    for (int i = nn->num_layers - 1; i >= 0; i--) {
        Layer* layer = nn->layers[i];

        // Compute gradients
        Matrix* input_T = matrix_transpose(layer->input);
        Matrix* delta_T = matrix_transpose(layer->delta);

        if (layer->dW) matrix_free(layer->dW);
        layer->dW = matrix_multiply(layer->delta, input_T);

        if (layer->db) matrix_free(layer->db);
        layer->db = matrix_copy(layer->delta);

        matrix_free(input_T);
        matrix_free(delta_T);

        // Propagate error to previous layer
        if (i > 0) {
            Layer* prev_layer = nn->layers[i - 1];
            Matrix* weights_T = matrix_transpose(layer->weights);
            Matrix* prev_error = matrix_multiply(weights_T, layer->delta);

            Matrix* prev_derivative = matrix_copy(prev_layer->z);
            apply_activation_derivative(prev_derivative, prev_layer->activation);

            if (prev_layer->delta) matrix_free(prev_layer->delta);
            prev_layer->delta = matrix_hadamard(prev_error, prev_derivative);

            matrix_free(weights_T);
            matrix_free(prev_error);
            matrix_free(prev_derivative);
        }
    }
}

void nn_update_weights(NeuralNetwork* nn) {
    for (int i = 0; i < nn->num_layers; i++) {
        Layer* layer = nn->layers[i];

        // Update weights: W = W - learning_rate * dW
        Matrix* weight_update = matrix_multiply_scalar(layer->dW, nn->learning_rate);
        Matrix* new_weights = matrix_subtract(layer->weights, weight_update);
        matrix_free(layer->weights);
        layer->weights = new_weights;
        matrix_free(weight_update);

        // Update biases: b = b - learning_rate * db
        Matrix* bias_update = matrix_multiply_scalar(layer->db, nn->learning_rate);
        Matrix* new_biases = matrix_subtract(layer->biases, bias_update);
        matrix_free(layer->biases);
        layer->biases = new_biases;
        matrix_free(bias_update);
    }
}

// Loss functions
double mse_loss(Matrix* predicted, Matrix* target) {
    double sum = 0.0;
    for (int i = 0; i < predicted->rows; i++) {
        for (int j = 0; j < predicted->cols; j++) {
            double diff = predicted->data[i][j] - target->data[i][j];
            sum += diff * diff;
        }
    }
    return sum / (predicted->rows * predicted->cols);
}

Matrix* mse_loss_derivative(Matrix* predicted, Matrix* target) {
    return matrix_subtract(predicted, target);
}

// Training
void nn_train(NeuralNetwork* nn, Matrix** inputs, Matrix** targets, int num_samples, int epochs) {
    for (int epoch = 0; epoch < epochs; epoch++) {
        double total_loss = 0.0;

        for (int i = 0; i < num_samples; i++) {
            // Forward pass
            Matrix* output = nn_forward(nn, inputs[i]);

            // Compute loss
            double loss = mse_loss(output, targets[i]);
            total_loss += loss;

            // Backward pass
            nn_backward(nn, inputs[i], targets[i]);

            // Update weights
            nn_update_weights(nn);
        }

        if ((epoch + 1) % 100 == 0 || epoch == 0) {
            printf("Epoch %d/%d - Loss: %.6f\n", epoch + 1, epochs, total_loss / num_samples);
        }
    }
}
