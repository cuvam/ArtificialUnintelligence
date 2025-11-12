#ifndef JSON_PARSER_H
#define JSON_PARSER_H

typedef struct {
    char* text;
    double label;
} TrainingExample;

typedef struct {
    TrainingExample* examples;
    int count;
} TrainingDataset;

// Load training data from JSON file
TrainingDataset* load_training_data(const char* filename);

// Free training dataset
void free_training_dataset(TrainingDataset* dataset);

#endif
