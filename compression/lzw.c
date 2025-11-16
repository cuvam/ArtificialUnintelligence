#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DICT_SIZE 4096  // 12-bit codes
#define INIT_DICT_SIZE 256  // ASCII characters

// Dictionary entry structure
typedef struct {
    unsigned char *string;
    int length;
    int code;
} DictEntry;

// Dictionary structure
typedef struct {
    DictEntry *entries;
    int size;
    int capacity;
} Dictionary;

// Initialize dictionary with single-byte entries
Dictionary* init_dictionary() {
    Dictionary *dict = (Dictionary*)malloc(sizeof(Dictionary));
    dict->capacity = MAX_DICT_SIZE;
    dict->entries = (DictEntry*)calloc(MAX_DICT_SIZE, sizeof(DictEntry));
    dict->size = INIT_DICT_SIZE;

    // Initialize with all single-byte values
    for (int i = 0; i < INIT_DICT_SIZE; i++) {
        dict->entries[i].string = (unsigned char*)malloc(1);
        dict->entries[i].string[0] = (unsigned char)i;
        dict->entries[i].length = 1;
        dict->entries[i].code = i;
    }

    return dict;
}

// Search for a string in the dictionary
int search_dictionary(Dictionary *dict, unsigned char *str, int len) {
    for (int i = 0; i < dict->size; i++) {
        if (dict->entries[i].length == len) {
            if (memcmp(dict->entries[i].string, str, len) == 0) {
                return dict->entries[i].code;
            }
        }
    }
    return -1;
}

// Add a new entry to the dictionary
int add_to_dictionary(Dictionary *dict, unsigned char *str, int len) {
    if (dict->size >= dict->capacity) {
        return -1;  // Dictionary full
    }

    int code = dict->size;
    dict->entries[code].string = (unsigned char*)malloc(len);
    memcpy(dict->entries[code].string, str, len);
    dict->entries[code].length = len;
    dict->entries[code].code = code;
    dict->size++;

    return code;
}

// Free dictionary memory
void free_dictionary(Dictionary *dict) {
    for (int i = 0; i < dict->size; i++) {
        free(dict->entries[i].string);
    }
    free(dict->entries);
    free(dict);
}

// Write a 12-bit code to output file
void write_code(FILE *output, int code, int *bit_buffer, int *bits_in_buffer) {
    *bit_buffer = (*bit_buffer << 12) | (code & 0xFFF);
    *bits_in_buffer += 12;

    // Write complete bytes
    while (*bits_in_buffer >= 8) {
        *bits_in_buffer -= 8;
        unsigned char byte = (*bit_buffer >> *bits_in_buffer) & 0xFF;
        fwrite(&byte, 1, 1, output);
    }
}

// Flush remaining bits
void flush_bits(FILE *output, int *bit_buffer, int *bits_in_buffer) {
    if (*bits_in_buffer > 0) {
        *bit_buffer <<= (8 - *bits_in_buffer);
        unsigned char byte = *bit_buffer & 0xFF;
        fwrite(&byte, 1, 1, output);
    }
}

// LZW compression function
int lzw_compress(const char *input_filename, const char *output_filename) {
    FILE *input = fopen(input_filename, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_filename);
        return 1;
    }

    FILE *output = fopen(output_filename, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", output_filename);
        fclose(input);
        return 1;
    }

    Dictionary *dict = init_dictionary();
    unsigned char buffer[MAX_DICT_SIZE];
    int buffer_len = 0;
    int c;

    int bit_buffer = 0;
    int bits_in_buffer = 0;

    // Read first byte
    if ((c = fgetc(input)) == EOF) {
        fclose(input);
        fclose(output);
        free_dictionary(dict);
        return 0;
    }

    buffer[buffer_len++] = (unsigned char)c;

    // Process the input file
    while ((c = fgetc(input)) != EOF) {
        buffer[buffer_len] = (unsigned char)c;

        // Check if current buffer + new byte exists in dictionary
        int code = search_dictionary(dict, buffer, buffer_len + 1);

        if (code != -1) {
            // String exists, extend buffer
            buffer_len++;
        } else {
            // String doesn't exist
            // Output code for current buffer
            int output_code = search_dictionary(dict, buffer, buffer_len);
            if (output_code != -1) {
                write_code(output, output_code, &bit_buffer, &bits_in_buffer);
            }

            // Add new string to dictionary
            buffer[buffer_len] = (unsigned char)c;
            add_to_dictionary(dict, buffer, buffer_len + 1);

            // Reset buffer to the new character
            buffer[0] = (unsigned char)c;
            buffer_len = 1;
        }
    }

    // Output code for remaining buffer
    if (buffer_len > 0) {
        int output_code = search_dictionary(dict, buffer, buffer_len);
        if (output_code != -1) {
            write_code(output, output_code, &bit_buffer, &bits_in_buffer);
        }
    }

    // Flush remaining bits
    flush_bits(output, &bit_buffer, &bits_in_buffer);

    fclose(input);
    fclose(output);
    free_dictionary(dict);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        printf("Compresses input_file using LZW algorithm and writes to output_file\n");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    printf("Compressing '%s' to '%s'...\n", input_file, output_file);

    if (lzw_compress(input_file, output_file) == 0) {
        // Get file sizes
        FILE *in = fopen(input_file, "rb");
        FILE *out = fopen(output_file, "rb");

        if (in && out) {
            fseek(in, 0, SEEK_END);
            long input_size = ftell(in);
            fseek(out, 0, SEEK_END);
            long output_size = ftell(out);
            fclose(in);
            fclose(out);

            printf("Compression complete!\n");
            printf("Input size:  %ld bytes\n", input_size);
            printf("Output size: %ld bytes\n", output_size);

            if (input_size > 0) {
                double ratio = (double)output_size / input_size * 100.0;
                printf("Compression ratio: %.2f%%\n", ratio);
            }
        }

        return 0;
    } else {
        fprintf(stderr, "Compression failed!\n");
        return 1;
    }
}
