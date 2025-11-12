#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "neural_network.h"

#define MAX_VOCAB_SIZE 100
#define MAX_TEXT_LENGTH 1000
#define MAX_WORD_LENGTH 50

// Vocabulary for sentiment analysis
typedef struct {
    char** words;
    int size;
} Vocabulary;

// Initialize vocabulary with common sentiment words
Vocabulary* create_vocabulary() {
    Vocabulary* vocab = (Vocabulary*)malloc(sizeof(Vocabulary));
    vocab->size = 30;
    vocab->words = (char**)malloc(vocab->size * sizeof(char*));

    // Positive sentiment words
    vocab->words[0] = strdup("good");
    vocab->words[1] = strdup("great");
    vocab->words[2] = strdup("excellent");
    vocab->words[3] = strdup("amazing");
    vocab->words[4] = strdup("wonderful");
    vocab->words[5] = strdup("fantastic");
    vocab->words[6] = strdup("love");
    vocab->words[7] = strdup("best");
    vocab->words[8] = strdup("perfect");
    vocab->words[9] = strdup("awesome");
    vocab->words[10] = strdup("happy");
    vocab->words[11] = strdup("like");
    vocab->words[12] = strdup("enjoy");
    vocab->words[13] = strdup("beautiful");
    vocab->words[14] = strdup("nice");

    // Negative sentiment words
    vocab->words[15] = strdup("bad");
    vocab->words[16] = strdup("terrible");
    vocab->words[17] = strdup("awful");
    vocab->words[18] = strdup("horrible");
    vocab->words[19] = strdup("worst");
    vocab->words[20] = strdup("hate");
    vocab->words[21] = strdup("poor");
    vocab->words[22] = strdup("disappointing");
    vocab->words[23] = strdup("useless");
    vocab->words[24] = strdup("boring");
    vocab->words[25] = strdup("sad");
    vocab->words[26] = strdup("angry");
    vocab->words[27] = strdup("disgusting");
    vocab->words[28] = strdup("ugly");
    vocab->words[29] = strdup("dislike");

    return vocab;
}

void free_vocabulary(Vocabulary* vocab) {
    for (int i = 0; i < vocab->size; i++) {
        free(vocab->words[i]);
    }
    free(vocab->words);
    free(vocab);
}

// Convert text to lowercase
void to_lowercase(char* text) {
    for (int i = 0; text[i]; i++) {
        text[i] = tolower(text[i]);
    }
}

// Create bag-of-words feature vector from text
Matrix* text_to_features(const char* text, Vocabulary* vocab) {
    Matrix* features = matrix_create(vocab->size, 1);
    matrix_fill(features, 0.0);

    // Create a copy of text for tokenization
    char* text_copy = strdup(text);
    to_lowercase(text_copy);

    // Tokenize and count word occurrences
    char* token = strtok(text_copy, " .,!?;:\n\t");
    while (token != NULL) {
        // Check if token matches any vocabulary word
        for (int i = 0; i < vocab->size; i++) {
            if (strcmp(token, vocab->words[i]) == 0) {
                // Increment count for this word
                double current = matrix_get(features, i, 0);
                matrix_set(features, i, 0, current + 1.0);
                break;
            }
        }
        token = strtok(NULL, " .,!?;:\n\t");
    }

    // Normalize features (divide by total word count)
    double total = 0.0;
    for (int i = 0; i < vocab->size; i++) {
        total += matrix_get(features, i, 0);
    }

    if (total > 0) {
        for (int i = 0; i < vocab->size; i++) {
            double val = matrix_get(features, i, 0);
            matrix_set(features, i, 0, val / total);
        }
    }

    free(text_copy);
    return features;
}

int main() {
    srand(time(NULL));

    printf("=== Sentiment Analysis Neural Network ===\n\n");

    // Create vocabulary
    Vocabulary* vocab = create_vocabulary();
    printf("Vocabulary size: %d words\n", vocab->size);
    printf("Positive words (0-14), Negative words (15-29)\n\n");

    // Training data: text samples with sentiment labels
    // Label: 1.0 = positive, 0.0 = negative

    const char* training_texts[] = {
        // Positive examples
        "This is a great product I love it",
        "Excellent quality very happy with purchase",
        "Amazing experience wonderful service",
        "Best thing ever so good",
        "Fantastic quality I enjoy using it",
        "Beautiful design and perfect functionality",
        "Great value awesome product",
        "Love this excellent choice",
        "Wonderful experience very nice",
        "Happy with this amazing product",

        // Negative examples
        "This is terrible I hate it",
        "Awful quality very disappointing",
        "Horrible experience worst ever",
        "Bad product poor design",
        "Useless and boring waste of money",
        "Disgusting quality I dislike it",
        "Worst purchase ever so bad",
        "Terrible experience very sad",
        "Awful service hate this",
        "Poor quality disappointing product"
    };

    double training_labels[] = {
        1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0,  // Positive
        0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0   // Negative
    };

    int num_samples = 20;

    // Convert training data to feature matrices
    Matrix** inputs = (Matrix**)malloc(num_samples * sizeof(Matrix*));
    Matrix** targets = (Matrix**)malloc(num_samples * sizeof(Matrix*));

    printf("Converting training data to features...\n");
    for (int i = 0; i < num_samples; i++) {
        inputs[i] = text_to_features(training_texts[i], vocab);
        targets[i] = matrix_create(1, 1);
        matrix_set(targets[i], 0, 0, training_labels[i]);
    }

    // Create neural network
    // Architecture: vocab_size (30) -> 16 -> 8 -> 1
    printf("\nCreating neural network...\n");
    printf("Architecture: %d -> 16 -> 8 -> 1\n\n", vocab->size);

    NeuralNetwork* nn = nn_create(3);
    nn_add_layer(nn, 0, vocab->size, 16, ACTIVATION_RELU);
    nn_add_layer(nn, 1, 16, 8, ACTIVATION_RELU);
    nn_add_layer(nn, 2, 8, 1, ACTIVATION_SIGMOID);
    nn->learning_rate = 0.1;

    // Train the network
    printf("Training for 500 epochs...\n\n");
    nn_train(nn, inputs, targets, num_samples, 500);

    printf("\n=== Testing on Training Data ===\n\n");

    // Test on training data
    int correct = 0;
    for (int i = 0; i < num_samples; i++) {
        Matrix* output = nn_forward(nn, inputs[i]);
        double prediction = matrix_get(output, 0, 0);
        double actual = training_labels[i];
        int predicted_class = prediction >= 0.5 ? 1 : 0;
        int actual_class = actual >= 0.5 ? 1 : 0;

        if (predicted_class == actual_class) correct++;

        if (i % 5 == 0) {
            printf("Sample %d: \"%s\"\n", i, training_texts[i]);
            printf("  Prediction: %.4f (%s), Actual: %.0f (%s)\n\n",
                   prediction,
                   predicted_class ? "POSITIVE" : "NEGATIVE",
                   actual,
                   actual_class ? "POSITIVE" : "NEGATIVE");
        }
    }

    printf("Training Accuracy: %d/%d (%.1f%%)\n\n", correct, num_samples,
           100.0 * correct / num_samples);

    // Test on new examples
    printf("=== Testing on New Data ===\n\n");

    const char* test_texts[] = {
        "I love this product it is wonderful",
        "This is the worst thing ever",
        "Great experience very happy",
        "Terrible quality I hate it",
        "Amazing and beautiful",
        "Awful and disappointing"
    };

    const char* expected[] = {
        "POSITIVE", "NEGATIVE", "POSITIVE", "NEGATIVE", "POSITIVE", "NEGATIVE"
    };

    int num_tests = 6;
    correct = 0;

    for (int i = 0; i < num_tests; i++) {
        Matrix* test_input = text_to_features(test_texts[i], vocab);
        Matrix* output = nn_forward(nn, test_input);
        double prediction = matrix_get(output, 0, 0);
        int predicted_class = prediction >= 0.5 ? 1 : 0;

        printf("Text: \"%s\"\n", test_texts[i]);
        printf("  Prediction: %.4f (%s)\n", prediction,
               predicted_class ? "POSITIVE" : "NEGATIVE");
        printf("  Expected: %s\n", expected[i]);

        if ((predicted_class == 1 && strcmp(expected[i], "POSITIVE") == 0) ||
            (predicted_class == 0 && strcmp(expected[i], "NEGATIVE") == 0)) {
            correct++;
            printf("  ✓ CORRECT\n\n");
        } else {
            printf("  ✗ INCORRECT\n\n");
        }

        matrix_free(test_input);
    }

    printf("Test Accuracy: %d/%d (%.1f%%)\n\n", correct, num_tests,
           100.0 * correct / num_tests);

    printf("=== Try Your Own Text ===\n");
    printf("Example usage to classify custom text:\n");
    printf("  Matrix* features = text_to_features(\"your text here\", vocab);\n");
    printf("  Matrix* output = nn_forward(nn, features);\n");
    printf("  double sentiment = matrix_get(output, 0, 0);\n");
    printf("  // sentiment >= 0.5: positive, < 0.5: negative\n\n");

    // Cleanup
    for (int i = 0; i < num_samples; i++) {
        matrix_free(inputs[i]);
        matrix_free(targets[i]);
    }
    free(inputs);
    free(targets);
    free_vocabulary(vocab);
    nn_free(nn);

    return 0;
}
