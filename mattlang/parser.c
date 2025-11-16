#include "matt.h"

static Parser parser;

static Token *current_token() {
    return &parser.tokens[parser.current];
}

static Token *previous_token() {
    return &parser.tokens[parser.current - 1];
}

static bool is_at_end() {
    return current_token()->type == TOKEN_EOF;
}

static bool check(TokenType type) {
    if (is_at_end()) return false;
    return current_token()->type == type;
}

static Token *advance() {
    if (!is_at_end()) parser.current++;
    return previous_token();
}

static bool match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

static void error_at(Token *token, const char *message) {
    fprintf(stderr, "[Line %d] Error at '%s': %s\n", token->line, token->lexeme, message);
    exit(1);
}

static void expect(TokenType type, const char *message) {
    if (current_token()->type == type) {
        advance();
        return;
    }
    error_at(current_token(), message);
}

static ASTNode *make_node(NodeType type) {
    ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = current_token()->line;
    return node;
}

// Forward declarations
static ASTNode *parse_expression();
static ASTNode *parse_statement();
static ASTNode *parse_declaration();
static TypeInfo *parse_type();

static TypeInfo *parse_type() {
    TypeInfo *type = (TypeInfo *)calloc(1, sizeof(TypeInfo));
    type->is_pointer = false;
    type->element_type = NULL;

    if (match(TOKEN_INT)) {
        type->base_type = TYPE_INT;
    } else if (match(TOKEN_FLOAT)) {
        type->base_type = TYPE_FLOAT;
    } else if (match(TOKEN_DOUBLE)) {
        type->base_type = TYPE_DOUBLE;
    } else if (match(TOKEN_LONG)) {
        type->base_type = TYPE_LONG;
    } else if (match(TOKEN_BOOL)) {
        type->base_type = TYPE_BOOL;
    } else if (match(TOKEN_CHAR)) {
        type->base_type = TYPE_CHAR;
    } else if (match(TOKEN_STRING)) {
        type->base_type = TYPE_STRING;
    } else if (match(TOKEN_VOID)) {
        type->base_type = TYPE_VOID;
    } else {
        error_at(current_token(), "Expected type");
    }

    // Check for array type
    if (match(TOKEN_LBRACKET)) {
        expect(TOKEN_RBRACKET, "Expected ']' after '['");
        TypeInfo *array_type = (TypeInfo *)calloc(1, sizeof(TypeInfo));
        array_type->base_type = TYPE_ARRAY;
        array_type->element_type = type;
        array_type->is_pointer = false;
        return array_type;
    }

    // Check for pointer type
    if (match(TOKEN_STAR)) {
        type->is_pointer = true;
    }

    return type;
}

static ASTNode *parse_primary() {
    // Integer literal
    if (match(TOKEN_INT_LITERAL)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_INT);
        node->data.literal.value.int_val = previous_token()->value.int_val;
        return node;
    }

    // Float literal
    if (match(TOKEN_FLOAT_LITERAL)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_FLOAT);
        node->data.literal.value.float_val = previous_token()->value.float_val;
        return node;
    }

    // String literal
    if (match(TOKEN_STRING_LITERAL)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_STRING);
        node->data.literal.value.str_val = strdup(previous_token()->value.str_val);
        return node;
    }

    // Char literal
    if (match(TOKEN_CHAR_LITERAL)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_CHAR);
        node->data.literal.value.char_val = previous_token()->value.char_val;
        return node;
    }

    // Boolean literals
    if (match(TOKEN_TRUE)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_BOOL);
        node->data.literal.value.bool_val = true;
        return node;
    }

    if (match(TOKEN_FALSE)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_BOOL);
        node->data.literal.value.bool_val = false;
        return node;
    }

    // Null literal
    if (match(TOKEN_NULL)) {
        ASTNode *node = make_node(NODE_LITERAL);
        node->data_type = make_type(TYPE_NULL);
        return node;
    }

    // Array literal
    if (match(TOKEN_LBRACKET)) {
        ASTNode *node = make_node(NODE_ARRAY_LITERAL);
        int capacity = 8;
        node->data.array_literal.elements = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);
        node->data.array_literal.elem_count = 0;

        if (!check(TOKEN_RBRACKET)) {
            do {
                if (node->data.array_literal.elem_count >= capacity) {
                    capacity *= 2;
                    node->data.array_literal.elements = (ASTNode **)realloc(
                        node->data.array_literal.elements, sizeof(ASTNode *) * capacity);
                }
                node->data.array_literal.elements[node->data.array_literal.elem_count++] =
                    parse_expression();
            } while (match(TOKEN_COMMA));
        }

        expect(TOKEN_RBRACKET, "Expected ']' after array literal");
        return node;
    }

    // Parenthesized expression or cast
    if (match(TOKEN_LPAREN)) {
        // Check if this is a cast
        int saved_pos = parser.current;
        TypeInfo *cast_type = NULL;

        // Try to parse as type
        if (check(TOKEN_INT) || check(TOKEN_FLOAT) || check(TOKEN_DOUBLE) ||
            check(TOKEN_LONG) || check(TOKEN_BOOL) || check(TOKEN_CHAR) ||
            check(TOKEN_STRING)) {
            cast_type = parse_type();
            if (match(TOKEN_RPAREN)) {
                // This is a cast
                ASTNode *node = make_node(NODE_CAST);
                node->data.cast.target_type = cast_type;
                node->data.cast.expr = parse_expression();
                return node;
            }
        }

        // Not a cast, restore position and parse as expression
        parser.current = saved_pos;
        ASTNode *expr = parse_expression();
        expect(TOKEN_RPAREN, "Expected ')' after expression");
        return expr;
    }

    // Identifier or function call
    if (match(TOKEN_IDENTIFIER) || match(TOKEN_PRINTF)) {
        Token *name_token = previous_token();
        ASTNode *node = make_node(NODE_IDENTIFIER);
        node->data.identifier.name = strdup(name_token->lexeme);
        return node;
    }

    error_at(current_token(), "Expected expression");
    return NULL;
}

static ASTNode *parse_postfix() {
    ASTNode *expr = parse_primary();

    while (true) {
        if (match(TOKEN_LBRACKET)) {
            // Array access
            ASTNode *node = make_node(NODE_ARRAY_ACCESS);
            node->data.array_access.array = expr;
            node->data.array_access.index = parse_expression();
            expect(TOKEN_RBRACKET, "Expected ']' after array index");
            expr = node;
        } else if (match(TOKEN_LPAREN)) {
            // Function call
            ASTNode *node = make_node(NODE_CALL);
            if (expr->type != NODE_IDENTIFIER) {
                error_at(current_token(), "Can only call functions");
            }
            node->data.call.name = expr->data.identifier.name;
            free(expr); // Free the identifier node

            int capacity = 8;
            node->data.call.args = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);
            node->data.call.arg_count = 0;

            if (!check(TOKEN_RPAREN)) {
                do {
                    if (node->data.call.arg_count >= capacity) {
                        capacity *= 2;
                        node->data.call.args = (ASTNode **)realloc(
                            node->data.call.args, sizeof(ASTNode *) * capacity);
                    }
                    node->data.call.args[node->data.call.arg_count++] = parse_expression();
                } while (match(TOKEN_COMMA));
            }

            expect(TOKEN_RPAREN, "Expected ')' after arguments");
            expr = node;
        } else if (match(TOKEN_DOT)) {
            // Member access
            ASTNode *node = make_node(NODE_MEMBER_ACCESS);
            node->data.member_access.object = expr;
            expect(TOKEN_IDENTIFIER, "Expected member name after '.'");
            node->data.member_access.member = strdup(previous_token()->lexeme);
            expr = node;
        } else {
            break;
        }
    }

    return expr;
}

static ASTNode *parse_unary() {
    if (match(TOKEN_NOT) || match(TOKEN_MINUS)) {
        ASTNode *node = make_node(NODE_UNARY_OP);
        node->data.unary.op = previous_token()->type;
        node->data.unary.operand = parse_unary();
        return node;
    }

    return parse_postfix();
}

static ASTNode *parse_multiplicative() {
    ASTNode *expr = parse_unary();

    while (match(TOKEN_STAR) || match(TOKEN_SLASH) || match(TOKEN_PERCENT)) {
        ASTNode *node = make_node(NODE_BINARY_OP);
        node->data.binary.op = previous_token()->type;
        node->data.binary.left = expr;
        node->data.binary.right = parse_unary();
        expr = node;
    }

    return expr;
}

static ASTNode *parse_additive() {
    ASTNode *expr = parse_multiplicative();

    while (match(TOKEN_PLUS) || match(TOKEN_MINUS)) {
        ASTNode *node = make_node(NODE_BINARY_OP);
        node->data.binary.op = previous_token()->type;
        node->data.binary.left = expr;
        node->data.binary.right = parse_multiplicative();
        expr = node;
    }

    return expr;
}

static ASTNode *parse_comparison() {
    ASTNode *expr = parse_additive();

    while (match(TOKEN_LT) || match(TOKEN_GT) || match(TOKEN_LTE) || match(TOKEN_GTE)) {
        ASTNode *node = make_node(NODE_BINARY_OP);
        node->data.binary.op = previous_token()->type;
        node->data.binary.left = expr;
        node->data.binary.right = parse_additive();
        expr = node;
    }

    return expr;
}

static ASTNode *parse_equality() {
    ASTNode *expr = parse_comparison();

    while (match(TOKEN_EQ) || match(TOKEN_NEQ)) {
        ASTNode *node = make_node(NODE_BINARY_OP);
        node->data.binary.op = previous_token()->type;
        node->data.binary.left = expr;
        node->data.binary.right = parse_comparison();
        expr = node;
    }

    return expr;
}

static ASTNode *parse_logical_and() {
    ASTNode *expr = parse_equality();

    while (match(TOKEN_AND)) {
        ASTNode *node = make_node(NODE_BINARY_OP);
        node->data.binary.op = previous_token()->type;
        node->data.binary.left = expr;
        node->data.binary.right = parse_equality();
        expr = node;
    }

    return expr;
}

static ASTNode *parse_logical_or() {
    ASTNode *expr = parse_logical_and();

    while (match(TOKEN_OR)) {
        ASTNode *node = make_node(NODE_BINARY_OP);
        node->data.binary.op = previous_token()->type;
        node->data.binary.left = expr;
        node->data.binary.right = parse_logical_and();
        expr = node;
    }

    return expr;
}

static ASTNode *parse_assignment() {
    ASTNode *expr = parse_logical_or();

    if (match(TOKEN_ASSIGN)) {
        ASTNode *node = make_node(NODE_ASSIGN);
        node->data.assign.target = expr;
        node->data.assign.value = parse_assignment(); // Right-associative
        return node;
    }

    return expr;
}

static ASTNode *parse_expression() {
    return parse_assignment();
}

static ASTNode *parse_block() {
    expect(TOKEN_LBRACE, "Expected '{'");

    ASTNode *node = make_node(NODE_BLOCK);
    int capacity = 16;
    node->data.block.statements = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);
    node->data.block.stmt_count = 0;

    while (!check(TOKEN_RBRACE) && !is_at_end()) {
        if (node->data.block.stmt_count >= capacity) {
            capacity *= 2;
            node->data.block.statements = (ASTNode **)realloc(
                node->data.block.statements, sizeof(ASTNode *) * capacity);
        }

        ASTNode *stmt = NULL;
        // Check if it's a declaration
        if (check(TOKEN_INT) || check(TOKEN_FLOAT) || check(TOKEN_DOUBLE) ||
            check(TOKEN_LONG) || check(TOKEN_BOOL) || check(TOKEN_CHAR) ||
            check(TOKEN_STRING)) {
            stmt = parse_declaration();
        } else {
            stmt = parse_statement();
        }
        node->data.block.statements[node->data.block.stmt_count++] = stmt;
    }

    expect(TOKEN_RBRACE, "Expected '}'");
    return node;
}

static ASTNode *parse_if_statement() {
    expect(TOKEN_IF, "Expected 'if'");
    expect(TOKEN_LPAREN, "Expected '(' after 'if'");
    ASTNode *condition = parse_expression();
    expect(TOKEN_RPAREN, "Expected ')' after condition");

    ASTNode *then_branch = parse_statement();
    ASTNode *else_branch = NULL;

    if (match(TOKEN_ELSE)) {
        else_branch = parse_statement();
    }

    ASTNode *node = make_node(NODE_IF);
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;

    return node;
}

static ASTNode *parse_while_statement() {
    expect(TOKEN_WHILE, "Expected 'while'");
    expect(TOKEN_LPAREN, "Expected '(' after 'while'");
    ASTNode *condition = parse_expression();
    expect(TOKEN_RPAREN, "Expected ')' after condition");

    ASTNode *body = parse_statement();

    ASTNode *node = make_node(NODE_WHILE);
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;

    return node;
}

static ASTNode *parse_for_statement() {
    expect(TOKEN_FOR, "Expected 'for'");
    expect(TOKEN_LPAREN, "Expected '(' after 'for'");

    ASTNode *init = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        if (check(TOKEN_INT) || check(TOKEN_FLOAT) || check(TOKEN_DOUBLE) ||
            check(TOKEN_LONG) || check(TOKEN_BOOL) || check(TOKEN_CHAR) ||
            check(TOKEN_STRING)) {
            init = parse_declaration();
            // Declaration already consumes semicolon
        } else {
            init = parse_expression();
            expect(TOKEN_SEMICOLON, "Expected ';' after for init");
        }
    } else {
        expect(TOKEN_SEMICOLON, "Expected ';'");
    }

    ASTNode *condition = NULL;
    if (!check(TOKEN_SEMICOLON)) {
        condition = parse_expression();
    }
    expect(TOKEN_SEMICOLON, "Expected ';' after for condition");

    ASTNode *increment = NULL;
    if (!check(TOKEN_RPAREN)) {
        increment = parse_expression();
    }
    expect(TOKEN_RPAREN, "Expected ')' after for clauses");

    ASTNode *body = parse_statement();

    ASTNode *node = make_node(NODE_FOR);
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.increment = increment;
    node->data.for_stmt.body = body;

    return node;
}

static ASTNode *parse_return_statement() {
    expect(TOKEN_RETURN, "Expected 'return'");

    ASTNode *node = make_node(NODE_RETURN);
    if (!check(TOKEN_SEMICOLON)) {
        node->data.return_stmt.value = parse_expression();
    } else {
        node->data.return_stmt.value = NULL;
    }

    expect(TOKEN_SEMICOLON, "Expected ';' after return");
    return node;
}

static ASTNode *parse_statement() {
    if (check(TOKEN_IF)) {
        return parse_if_statement();
    }
    if (check(TOKEN_WHILE)) {
        return parse_while_statement();
    }
    if (check(TOKEN_FOR)) {
        return parse_for_statement();
    }
    if (check(TOKEN_RETURN)) {
        return parse_return_statement();
    }
    if (check(TOKEN_LBRACE)) {
        return parse_block();
    }
    if (match(TOKEN_BREAK)) {
        ASTNode *node = make_node(NODE_BREAK);
        expect(TOKEN_SEMICOLON, "Expected ';' after 'break'");
        return node;
    }
    if (match(TOKEN_CONTINUE)) {
        ASTNode *node = make_node(NODE_CONTINUE);
        expect(TOKEN_SEMICOLON, "Expected ';' after 'continue'");
        return node;
    }

    // Expression statement
    ASTNode *node = make_node(NODE_EXPR_STMT);
    node->data.expr_stmt.expr = parse_expression();
    expect(TOKEN_SEMICOLON, "Expected ';' after expression");
    return node;
}

static ASTNode *parse_declaration() {
    TypeInfo *type = parse_type();
    expect(TOKEN_IDENTIFIER, "Expected variable name");
    char *name = strdup(previous_token()->lexeme);

    ASTNode *node = make_node(NODE_VAR_DECL);
    node->data.var_decl.name = name;
    node->data.var_decl.var_type = type;

    expect(TOKEN_ASSIGN, "Expected '=' after variable name (all variables must be initialized)");
    node->data.var_decl.initializer = parse_expression();

    expect(TOKEN_SEMICOLON, "Expected ';' after declaration");
    return node;
}

static ASTNode *parse_function() {
    TypeInfo *return_type = parse_type();
    expect(TOKEN_IDENTIFIER, "Expected function name");
    char *name = strdup(previous_token()->lexeme);

    expect(TOKEN_LPAREN, "Expected '(' after function name");

    int param_capacity = 8;
    char **param_names = (char **)malloc(sizeof(char *) * param_capacity);
    TypeInfo **param_types = (TypeInfo **)malloc(sizeof(TypeInfo *) * param_capacity);
    int param_count = 0;

    if (!check(TOKEN_RPAREN)) {
        do {
            if (param_count >= param_capacity) {
                param_capacity *= 2;
                param_names = (char **)realloc(param_names, sizeof(char *) * param_capacity);
                param_types = (TypeInfo **)realloc(param_types, sizeof(TypeInfo *) * param_capacity);
            }

            param_types[param_count] = parse_type();
            expect(TOKEN_IDENTIFIER, "Expected parameter name");
            param_names[param_count] = strdup(previous_token()->lexeme);
            param_count++;
        } while (match(TOKEN_COMMA));
    }

    expect(TOKEN_RPAREN, "Expected ')' after parameters");

    ASTNode *body = parse_block();

    ASTNode *node = make_node(NODE_FUNCTION);
    node->data.function.name = name;
    node->data.function.return_type = return_type;
    node->data.function.param_names = param_names;
    node->data.function.param_types = param_types;
    node->data.function.param_count = param_count;
    node->data.function.body = body;

    return node;
}

ASTNode *parse(Token *tokens, int token_count) {
    parser.tokens = tokens;
    parser.token_count = token_count;
    parser.current = 0;

    ASTNode *program = make_node(NODE_PROGRAM);
    int capacity = 16;
    program->data.program.functions = (ASTNode **)malloc(sizeof(ASTNode *) * capacity);
    program->data.program.func_count = 0;

    while (!is_at_end()) {
        if (program->data.program.func_count >= capacity) {
            capacity *= 2;
            program->data.program.functions = (ASTNode **)realloc(
                program->data.program.functions, sizeof(ASTNode *) * capacity);
        }

        program->data.program.functions[program->data.program.func_count++] = parse_function();
    }

    return program;
}
