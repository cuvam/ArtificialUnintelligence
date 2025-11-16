#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// DEFLATE constants
#define WINDOW_SIZE 32768      // 32KB sliding window
#define MIN_MATCH 3            // Minimum match length
#define MAX_MATCH 258          // Maximum match length
#define HASH_BITS 15
#define HASH_SIZE (1 << HASH_BITS)
#define HASH_MASK (HASH_SIZE - 1)
#define MAX_DIST 32768         // Maximum distance for LZ77

// Huffman coding constants
#define MAX_BITS 15
#define LITERALS 256           // Number of literal bytes
#define LENGTH_CODES 29        // Number of length codes
#define DIST_CODES 30          // Number of distance codes
#define MAX_CODES (LITERALS + 1 + LENGTH_CODES)  // 286 codes

// Bit buffer structure
typedef struct {
    uint32_t bits;
    int count;
    unsigned char *output;
    size_t pos;
    size_t capacity;
} BitBuffer;

// Huffman tree node
typedef struct HuffNode {
    int freq;
    int symbol;
    struct HuffNode *left;
    struct HuffNode *right;
} HuffNode;

// Huffman code
typedef struct {
    uint16_t code;
    uint8_t length;
} HuffCode;

// LZ77 token
typedef struct {
    int literal;      // -1 if this is a length/distance pair
    int length;
    int distance;
} LZ77Token;

// Initialize bit buffer
BitBuffer* init_bitbuffer(size_t capacity) {
    BitBuffer *bb = (BitBuffer*)malloc(sizeof(BitBuffer));
    bb->bits = 0;
    bb->count = 0;
    bb->output = (unsigned char*)malloc(capacity);
    bb->pos = 0;
    bb->capacity = capacity;
    return bb;
}

// Write bits to buffer (LSB first for DEFLATE)
void write_bits(BitBuffer *bb, uint32_t bits, int count) {
    bb->bits |= (bits << bb->count);
    bb->count += count;

    while (bb->count >= 8) {
        if (bb->pos >= bb->capacity) {
            bb->capacity *= 2;
            bb->output = (unsigned char*)realloc(bb->output, bb->capacity);
        }
        bb->output[bb->pos++] = bb->bits & 0xFF;
        bb->bits >>= 8;
        bb->count -= 8;
    }
}

// Flush remaining bits
void flush_bits(BitBuffer *bb) {
    if (bb->count > 0) {
        bb->output[bb->pos++] = bb->bits & 0xFF;
    }
}

// Hash function for LZ77
static inline uint32_t hash3(const unsigned char *data) {
    return ((data[0] << 10) ^ (data[1] << 5) ^ data[2]) & HASH_MASK;
}

// LZ77 compression
LZ77Token* lz77_compress(const unsigned char *data, size_t size, size_t *token_count) {
    LZ77Token *tokens = (LZ77Token*)malloc(size * 2 * sizeof(LZ77Token));
    int *hash_table = (int*)malloc(HASH_SIZE * sizeof(int));
    int *prev = (int*)malloc(size * sizeof(int));

    // Initialize hash table
    for (int i = 0; i < HASH_SIZE; i++) {
        hash_table[i] = -1;
    }

    *token_count = 0;
    size_t pos = 0;

    while (pos < size) {
        int best_length = 0;
        int best_distance = 0;

        // Try to find a match
        if (pos + MIN_MATCH <= size) {
            uint32_t hash = hash3(&data[pos]);
            int match_pos = hash_table[hash];

            // Search hash chain for best match
            int chain_len = 0;
            while (match_pos >= 0 && chain_len < 128) {
                int distance = pos - match_pos;

                if (distance > MAX_DIST || distance <= 0) break;

                // Check match length
                int max_len = (size - pos < MAX_MATCH) ? (size - pos) : MAX_MATCH;
                int len = 0;

                while (len < max_len && data[match_pos + len] == data[pos + len]) {
                    len++;
                }

                if (len >= MIN_MATCH && len > best_length) {
                    best_length = len;
                    best_distance = distance;
                }

                match_pos = prev[match_pos];
                chain_len++;
            }
        }

        if (best_length >= MIN_MATCH) {
            // Output length/distance pair
            tokens[*token_count].literal = -1;
            tokens[*token_count].length = best_length;
            tokens[*token_count].distance = best_distance;
            (*token_count)++;

            // Update hash table for all positions in the match
            for (int i = 0; i < best_length && pos + i + 2 < size; i++) {
                uint32_t h = hash3(&data[pos + i]);
                prev[pos + i] = hash_table[h];
                hash_table[h] = pos + i;
            }

            pos += best_length;
        } else {
            // Output literal
            tokens[*token_count].literal = data[pos];
            tokens[*token_count].length = 0;
            tokens[*token_count].distance = 0;
            (*token_count)++;

            // Update hash table
            if (pos + 2 < size) {
                uint32_t h = hash3(&data[pos]);
                prev[pos] = hash_table[h];
                hash_table[h] = pos;
            }

            pos++;
        }
    }

    free(hash_table);
    free(prev);
    return tokens;
}

// Convert length to DEFLATE length code
void get_length_code(int length, int *code, int *extra_bits, int *extra_value) {
    static const int length_base[] = {
        3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
        35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258
    };
    static const int length_extra[] = {
        0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
        3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0
    };

    for (int i = 0; i < LENGTH_CODES; i++) {
        if (length_base[i] <= length &&
            (i == LENGTH_CODES - 1 || length < length_base[i + 1])) {
            *code = 257 + i;
            *extra_bits = length_extra[i];
            *extra_value = length - length_base[i];
            return;
        }
    }
}

// Convert distance to DEFLATE distance code
void get_distance_code(int distance, int *code, int *extra_bits, int *extra_value) {
    static const int dist_base[] = {
        1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193,
        257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145,
        8193, 12289, 16385, 24577
    };
    static const int dist_extra[] = {
        0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
        7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
    };

    for (int i = 0; i < DIST_CODES; i++) {
        if (dist_base[i] <= distance &&
            (i == DIST_CODES - 1 || distance < dist_base[i + 1])) {
            *code = i;
            *extra_bits = dist_extra[i];
            *extra_value = distance - dist_base[i];
            return;
        }
    }
}

// Compare function for qsort (for Huffman tree)
int compare_nodes(const void *a, const void *b) {
    HuffNode *na = *(HuffNode**)a;
    HuffNode *nb = *(HuffNode**)b;
    return na->freq - nb->freq;
}

// Build Huffman tree
HuffNode* build_huffman_tree(int *freqs, int num_symbols) {
    HuffNode **nodes = (HuffNode**)malloc(num_symbols * 2 * sizeof(HuffNode*));
    int node_count = 0;

    // Create leaf nodes
    for (int i = 0; i < num_symbols; i++) {
        if (freqs[i] > 0) {
            HuffNode *node = (HuffNode*)malloc(sizeof(HuffNode));
            node->freq = freqs[i];
            node->symbol = i;
            node->left = NULL;
            node->right = NULL;
            nodes[node_count++] = node;
        }
    }

    if (node_count == 0) {
        // No symbols - create dummy node
        HuffNode *node = (HuffNode*)malloc(sizeof(HuffNode));
        node->freq = 1;
        node->symbol = 0;
        node->left = NULL;
        node->right = NULL;
        free(nodes);
        return node;
    }

    if (node_count == 1) {
        // Only one symbol - create a parent
        HuffNode *parent = (HuffNode*)malloc(sizeof(HuffNode));
        parent->freq = nodes[0]->freq;
        parent->symbol = -1;
        parent->left = nodes[0];
        parent->right = NULL;
        free(nodes);
        return parent;
    }

    // Build tree bottom-up
    while (node_count > 1) {
        // Sort nodes by frequency
        qsort(nodes, node_count, sizeof(HuffNode*), compare_nodes);

        // Take two lowest frequency nodes
        HuffNode *left = nodes[0];
        HuffNode *right = nodes[1];

        // Create parent node
        HuffNode *parent = (HuffNode*)malloc(sizeof(HuffNode));
        parent->freq = left->freq + right->freq;
        parent->symbol = -1;
        parent->left = left;
        parent->right = right;

        // Replace first two nodes with parent
        nodes[0] = parent;
        for (int i = 1; i < node_count - 1; i++) {
            nodes[i] = nodes[i + 1];
        }
        node_count--;
    }

    HuffNode *root = nodes[0];
    free(nodes);
    return root;
}

// Generate Huffman codes from tree
void generate_codes(HuffNode *node, HuffCode *codes, uint16_t code, uint8_t depth) {
    if (node == NULL) return;

    if (node->left == NULL && node->right == NULL) {
        // Leaf node
        codes[node->symbol].code = code;
        codes[node->symbol].length = depth;
    } else {
        if (node->left)
            generate_codes(node->left, codes, code, depth + 1);
        if (node->right)
            generate_codes(node->right, codes, (code | (1 << depth)), depth + 1);
    }
}

// Free Huffman tree
void free_huffman_tree(HuffNode *node) {
    if (node == NULL) return;
    free_huffman_tree(node->left);
    free_huffman_tree(node->right);
    free(node);
}

// DEFLATE compression (simplified - uses fixed Huffman codes for simplicity)
int deflate_compress(const char *input_filename, const char *output_filename) {
    // Read input file
    FILE *input = fopen(input_filename, "rb");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_filename);
        return 1;
    }

    fseek(input, 0, SEEK_END);
    size_t input_size = ftell(input);
    fseek(input, 0, SEEK_SET);

    unsigned char *input_data = NULL;
    if (input_size > 0) {
        input_data = (unsigned char*)malloc(input_size);
        size_t bytes_read = fread(input_data, 1, input_size, input);
        fclose(input);

        if (bytes_read != input_size) {
            fprintf(stderr, "Error: Failed to read input file\n");
            free(input_data);
            return 1;
        }
    } else {
        fclose(input);
    }

    // Perform LZ77 compression
    size_t token_count = 0;
    LZ77Token *tokens = NULL;

    if (input_size > 0) {
        tokens = lz77_compress(input_data, input_size, &token_count);
    }

    // Calculate symbol frequencies
    int lit_freq[MAX_CODES] = {0};
    int dist_freq[DIST_CODES] = {0};

    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].literal >= 0) {
            lit_freq[tokens[i].literal]++;
        } else {
            int code = 0, extra_bits = 0, extra_value = 0;
            get_length_code(tokens[i].length, &code, &extra_bits, &extra_value);
            lit_freq[code]++;

            get_distance_code(tokens[i].distance, &code, &extra_bits, &extra_value);
            dist_freq[code]++;
        }
    }
    lit_freq[256]++;  // End of block marker

    // Build Huffman trees
    HuffNode *lit_tree = build_huffman_tree(lit_freq, MAX_CODES);
    HuffNode *dist_tree = build_huffman_tree(dist_freq, DIST_CODES);

    // Generate Huffman codes
    HuffCode lit_codes[MAX_CODES] = {0};
    HuffCode dist_codes[DIST_CODES] = {0};
    generate_codes(lit_tree, lit_codes, 0, 0);
    generate_codes(dist_tree, dist_codes, 0, 0);

    // Initialize bit buffer (at least 1024 bytes)
    size_t buffer_size = (input_size * 2 > 1024) ? input_size * 2 : 1024;
    BitBuffer *bb = init_bitbuffer(buffer_size);

    // Write DEFLATE block header
    write_bits(bb, 1, 1);  // BFINAL = 1 (last block)
    write_bits(bb, 1, 2);  // BTYPE = 01 (fixed Huffman)

    // For simplicity, we'll use fixed Huffman codes
    // Fixed codes: literals 0-143: 8 bits (00110000-10111111)
    //              literals 144-255: 9 bits (110010000-111111111)
    //              lengths 256-279: 7 bits (0000000-0010111)
    //              lengths 280-287: 8 bits (11000000-11000111)

    // Write compressed data using fixed Huffman codes
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].literal >= 0) {
            // Literal byte - use fixed Huffman code
            int lit = tokens[i].literal;
            if (lit <= 143) {
                write_bits(bb, 0x30 + lit, 8);  // Reversed: 0x30 = 00110000
            } else {
                write_bits(bb, 0x190 + (lit - 144), 9);  // Reversed: 0x190 = 110010000
            }
        } else {
            // Length/distance pair
            int len_code = 0, len_extra_bits = 0, len_extra_value = 0;
            get_length_code(tokens[i].length, &len_code, &len_extra_bits, &len_extra_value);

            // Write length code
            if (len_code <= 279) {
                write_bits(bb, len_code - 256, 7);
            } else {
                write_bits(bb, 0xC0 + (len_code - 280), 8);
            }

            // Write length extra bits
            if (len_extra_bits > 0) {
                write_bits(bb, len_extra_value, len_extra_bits);
            }

            // Write distance code and extra bits
            int dist_code = 0, dist_extra_bits = 0, dist_extra_value = 0;
            get_distance_code(tokens[i].distance, &dist_code, &dist_extra_bits, &dist_extra_value);

            // Distance codes are 5 bits, reversed
            uint8_t reversed_dist = 0;
            for (int j = 0; j < 5; j++) {
                if (dist_code & (1 << j)) {
                    reversed_dist |= (1 << (4 - j));
                }
            }
            write_bits(bb, reversed_dist, 5);

            // Write distance extra bits
            if (dist_extra_bits > 0) {
                write_bits(bb, dist_extra_value, dist_extra_bits);
            }
        }
    }

    // Write end-of-block marker (256)
    write_bits(bb, 0, 7);

    // Flush bits
    flush_bits(bb);

    // Write output file
    FILE *output = fopen(output_filename, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", output_filename);
        free(input_data);
        free(tokens);
        free(bb->output);
        free(bb);
        free_huffman_tree(lit_tree);
        free_huffman_tree(dist_tree);
        return 1;
    }

    fwrite(bb->output, 1, bb->pos, output);
    fclose(output);

    // Print statistics
    printf("Compression complete!\n");
    printf("Input size:  %zu bytes\n", input_size);
    printf("Output size: %zu bytes\n", bb->pos);
    if (input_size > 0) {
        printf("Compression ratio: %.2f%%\n", (double)bb->pos / input_size * 100.0);
    }

    // Cleanup
    if (input_data) free(input_data);
    if (tokens) free(tokens);
    free(bb->output);
    free(bb);
    free_huffman_tree(lit_tree);
    free_huffman_tree(dist_tree);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_file> <output_file>\n", argv[0]);
        printf("Compresses input_file using DEFLATE algorithm and writes to output_file\n");
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = argv[2];

    printf("Compressing '%s' to '%s' using DEFLATE...\n", input_file, output_file);

    return deflate_compress(input_file, output_file);
}
