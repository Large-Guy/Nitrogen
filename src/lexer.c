#include "lexer.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lexer {
    char* source;
    char* start;
    char* current;
    uint32_t line;
};

static bool is_end(const struct lexer* lexer) {
    return *lexer->current == '\0';
}

static struct token make_token(const struct lexer* lexer, enum token_type type) {
    struct token token;
    token.start = lexer->start;
    token.length = lexer->current - lexer->start;
    token.type = type;
    token.line = lexer->line;
    return token;
}

char advance(struct lexer* lexer) {
    lexer->current++;
    return lexer->current[-1];
}

static bool match(struct lexer* lexer, char expected) {
    if (is_end(lexer))
        return false;
    if (*lexer->current != expected)
        return false;
    lexer->current++;
    return true;
}

static char peek(const struct lexer* lexer) {
    return *lexer->current;
}

static char peek_next(const struct lexer* lexer) {
    if (is_end(lexer))
        return '\0';
    return lexer->current[1];
}

static void skip_whitespace(struct lexer* lexer) {
    while (true) {
        char c = peek(lexer);
        switch (c) {
            case '/': {
                if (peek_next(lexer) == '/') {
                    while (peek(lexer) != '\n' && !is_end(lexer)) {
                        advance(lexer);
                    }
                }
                else {
                    return;
                }
            }
            case '\n':
                lexer->line++;
            case ' ':
            case '\t':
                advance(lexer);
                break;
            default:
                return;
        }
    }
}

static struct token string(struct lexer* lexer) {
    while (peek(lexer) != '"' && !is_end(lexer)) {
        if (peek_next(lexer) == '\n') {
            lexer->line++;
        }
        advance(lexer);
    }

    assert(!is_end(lexer));

    advance(lexer); //Close quote
    return make_token(lexer, TOKEN_TYPE_STRING_LITERAL);
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static struct token number(struct lexer* lexer) {
    bool floating = false;

    while (is_digit(peek(lexer))) {
        advance(lexer);
    }

    if (peek(lexer) == '.' && is_digit(peek_next(lexer))) {
        floating = true;
        advance(lexer);

        while (is_digit(peek(lexer))) {
            advance(lexer);
        }
    }

    if (peek(lexer) == 'f') {
        floating = true;
        advance(lexer);
    }

    return make_token(lexer, floating ? TOKEN_TYPE_FLOATING : TOKEN_TYPE_INTEGER);
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static enum token_type check_keyword(struct lexer* lexer, uint32_t start, uint32_t length, const char* rest, enum token_type type) {
    if (lexer->current - lexer->start == start + length && memcmp(lexer->start + start, rest, length) == 0) {
        return type;
    }
    return TOKEN_TYPE_IDENTIFIER;
}

static enum token_type type(struct lexer* lexer) { //TODO: Test if this actually works!
    switch (*lexer->start) {
        case 'r': {
            if (lexer->current - lexer->start > 1 && lexer->start[1] == 'e' && lexer->current - lexer->start > 2) {
                switch (lexer->start[2]) {
                    case 'f': return check_keyword(lexer, 2, 0, "", TOKEN_TYPE_REF);
                    case 't': return check_keyword(lexer, 2, 4, "turn", TOKEN_TYPE_RETURN);
                }
            }
            break;
        }
        case 'v': return check_keyword(lexer, 1, 3, "oid", TOKEN_TYPE_VOID);
        case 'i': {
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case '8': return check_keyword(lexer, 2, 0, "", TOKEN_TYPE_I8);
                    case '1': return check_keyword(lexer, 2, 1, "6", TOKEN_TYPE_I16);
                    case '3': return check_keyword(lexer, 2, 1, "2", TOKEN_TYPE_I32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOKEN_TYPE_I64);
                    case 's': return check_keyword(lexer, 2, 3, "ize", TOKEN_TYPE_ISIZE);
                    case 'm': return check_keyword(lexer, 2, 4, "port", TOKEN_TYPE_IMPORT);
                    case 'f': return check_keyword(lexer, 2, 0, "", TOKEN_TYPE_IF);
                    case 'n': return check_keyword(lexer, 2, 7, "terface", TOKEN_TYPE_INTERFACE);
                    default: break;
                }
            }
            break;
        }
        case 'u': {
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case '8': return check_keyword(lexer, 2, 0, "", TOKEN_TYPE_U8);
                    case '1': return check_keyword(lexer, 2, 1, "6", TOKEN_TYPE_U16);
                    case '3': return check_keyword(lexer, 2, 1, "2", TOKEN_TYPE_U32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOKEN_TYPE_U64);
                    case 's': return check_keyword(lexer, 2, 3, "ize", TOKEN_TYPE_USIZE);
                    case 'n': return check_keyword(lexer, 2, 3, "ion", TOKEN_TYPE_UNION);
                    default: break;
                }
            }
            break;
        }
        case 's': {
            if (lexer->current - lexer->start > 1 && lexer->start[1] == 't' && lexer->current - lexer->start > 2) {
                switch (lexer->start[2]) {
                    case 'r': {
                        if (lexer->current - lexer->start > 3) {
                            switch (lexer->start[3]) {
                                case 'i': return check_keyword(lexer, 4, 2, "ng", TOKEN_TYPE_STRING);
                                case 'u': return check_keyword(lexer, 4, 2, "ct", TOKEN_TYPE_STRUCT);
                                default: break;
                            }
                        }
                        break;
                    }
                    case 'a': return check_keyword(lexer, 3, 3, "tic", TOKEN_TYPE_STATIC);
                    default: break;
                }
            }
            break;
        }
        case 'm': {
            if (lexer->current - lexer->start > 1 && lexer->start[1] == 'o' && lexer->current - lexer->start > 2) {
                switch (lexer->start[2]) {
                    case 'v': return check_keyword(lexer, 3, 1, "e", TOKEN_TYPE_MOVE);
                    case 'd': return check_keyword(lexer, 3, 3, "ule", TOKEN_TYPE_MODULE);
                    default: break;
                }
            }
            break;
        }
        case 'c': {
            if (lexer->current - lexer->start > 1 && lexer->start[1] == 'o' && lexer->current - lexer->start > 2) {
                switch (lexer->start[2]) {
                    case 'p': return check_keyword(lexer, 3, 1, "y", TOKEN_TYPE_COPY);
                    case 'n': return check_keyword(lexer, 3, 2, "st", TOKEN_TYPE_CONST);
                    default: break;
                }
            }
            break;
        }
        case 'n': return check_keyword(lexer, 1, 3, "ull", TOKEN_TYPE_NULL);
        case 't': return check_keyword(lexer, 1, 3, "rue", TOKEN_TYPE_TRUE);
        case 'f': {
            if (lexer->current - lexer->start > 1) {
                switch (lexer->start[1]) {
                    case 'a': return check_keyword(lexer, 2, 3, "lse", TOKEN_TYPE_FALSE);
                    case 'o': return check_keyword(lexer, 2, 1, "r", TOKEN_TYPE_FOR);
                    case '3': return check_keyword(lexer, 2, 1, "2", TOKEN_TYPE_F32);
                    case '6': return check_keyword(lexer, 2, 1, "4", TOKEN_TYPE_F64);
                    default: break;
                }
            }
        }
        case 'e': return check_keyword(lexer, 1, 3, "lse", TOKEN_TYPE_ELSE);
        case 'w': return check_keyword(lexer, 1, 4, "hile", TOKEN_TYPE_WHILE);
        case 'd': return check_keyword(lexer, 1, 1, "o", TOKEN_TYPE_DO);
        case 'o': return check_keyword(lexer, 1, 7, "perator", TOKEN_TYPE_OPERATOR);
        default: break;
    }
    return TOKEN_TYPE_IDENTIFIER;
}

static struct token identifier(struct lexer* lexer) {
    while (is_alpha(peek(lexer)) || is_digit(peek(lexer)))
        advance(lexer);
    return make_token(lexer, type(lexer));
}

struct token lexer_scan(struct lexer* lexer) {
    skip_whitespace(lexer);

    lexer->start = lexer->current;

    if (is_end(lexer)) {
        return make_token(lexer, TOKEN_TYPE_EOF);
    }

    char c = advance(lexer);

    if (is_alpha(c)) {
        return identifier(lexer);
    } else if (is_digit(c)) {
        return number(lexer);
    }

    //Common tokens
    switch (c) {
        case '(': return make_token(lexer, TOKEN_TYPE_LEFT_PAREN);
        case ')': return make_token(lexer, TOKEN_TYPE_RIGHT_PAREN);
        case '{': return make_token(lexer, TOKEN_TYPE_LEFT_BRACE);
        case '}': return make_token(lexer, TOKEN_TYPE_RIGHT_BRACE);
        case '[': return make_token(lexer, TOKEN_TYPE_LEFT_BRACKET);
        case ']': return make_token(lexer, TOKEN_TYPE_RIGHT_BRACKET);
        case ';': return make_token(lexer, TOKEN_TYPE_SEMICOLON);
        case '.': return make_token(lexer, TOKEN_TYPE_DOT);
        case ',': return make_token(lexer, TOKEN_TYPE_COMMA);
            
        case ':' : return make_token(lexer, match(lexer, ':') ? TOKEN_TYPE_COLON_COLON : TOKEN_TYPE_COLON);
        case '+': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_PLUS_EQUAL : match(lexer, '+') ? TOKEN_TYPE_PLUS_PLUS : TOKEN_TYPE_PLUS);
        case '-': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_MINUS_EQUAL : match(lexer, '-') ? TOKEN_TYPE_MINUS_MINUS : TOKEN_TYPE_MINUS);
        case '*': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_STAR_EQUAL : TOKEN_TYPE_STAR);
        case '%': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_PERCENT_EQUAL : TOKEN_TYPE_PERCENT);
        case '/': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_SLASH_EQUAL : TOKEN_TYPE_SLASH);
        case '=': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_EQUAL_EQUAL : TOKEN_TYPE_EQUAL);
        case '!': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_BANG_EQUAL : TOKEN_TYPE_BANG);
        case '<': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_LESS_EQUAL : TOKEN_TYPE_LESS);
        case '>': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_GREATER_EQUAL : TOKEN_TYPE_GREATER);

        case '&': return make_token(lexer, match(lexer, '&') ? TOKEN_TYPE_AND_AND : TOKEN_TYPE_AND);
        case '|': return make_token(lexer, match(lexer, '|') ? TOKEN_TYPE_PIPE_PIPE : TOKEN_TYPE_PIPE);

        case '"': return string(lexer);
        default: break;
    }

    return make_token(lexer, TOKEN_TYPE_ERROR);
}

struct lexer* lexer_new(char* source) {
    struct lexer* lexer = malloc(sizeof(struct lexer));
    lexer->source = source;
    lexer->start = source;
    lexer->current = source;
    lexer->line = 1;
    return lexer;
}

const char* token_type_to_string(enum token_type e) {
    switch (e) {
        case TOKEN_TYPE_ERROR: return "TOKEN_TYPE_ERROR";
        case TOKEN_TYPE_EOF: return "TOKEN_TYPE_EOF";
        case TOKEN_TYPE_LEFT_PAREN: return "TOKEN_TYPE_LEFT_PAREN";
        case TOKEN_TYPE_RIGHT_PAREN: return "TOKEN_TYPE_RIGHT_PAREN";
        case TOKEN_TYPE_LEFT_BRACE: return "TOKEN_TYPE_LEFT_BRACE";
        case TOKEN_TYPE_RIGHT_BRACE: return "TOKEN_TYPE_RIGHT_BRACE";
        case TOKEN_TYPE_LEFT_BRACKET: return "TOKEN_TYPE_LEFT_BRACKET";
        case TOKEN_TYPE_RIGHT_BRACKET: return "TOKEN_TYPE_RIGHT_BRACKET";
        case TOKEN_TYPE_SEMICOLON: return "TOKEN_TYPE_SEMICOLON";
        case TOKEN_TYPE_DOT: return "TOKEN_TYPE_DOT";
        case TOKEN_TYPE_COMMA: return "TOKEN_TYPE_COMMA";
        case TOKEN_TYPE_PLUS: return "TOKEN_TYPE_PLUS";
        case TOKEN_TYPE_PLUS_PLUS: return "TOKEN_TYPE_PLUS_PLUS";
        case TOKEN_TYPE_PLUS_EQUAL: return "TOKEN_TYPE_PLUS_EQUAL";
        case TOKEN_TYPE_MINUS: return "TOKEN_TYPE_MINUS";
        case TOKEN_TYPE_MINUS_MINUS: return "TOKEN_TYPE_MINUS_MINUS";
        case TOKEN_TYPE_MINUS_EQUAL: return "TOKEN_TYPE_MINUS_EQUAL";
        case TOKEN_TYPE_STAR: return "TOKEN_TYPE_STAR";
        case TOKEN_TYPE_STAR_EQUAL: return "TOKEN_TYPE_STAR_EQUAL";
        case TOKEN_TYPE_SLASH: return "TOKEN_TYPE_SLASH";
        case TOKEN_TYPE_SLASH_EQUAL: return "TOKEN_TYPE_SLASH_EQUAL";
        case TOKEN_TYPE_BANG: return "TOKEN_TYPE_BANG";
        case TOKEN_TYPE_BANG_EQUAL: return "TOKEN_TYPE_BANG_EQUAL";
        case TOKEN_TYPE_EQUAL: return "TOKEN_TYPE_EQUAL";
        case TOKEN_TYPE_EQUAL_EQUAL: return "TOKEN_TYPE_EQUAL_EQUAL";
        case TOKEN_TYPE_GREATER: return "TOKEN_TYPE_GREATER";
        case TOKEN_TYPE_GREATER_EQUAL: return "TOKEN_TYPE_GREATER_EQUAL";
        case TOKEN_TYPE_LESS: return "TOKEN_TYPE_LESS";
        case TOKEN_TYPE_LESS_EQUAL: return "TOKEN_TYPE_LESS_EQUAL";
        case TOKEN_TYPE_COLON: return "TOKEN_TYPE_COLON";
        case TOKEN_TYPE_COLON_COLON: return "TOKEN_TYPE_COLON_COLON";
        case TOKEN_TYPE_TILDE: return "TOKEN_TYPE_TILDE";
        case TOKEN_TYPE_MOVE: return "TOKEN_TYPE_MOVE";
        case TOKEN_TYPE_COPY: return "TOKEN_TYPE_COPY";
        case TOKEN_TYPE_STRING_LITERAL: return "TOKEN_TYPE_STRING_LITERAL";
        case TOKEN_TYPE_INTEGER: return "TOKEN_TYPE_INTEGER";
        case TOKEN_TYPE_FLOATING: return "TOKEN_TYPE_FLOATING";
        case TOKEN_TYPE_IDENTIFIER: return "TOKEN_TYPE_IDENTIFIER";
        case TOKEN_TYPE_MODULE: return "TOKEN_TYPE_MODULE";
        case TOKEN_TYPE_IMPORT: return "TOKEN_TYPE_IMPORT";
        case TOKEN_TYPE_RETURN: return "TOKEN_TYPE_RETURN";
        case TOKEN_TYPE_STRUCT: return "TOKEN_TYPE_STRUCT";
        case TOKEN_TYPE_UNION: return "TOKEN_TYPE_UNION";
        case TOKEN_TYPE_INTERFACE: return "TOKEN_TYPE_INTERFACE";
        case TOKEN_TYPE_STATIC: return "TOKEN_TYPE_STATIC";
        case TOKEN_TYPE_REF: return "TOKEN_TYPE_REF";
        case TOKEN_TYPE_CONST: return "TOKEN_TYPE_CONST";
        case TOKEN_TYPE_OPERATOR: return "TOKEN_TYPE_OPERATOR";
        case TOKEN_TYPE_UNIQUE: return "TOKEN_TYPE_UNIQUE";
        case TOKEN_TYPE_SHARED: return "TOKEN_TYPE_SHARED";
        case TOKEN_TYPE_VOID: return "TOKEN_TYPE_VOID";
        case TOKEN_TYPE_I8: return "TOKEN_TYPE_I8";
        case TOKEN_TYPE_I16: return "TOKEN_TYPE_I16";
        case TOKEN_TYPE_I32: return "TOKEN_TYPE_I32";
        case TOKEN_TYPE_I64: return "TOKEN_TYPE_I64";
        case TOKEN_TYPE_U8: return "TOKEN_TYPE_U8";
        case TOKEN_TYPE_U16: return "TOKEN_TYPE_U16";
        case TOKEN_TYPE_U32: return "TOKEN_TYPE_U32";
        case TOKEN_TYPE_U64: return "TOKEN_TYPE_U64";
        case TOKEN_TYPE_F32: return "TOKEN_TYPE_F32";
        case TOKEN_TYPE_F64: return "TOKEN_TYPE_F64";
        case TOKEN_TYPE_ISIZE: return "TOKEN_TYPE_ISIZE";
        case TOKEN_TYPE_USIZE: return "TOKEN_TYPE_USIZE";
        case TOKEN_TYPE_STRING: return "TOKEN_TYPE_STRING";
        case TOKEN_TYPE_NULL: return "TOKEN_TYPE_NULL";
        case TOKEN_TYPE_TRUE: return "TOKEN_TYPE_TRUE";
        case TOKEN_TYPE_FALSE: return "TOKEN_TYPE_FALSE";
        case TOKEN_TYPE_IF: return "TOKEN_TYPE_IF";
        case TOKEN_TYPE_ELSE: return "TOKEN_TYPE_ELSE";
        case TOKEN_TYPE_WHILE: return "TOKEN_TYPE_WHILE";
        case TOKEN_TYPE_DO: return "TOKEN_TYPE_DO";
        case TOKEN_TYPE_FOR: return "TOKEN_TYPE_FOR";
        default: return "unknown";
    }
}

void lexer_free(struct lexer* lexer) {
    free(lexer);
}

struct token lexer_peek(struct lexer* lexer) {
    char* saved_start = lexer->start;
    char* saved_current = lexer->current;
    uint32_t saved_line = lexer->line;

    struct token next = lexer_scan(lexer);

    lexer->start = saved_start;
    lexer->current = saved_current;
    lexer->line = saved_line;

    return next;
}

void lexer_reset(struct lexer* lexer) {
    lexer->current = lexer->source;
    lexer->start = lexer->source;
    lexer->line = 1;
}
