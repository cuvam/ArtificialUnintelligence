#ifndef MATT_H
#define MATT_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* Token Types */
typedef enum {
    // Literals
    TOKEN_INT_LITERAL,
    TOKEN_FLOAT_LITERAL,
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,

    // Keywords
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_DOUBLE,
    TOKEN_LONG,
    TOKEN_BOOL,
    TOKEN_CHAR,
    TOKEN_STRING,
    TOKEN_VOID,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_RETURN,
    TOKEN_PRINTF,

    // Identifiers
    TOKEN_IDENTIFIER,

    // Operators
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTE,
    TOKEN_GTE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,

    // Delimiters
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_COLON,

    // Special
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int line;
    int column;
    union {
        int int_val;
        double float_val;
        char *str_val;
        char char_val;
    } value;
} Token;

/* AST Node Types */
typedef enum {
    NODE_PROGRAM,
    NODE_FUNCTION,
    NODE_BLOCK,
    NODE_VAR_DECL,
    NODE_RETURN,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_SWITCH,
    NODE_CASE,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_EXPR_STMT,
    NODE_BINARY_OP,
    NODE_UNARY_OP,
    NODE_ASSIGN,
    NODE_CALL,
    NODE_ARRAY_ACCESS,
    NODE_ARRAY_LITERAL,
    NODE_MEMBER_ACCESS,
    NODE_CAST,
    NODE_LITERAL,
    NODE_IDENTIFIER,
} NodeType;

typedef enum {
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_DOUBLE,
    TYPE_LONG,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_ARRAY,
    TYPE_NULL,
    TYPE_UNKNOWN
} DataType;

typedef struct ASTNode ASTNode;
typedef struct TypeInfo TypeInfo;

struct TypeInfo {
    DataType base_type;
    TypeInfo *element_type;  // For arrays
    bool is_pointer;
};

struct ASTNode {
    NodeType type;
    TypeInfo *data_type;
    int line;

    union {
        // Program
        struct {
            ASTNode **functions;
            int func_count;
        } program;

        // Function
        struct {
            char *name;
            TypeInfo *return_type;
            char **param_names;
            TypeInfo **param_types;
            int param_count;
            ASTNode *body;
        } function;

        // Block
        struct {
            ASTNode **statements;
            int stmt_count;
        } block;

        // Variable declaration
        struct {
            char *name;
            TypeInfo *var_type;
            ASTNode *initializer;
        } var_decl;

        // Return
        struct {
            ASTNode *value;
        } return_stmt;

        // If statement
        struct {
            ASTNode *condition;
            ASTNode *then_branch;
            ASTNode *else_branch;
        } if_stmt;

        // While loop
        struct {
            ASTNode *condition;
            ASTNode *body;
        } while_stmt;

        // For loop
        struct {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *increment;
            ASTNode *body;
        } for_stmt;

        // Switch
        struct {
            ASTNode *expr;
            ASTNode **cases;
            int case_count;
            ASTNode *default_case;
        } switch_stmt;

        // Case
        struct {
            ASTNode *value;
            ASTNode **statements;
            int stmt_count;
        } case_stmt;

        // Expression statement
        struct {
            ASTNode *expr;
        } expr_stmt;

        // Binary operation
        struct {
            TokenType op;
            ASTNode *left;
            ASTNode *right;
        } binary;

        // Unary operation
        struct {
            TokenType op;
            ASTNode *operand;
        } unary;

        // Assignment
        struct {
            ASTNode *target;
            ASTNode *value;
        } assign;

        // Function call
        struct {
            char *name;
            ASTNode **args;
            int arg_count;
        } call;

        // Array access
        struct {
            ASTNode *array;
            ASTNode *index;
        } array_access;

        // Array literal
        struct {
            ASTNode **elements;
            int elem_count;
        } array_literal;

        // Member access
        struct {
            ASTNode *object;
            char *member;
        } member_access;

        // Cast
        struct {
            TypeInfo *target_type;
            ASTNode *expr;
        } cast;

        // Literal
        struct {
            union {
                int int_val;
                double float_val;
                char *str_val;
                char char_val;
                bool bool_val;
            } value;
        } literal;

        // Identifier
        struct {
            char *name;
        } identifier;
    } data;
};

/* Runtime Value */
typedef struct {
    DataType type;
    union {
        int int_val;
        double float_val;
        long long_val;
        bool bool_val;
        char char_val;
        char *str_val;
        struct {
            void **elements;
            int length;
            int capacity;
            DataType elem_type;
        } array;
    } value;
} Value;

/* Lexer */
typedef struct {
    const char *source;
    int start;
    int current;
    int line;
    int column;
} Lexer;

/* Parser */
typedef struct {
    Token *tokens;
    int token_count;
    int current;
} Parser;

/* Symbol Table Entry */
typedef struct SymbolEntry {
    char *name;
    TypeInfo *type;
    Value value;
    bool is_initialized;
    struct SymbolEntry *next;
} SymbolEntry;

/* Scope */
typedef struct Scope {
    SymbolEntry *symbols;
    struct Scope *parent;
} Scope;

/* Interpreter Context */
typedef struct {
    Scope *global_scope;
    Scope *current_scope;
    bool in_loop;
    bool should_break;
    bool should_continue;
    bool should_return;
    Value return_value;
} Context;

/* Function prototypes */
// Lexer
void init_lexer(Lexer *lexer, const char *source);
Token next_token(Lexer *lexer);
Token *tokenize(const char *source, int *token_count);

// Parser
ASTNode *parse(Token *tokens, int token_count);

// Type checker
bool check_types(ASTNode *ast);

// Interpreter
Value interpret(ASTNode *ast);

// Utility
void free_ast(ASTNode *node);
void free_tokens(Token *tokens, int count);
TypeInfo *make_type(DataType base_type);
TypeInfo *make_array_type(DataType elem_type);
bool types_equal(TypeInfo *a, TypeInfo *b);
const char *type_to_string(TypeInfo *type);
void print_value(Value v);

#endif
