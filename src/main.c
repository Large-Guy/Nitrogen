#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct chunk {
    char* symbol;
    uint8_t* code;
    size_t capacity;
    size_t size;
};

struct chunk* chunk_new(char* symbol);

void chunk_free(struct chunk* chunk);

void chunk_push(struct chunk* chunk, uint8_t byte);

void chunk_debug(struct chunk* chunk);

struct chunk* chunk_new(char* symbol) {
    struct chunk* chunk = malloc(sizeof(struct chunk));
    assert(chunk);
    chunk->symbol = malloc(strlen(symbol) + 1);
    assert(chunk->symbol);
    strcpy(chunk->symbol, symbol);
    chunk->code = malloc(1);
    chunk->capacity = 1;
    chunk->size = 0;
    return chunk;
}

void chunk_free(struct chunk* chunk) {
    assert(chunk != NULL);
    free(chunk->symbol);
    free(chunk->code);
    free(chunk);
}

void chunk_push(struct chunk* chunk, uint8_t byte) {
    if (chunk->size >= chunk->capacity) {
        chunk->capacity *= 2;
        chunk->code = realloc(chunk->code, chunk->capacity);
        assert(chunk->code);
    }
    chunk->code[chunk->size++] = byte;
}

void chunk_debug(struct chunk* chunk) {
    assert(chunk != NULL);
    assert(chunk->code != NULL);
    for (size_t i = 0; i < chunk->size; i++) {
        printf("Byte: %lu, %d\n", i, chunk->code[i]);
    }
}

enum token_type {
    TOKEN_TYPE_RETURN,
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_LEFT_PAREN,
    TOKEN_TYPE_RIGHT_PAREN,
    TOKEN_TYPE_LEFT_BRACE,
    TOKEN_TYPE_RIGHT_BRACE,
    TOKEN_TYPE_LEFT_BRACKET,
    TOKEN_TYPE_RIGHT_BRACKET,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_DOT,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_PLUS,
    TOKEN_TYPE_MINUS,
    TOKEN_TYPE_STAR,
    TOKEN_TYPE_SLASH,
    TOKEN_TYPE_BANG,
    TOKEN_TYPE_BANG_EQUAL,
    TOKEN_TYPE_EQUAL,
    TOKEN_TYPE_EQUAL_EQUAL,
    TOKEN_TYPE_GREATER,
    TOKEN_TYPE_GREATER_EQUAL,
    TOKEN_TYPE_LESS,
    TOKEN_TYPE_LESS_EQUAL,
};

struct lexer {
    char* start;
    char* current;
    uint32_t line;
};

struct token {
    enum token_type type;
    char* start;
    size_t length;
    uint32_t line;
};

void lexer_scan(struct chunk* output, char* text);

static bool is_end(const struct lexer* lexer) {
    return *lexer->current == '\0';
}

static struct token make_token(const struct lexer* lexer, enum token_type type) {
    struct token token;
    token.start = lexer->current;
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
            case '\n':
                lexer->line++;
            case '/': {
                if (peek_next(lexer) != '/') {
                    while (peek(lexer) != '\n' && !is_end(lexer)) {
                        advance(lexer);
                    }
                }
                else {
                    return;
                }
            }
            case ' ':
            case '\t':
                advance(lexer);
                break;
            default:
                return;
        }
    }
}

static struct token scan_token(struct lexer* lexer) {
    lexer->start = lexer->current;

    if (is_end(lexer)) {
        return make_token(lexer, TOKEN_TYPE_EOF);
    }

    char c = advance(lexer);

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
        case '+': return make_token(lexer, TOKEN_TYPE_PLUS);
        case '-': return make_token(lexer, TOKEN_TYPE_MINUS);
        case '*': return make_token(lexer, TOKEN_TYPE_STAR);
        case '/': return make_token(lexer, TOKEN_TYPE_SLASH);
        //Special cases
        case '=': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_EQUAL_EQUAL : TOKEN_TYPE_EQUAL);
        case '!': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_BANG_EQUAL : TOKEN_TYPE_BANG);
        case '<': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_LESS_EQUAL : TOKEN_TYPE_LESS);
        case '>': return make_token(lexer, match(lexer, '=') ? TOKEN_TYPE_GREATER_EQUAL : TOKEN_TYPE_GREATER);
        default: break;
    }
}

void lexer_scan(struct chunk* output, char* text) {
    struct lexer lexer;
    lexer.start = text;
    lexer.current = text;
    lexer.line = 1;

    while (true) {
        struct token token = scan_token(&lexer);
        
        if (token.type == TOKEN_TYPE_EOF)
            break;

        printf("Token: %d\n", token.type);
    }
}


int main(void) {
    struct chunk* main = chunk_new("_main");

    lexer_scan(main, "()!=");
    
    chunk_push(main, 1);
    chunk_push(main, 2);
    chunk_push(main, 3);
    
    chunk_free(main);
    return 0;
}