#include "matt.h"

static char *read_file(const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file: %s\n", path);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char *buffer = (char *)malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Could not allocate memory for file\n");
        exit(1);
    }

    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = '\0';

    fclose(file);
    return buffer;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file.matt>\n", argv[0]);
        return 1;
    }

    // Read source file
    char *source = read_file(argv[1]);

    // Lexical analysis
    int token_count;
    Token *tokens = tokenize(source, &token_count);

    // Check for lexer errors
    if (tokens[token_count - 1].type == TOKEN_ERROR) {
        fprintf(stderr, "Lexer error: %s\n", tokens[token_count - 1].lexeme);
        free(source);
        free_tokens(tokens, token_count);
        return 1;
    }

    // Parsing
    ASTNode *ast = parse(tokens, token_count);

    // Type checking (currently minimal - would be expanded)
    // bool type_check_result = check_types(ast);
    // if (!type_check_result) {
    //     fprintf(stderr, "Type checking failed\n");
    //     free(source);
    //     free_tokens(tokens, token_count);
    //     free_ast(ast);
    //     return 1;
    // }

    // Interpretation
    Value result = interpret(ast);

    // Cleanup
    // TODO: Fix memory management to avoid double-free
    // free(source);
    // free_tokens(tokens, token_count);
    // free_ast(ast);

    // Return exit code from main function
    if (result.type == TYPE_INT) {
        return result.value.int_val;
    }

    return 0;
}
