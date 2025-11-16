#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_DICT_SIZE 4096  // 12-bit codes
#define INIT_DICT_SIZE 256  // ASCII characters
#define HASH_TABLE_SIZE 8192  // Power of 2 for fast modulo
#define MAX_STRING_LEN 256
#define BUFFER_SIZE 65536  // 64KB buffer for I/O

// Hash table entry for fast lookups
typedef struct HashEntry {
    unsigned char *string;
    int length;
    int code;
    struct HashEntry *next;  // For collision chaining
} HashEntry;

// Dictionary structure with hash table
typedef struct {
    unsigned char *string_pool;  // Pre-allocated memory pool
    int pool_offset;
    int size;
    int capacity;
    HashEntry **hash_table;
    HashEntry *entries;  // Pre-allocated entries
} Dictionary;

// Fast hash function (FNV-1a)
static inline uint32_t hash_string(const unsigned char *str, int len) {
    uint32_t hash = 2166136261u;
    for (int i = 0; i < len; i++) {
        hash ^= str[i];
        hash *= 16777619u;
    }
    return hash;
}

// Initialize dictionary with hash table
Dictionary* init_dictionary() {
    Dictionary *dict = (Dictionary*)malloc(sizeof(Dictionary));
    dict->capacity = MAX_DICT_SIZE;
    dict->size = INIT_DICT_SIZE;
    dict->pool_offset = 0;

    // Pre-allocate string pool
    dict->string_pool = (unsigned char*)malloc(MAX_DICT_SIZE * MAX_STRING_LEN);

    // Pre-allocate entries
    dict->entries = (HashEntry*)calloc(MAX_DICT_SIZE, sizeof(HashEntry));

    // Initialize hash table
    dict->hash_table = (HashEntry**)calloc(HASH_TABLE_SIZE, sizeof(HashEntry*));

    // Initialize with all single-byte values
    for (int i = 0; i < INIT_DICT_SIZE; i++) {
        dict->entries[i].string = &dict->string_pool[dict->pool_offset];
        dict->entries[i].string[0] = (unsigned char)i;
        dict->entries[i].length = 1;
        dict->entries[i].code = i;
        dict->entries[i].next = NULL;
        dict->pool_offset += 1;

        // Add to hash table
        uint32_t hash = hash_string(dict->entries[i].string, 1);
        int idx = hash & (HASH_TABLE_SIZE - 1);
        dict->entries[i].next = dict->hash_table[idx];
        dict->hash_table[idx] = &dict->entries[i];
    }

    return dict;
}

// Fast dictionary search using hash table
static inline int search_dictionary(Dictionary *dict, const unsigned char *str, int len) {
    uint32_t hash = hash_string(str, len);
    int idx = hash & (HASH_TABLE_SIZE - 1);

    HashEntry *entry = dict->hash_table[idx];
    while (entry) {
        if (entry->length == len && memcmp(entry->string, str, len) == 0) {
            return entry->code;
        }
        entry = entry->next;
    }
    return -1;
}

// Add a new entry to the dictionary with hash table
static inline int add_to_dictionary(Dictionary *dict, const unsigned char *str, int len) {
    if (dict->size >= dict->capacity || dict->pool_offset + len > MAX_DICT_SIZE * MAX_STRING_LEN) {
        return -1;  // Dictionary full
    }

    int code = dict->size;
    HashEntry *new_entry = &dict->entries[code];

    // Use pre-allocated pool
    new_entry->string = &dict->string_pool[dict->pool_offset];
    memcpy(new_entry->string, str, len);
    new_entry->length = len;
    new_entry->code = code;
    dict->pool_offset += len;

    // Add to hash table
    uint32_t hash = hash_string(str, len);
    int idx = hash & (HASH_TABLE_SIZE - 1);
    new_entry->next = dict->hash_table[idx];
    dict->hash_table[idx] = new_entry;

    dict->size++;
    return code;
}

// Free dictionary memory
void free_dictionary(Dictionary *dict) {
    free(dict->string_pool);
    free(dict->entries);
    free(dict->hash_table);
    free(dict);
}

// Write a 12-bit code to output file (with buffering)
static inline void write_code(unsigned char **output_buffer, int *buf_pos,
                              int code, int *bit_buffer, int *bits_in_buffer) {
    *bit_buffer = (*bit_buffer << 12) | (code & 0xFFF);
    *bits_in_buffer += 12;

    // Write complete bytes to buffer
    while (*bits_in_buffer >= 8) {
        *bits_in_buffer -= 8;
        (*output_buffer)[(*buf_pos)++] = (*bit_buffer >> *bits_in_buffer) & 0xFF;
    }
}

// Flush remaining bits
static inline void flush_bits(unsigned char *output_buffer, int *buf_pos,
                              int *bit_buffer, int *bits_in_buffer) {
    if (*bits_in_buffer > 0) {
        *bit_buffer <<= (8 - *bits_in_buffer);
        output_buffer[(*buf_pos)++] = *bit_buffer & 0xFF;
    }
}

// LZW compression function with buffered I/O
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

    // Allocate buffers
    unsigned char *input_buffer = (unsigned char*)malloc(BUFFER_SIZE);
    unsigned char *output_buffer = (unsigned char*)malloc(BUFFER_SIZE);
    unsigned char string_buffer[MAX_STRING_LEN];

    Dictionary *dict = init_dictionary();
    int string_len = 0;
    int bit_buffer = 0;
    int bits_in_buffer = 0;
    int output_pos = 0;

    // Read input in chunks
    size_t bytes_read;
    int first_byte = 1;

    while ((bytes_read = fread(input_buffer, 1, BUFFER_SIZE, input)) > 0) {
        for (size_t i = 0; i < bytes_read; i++) {
            unsigned char c = input_buffer[i];

            if (first_byte) {
                string_buffer[string_len++] = c;
                first_byte = 0;
                continue;
            }

            string_buffer[string_len] = c;

            // Check if current string + new byte exists in dictionary
            int code = search_dictionary(dict, string_buffer, string_len + 1);

            if (code != -1) {
                // String exists, extend buffer
                string_len++;
            } else {
                // String doesn't exist - output code for current string
                int output_code = search_dictionary(dict, string_buffer, string_len);
                if (output_code != -1) {
                    write_code(&output_buffer, &output_pos, output_code,
                              &bit_buffer, &bits_in_buffer);

                    // Flush output buffer if needed
                    if (output_pos >= BUFFER_SIZE - 16) {
                        fwrite(output_buffer, 1, output_pos, output);
                        output_pos = 0;
                    }
                }

                // Add new string to dictionary
                string_buffer[string_len] = c;
                add_to_dictionary(dict, string_buffer, string_len + 1);

                // Reset string to the new character
                string_buffer[0] = c;
                string_len = 1;
            }
        }
    }

    // Output code for remaining string
    if (string_len > 0) {
        int output_code = search_dictionary(dict, string_buffer, string_len);
        if (output_code != -1) {
            write_code(&output_buffer, &output_pos, output_code,
                      &bit_buffer, &bits_in_buffer);
        }
    }

    // Flush remaining bits
    flush_bits(output_buffer, &output_pos, &bit_buffer, &bits_in_buffer);

    // Write remaining output
    if (output_pos > 0) {
        fwrite(output_buffer, 1, output_pos, output);
    }

    free(input_buffer);
    free(output_buffer);
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
