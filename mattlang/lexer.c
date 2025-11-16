#include "matt.h"

void init_lexer(Lexer *lexer, const char *source) {
    lexer->source = source;
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
}

static bool is_at_end(Lexer *lexer) {
    return lexer->source[lexer->current] == '\0';
}

static char advance(Lexer *lexer) {
    char c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    return c;
}

static char peek(Lexer *lexer) {
    return lexer->source[lexer->current];
}

static char peek_next(Lexer *lexer) {
    if (is_at_end(lexer)) return '\0';
    return lexer->source[lexer->current + 1];
}

static bool match(Lexer *lexer, char expected) {
    if (is_at_end(lexer)) return false;
    if (lexer->source[lexer->current] != expected) return false;
    lexer->current++;
    lexer->column++;
    return true;
}

static void skip_whitespace(Lexer *lexer) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
            case '\n':
                advance(lexer);
                break;
            case '/':
                if (peek_next(lexer) == '/') {
                    // Single-line comment
                    while (!is_at_end(lexer) && peek(lexer) != '\n') {
                        advance(lexer);
                    }
                } else if (peek_next(lexer) == '*') {
                    // Multi-line comment
                    advance(lexer); // consume /
                    advance(lexer); // consume *
                    while (!is_at_end(lexer)) {
                        if (peek(lexer) == '*' && peek_next(lexer) == '/') {
                            advance(lexer); // consume *
                            advance(lexer); // consume /
                            break;
                        }
                        advance(lexer);
                    }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    token.type = type;
    int length = lexer->current - lexer->start;
    token.lexeme = (char *)malloc(length + 1);
    memcpy(token.lexeme, lexer->source + lexer->start, length);
    token.lexeme[length] = '\0';
    token.line = lexer->line;
    token.column = lexer->column - length;
    return token;
}

static Token error_token(Lexer *lexer, const char *message) {
    Token token;
    token.type = TOKEN_ERROR;
    token.lexeme = strdup(message);
    token.line = lexer->line;
    token.column = lexer->column;
    return token;
}

static Token string_token(Lexer *lexer) {
    while (!is_at_end(lexer) && peek(lexer) != '"') {
        if (peek(lexer) == '\\') {
            advance(lexer);
            if (!is_at_end(lexer)) advance(lexer);
        } else {
            advance(lexer);
        }
    }

    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated string");
    }

    advance(lexer); // closing "

    Token token = make_token(lexer, TOKEN_STRING_LITERAL);
    // Remove quotes and process escape sequences
    int src_len = strlen(token.lexeme);
    char *str_val = (char *)malloc(src_len - 1);
    int j = 0;
    for (int i = 1; i < src_len - 1; i++) {
        if (token.lexeme[i] == '\\' && i + 1 < src_len - 1) {
            i++;
            switch (token.lexeme[i]) {
                case 'n': str_val[j++] = '\n'; break;
                case 't': str_val[j++] = '\t'; break;
                case 'r': str_val[j++] = '\r'; break;
                case '\\': str_val[j++] = '\\'; break;
                case '"': str_val[j++] = '"'; break;
                default: str_val[j++] = token.lexeme[i]; break;
            }
        } else {
            str_val[j++] = token.lexeme[i];
        }
    }
    str_val[j] = '\0';
    token.value.str_val = str_val;
    return token;
}

static Token char_token(Lexer *lexer) {
    if (is_at_end(lexer)) {
        return error_token(lexer, "Unterminated character literal");
    }

    char c = advance(lexer);
    if (c == '\\') {
        if (is_at_end(lexer)) {
            return error_token(lexer, "Unterminated character literal");
        }
        char escaped = advance(lexer);
        switch (escaped) {
            case 'n': c = '\n'; break;
            case 't': c = '\t'; break;
            case 'r': c = '\r'; break;
            case '\\': c = '\\'; break;
            case '\'': c = '\''; break;
            default: c = escaped; break;
        }
    }

    if (is_at_end(lexer) || peek(lexer) != '\'') {
        return error_token(lexer, "Unterminated character literal");
    }

    advance(lexer); // closing '

    Token token = make_token(lexer, TOKEN_CHAR_LITERAL);
    token.value.char_val = c;
    return token;
}

static Token number_token(Lexer *lexer) {
    while (isdigit(peek(lexer))) {
        advance(lexer);
    }

    bool is_float = false;
    if (peek(lexer) == '.' && isdigit(peek_next(lexer))) {
        is_float = true;
        advance(lexer); // consume .
        while (isdigit(peek(lexer))) {
            advance(lexer);
        }
    }

    Token token = make_token(lexer, is_float ? TOKEN_FLOAT_LITERAL : TOKEN_INT_LITERAL);
    if (is_float) {
        token.value.float_val = atof(token.lexeme);
    } else {
        token.value.int_val = atoi(token.lexeme);
    }
    return token;
}

static TokenType check_keyword(const char *start, int length, const char *rest, TokenType type) {
    if (strlen(rest) == (size_t)length && memcmp(start, rest, length) == 0) {
        return type;
    }
    return TOKEN_IDENTIFIER;
}

static TokenType identifier_type(const char *lexeme) {
    switch (lexeme[0]) {
        case 'b':
            if (strcmp(lexeme, "bool") == 0) return TOKEN_BOOL;
            if (strcmp(lexeme, "break") == 0) return TOKEN_BREAK;
            break;
        case 'c':
            if (strcmp(lexeme, "char") == 0) return TOKEN_CHAR;
            if (strcmp(lexeme, "case") == 0) return TOKEN_CASE;
            if (strcmp(lexeme, "continue") == 0) return TOKEN_CONTINUE;
            break;
        case 'd':
            if (strcmp(lexeme, "double") == 0) return TOKEN_DOUBLE;
            if (strcmp(lexeme, "default") == 0) return TOKEN_DEFAULT;
            break;
        case 'e':
            if (strcmp(lexeme, "else") == 0) return TOKEN_ELSE;
            break;
        case 'f':
            if (strcmp(lexeme, "float") == 0) return TOKEN_FLOAT;
            if (strcmp(lexeme, "false") == 0) return TOKEN_FALSE;
            if (strcmp(lexeme, "for") == 0) return TOKEN_FOR;
            break;
        case 'i':
            if (strcmp(lexeme, "int") == 0) return TOKEN_INT;
            if (strcmp(lexeme, "if") == 0) return TOKEN_IF;
            break;
        case 'l':
            if (strcmp(lexeme, "long") == 0) return TOKEN_LONG;
            break;
        case 'n':
            if (strcmp(lexeme, "null") == 0) return TOKEN_NULL;
            break;
        case 'p':
            if (strcmp(lexeme, "printf") == 0) return TOKEN_PRINTF;
            break;
        case 'r':
            if (strcmp(lexeme, "return") == 0) return TOKEN_RETURN;
            break;
        case 's':
            if (strcmp(lexeme, "string") == 0) return TOKEN_STRING;
            if (strcmp(lexeme, "switch") == 0) return TOKEN_SWITCH;
            break;
        case 't':
            if (strcmp(lexeme, "true") == 0) return TOKEN_TRUE;
            break;
        case 'v':
            if (strcmp(lexeme, "void") == 0) return TOKEN_VOID;
            break;
        case 'w':
            if (strcmp(lexeme, "while") == 0) return TOKEN_WHILE;
            break;
    }
    return TOKEN_IDENTIFIER;
}

static Token identifier_token(Lexer *lexer) {
    while (isalnum(peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    Token token = make_token(lexer, TOKEN_IDENTIFIER);
    token.type = identifier_type(token.lexeme);
    return token;
}

Token next_token(Lexer *lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    char c = advance(lexer);

    if (isalpha(c) || c == '_') {
        while (isalnum(peek(lexer)) || peek(lexer) == '_') {
            advance(lexer);
        }
        Token token = make_token(lexer, TOKEN_IDENTIFIER);
        token.type = identifier_type(token.lexeme);
        return token;
    }

    if (isdigit(c)) {
        lexer->current--;
        lexer->column--;
        return number_token(lexer);
    }

    switch (c) {
        case '(': return make_token(lexer, TOKEN_LPAREN);
        case ')': return make_token(lexer, TOKEN_RPAREN);
        case '{': return make_token(lexer, TOKEN_LBRACE);
        case '}': return make_token(lexer, TOKEN_RBRACE);
        case '[': return make_token(lexer, TOKEN_LBRACKET);
        case ']': return make_token(lexer, TOKEN_RBRACKET);
        case ';': return make_token(lexer, TOKEN_SEMICOLON);
        case ',': return make_token(lexer, TOKEN_COMMA);
        case '.': return make_token(lexer, TOKEN_DOT);
        case ':': return make_token(lexer, TOKEN_COLON);
        case '+': return make_token(lexer, TOKEN_PLUS);
        case '-': return make_token(lexer, TOKEN_MINUS);
        case '*': return make_token(lexer, TOKEN_STAR);
        case '/': return make_token(lexer, TOKEN_SLASH);
        case '%': return make_token(lexer, TOKEN_PERCENT);
        case '!':
            return make_token(lexer, match(lexer, '=') ? TOKEN_NEQ : TOKEN_NOT);
        case '=':
            return make_token(lexer, match(lexer, '=') ? TOKEN_EQ : TOKEN_ASSIGN);
        case '<':
            return make_token(lexer, match(lexer, '=') ? TOKEN_LTE : TOKEN_LT);
        case '>':
            return make_token(lexer, match(lexer, '=') ? TOKEN_GTE : TOKEN_GT);
        case '&':
            if (match(lexer, '&')) return make_token(lexer, TOKEN_AND);
            return error_token(lexer, "Unexpected character '&'");
        case '|':
            if (match(lexer, '|')) return make_token(lexer, TOKEN_OR);
            return error_token(lexer, "Unexpected character '|'");
        case '"':
            return string_token(lexer);
        case '\'':
            return char_token(lexer);
    }

    return error_token(lexer, "Unexpected character");
}

Token *tokenize(const char *source, int *token_count) {
    Lexer lexer;
    init_lexer(&lexer, source);

    int capacity = 256;
    Token *tokens = (Token *)malloc(sizeof(Token) * capacity);
    *token_count = 0;

    while (true) {
        Token token = next_token(&lexer);

        if (*token_count >= capacity) {
            capacity *= 2;
            tokens = (Token *)realloc(tokens, sizeof(Token) * capacity);
        }

        tokens[*token_count] = token;
        (*token_count)++;

        if (token.type == TOKEN_EOF || token.type == TOKEN_ERROR) {
            break;
        }
    }

    return tokens;
}

void free_tokens(Token *tokens, int count) {
    for (int i = 0; i < count; i++) {
        free(tokens[i].lexeme);
        if (tokens[i].type == TOKEN_STRING_LITERAL) {
            free(tokens[i].value.str_val);
        }
    }
    free(tokens);
}
