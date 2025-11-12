#include "json_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 2048
#define MAX_TEXT_LENGTH 1024

// Skip whitespace in string
char* skip_whitespace(char* str) {
    while (*str && isspace(*str)) str++;
    return str;
}

// Extract string value between quotes (skips field name, gets value)
char* extract_string(char* str, char* output, int max_len) {
    // Find the colon (separator between field name and value)
    char* colon = strchr(str, ':');
    if (!colon) return NULL;

    // Find the opening quote after the colon
    char* start = strchr(colon, '"');
    if (!start) return NULL;
    start++; // Skip opening quote

    char* end = start;
    int len = 0;

    // Find closing quote, handling escaped quotes
    while (*end && *end != '"' && len < max_len - 1) {
        if (*end == '\\' && *(end + 1) == '"') {
            output[len++] = '"';
            end += 2;
        } else {
            output[len++] = *end++;
        }
    }

    output[len] = '\0';
    return end + 1; // Return position after closing quote
}

// Extract double value
double extract_double(char* str) {
    char* colon = strchr(str, ':');
    if (!colon) return 0.0;
    return atof(colon + 1);
}

// Count number of training examples in file
int count_examples(FILE* file) {
    int count = 0;
    char line[MAX_LINE_LENGTH];

    rewind(file);

    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, "\"text\"")) {
            count++;
        }
    }

    rewind(file);
    return count;
}

TrainingDataset* load_training_data(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        return NULL;
    }

    // Count examples first
    int num_examples = count_examples(file);
    if (num_examples == 0) {
        fprintf(stderr, "Error: No training examples found in file\n");
        fclose(file);
        return NULL;
    }

    // Allocate dataset
    TrainingDataset* dataset = (TrainingDataset*)malloc(sizeof(TrainingDataset));
    dataset->count = num_examples;
    dataset->examples = (TrainingExample*)malloc(num_examples * sizeof(TrainingExample));

    // Parse file
    char line[MAX_LINE_LENGTH];
    int example_index = 0;
    char current_text[MAX_TEXT_LENGTH] = "";
    double current_label = 0.0;
    int has_text = 0;
    int has_label = 0;

    while (fgets(line, sizeof(line), file)) {
        char* trimmed = skip_whitespace(line);

        // Check for "text" field
        if (strstr(trimmed, "\"text\"")) {
            char text_buffer[MAX_TEXT_LENGTH];
            if (extract_string(trimmed, text_buffer, MAX_TEXT_LENGTH)) {
                strcpy(current_text, text_buffer);
                has_text = 1;
            }
        }

        // Check for "label" field
        if (strstr(trimmed, "\"label\"")) {
            current_label = extract_double(trimmed);
            has_label = 1;
        }

        // When we have both text and label, create an example
        if (has_text && has_label && example_index < num_examples) {
            dataset->examples[example_index].text = strdup(current_text);
            dataset->examples[example_index].label = current_label;
            example_index++;
            has_text = 0;
            has_label = 0;
        }
    }

    fclose(file);

    if (example_index != num_examples) {
        fprintf(stderr, "Warning: Expected %d examples but parsed %d\n",
                num_examples, example_index);
        dataset->count = example_index;
    }

    return dataset;
}

void free_training_dataset(TrainingDataset* dataset) {
    if (!dataset) return;

    for (int i = 0; i < dataset->count; i++) {
        free(dataset->examples[i].text);
    }

    free(dataset->examples);
    free(dataset);
}
