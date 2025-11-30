#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "lexer.h"
#include "parser.h"

struct file {
    FILE* file;
    size_t size;
    char* contents;
};

struct file* file_read(const char* filename) {
    struct file* file = malloc(sizeof(struct file));
    file->file = fopen(filename, "r");
    if (file->file == NULL) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file->file, 0L, SEEK_END);
    file->size = ftell(file->file);

    rewind(file->file);
    file->contents = malloc(file->size + 1);
    if (file->contents == NULL) {
        fprintf(stderr, "Failed to allocate memory for file contents\n");
        exit(EXIT_FAILURE);
    }

    fread(file->contents, sizeof(char), file->size, file->file);
    file->contents[file->size] = '\0';

    return file;
}

void file_close(struct file* file) {
    if (file->contents != NULL) {
        free(file->contents);
    }
    free(file);
}

int main(void) {
    struct file* input = file_read("main.cl");

    printf(input->contents);
    printf("\n\n");

    struct lexer* lexer = lexer_new(input->contents);

    while (true) {
        struct token token = lexer_scan(lexer);
        if (token.type == TOKEN_TYPE_EOF) {
            break;
        }
        printf("Token - type-id: %d, string-name: %s, contents: %.*s\n", token.type, token_type_to_string(token.type), (uint32_t)token.length, token.start);
    }

    printf("\nbuilding ast...\n\n");

    lexer_reset(lexer);

    struct ast_node* root = ast_node_build(lexer);

    printf("\n");

    ast_node_debug(root);

    printf("\nfinished building ast...\n\n");
    
    file_close(input);
    
    struct chunk* chunk = chunk_new("main");

    int a = chunk_declare(chunk, 4);
    int b = chunk_declare(chunk, 4);
    
    chunk_push(chunk, OP_IMM_32);
    chunk_push32(chunk, 32);
    chunk_push(chunk, OP_SET);
    chunk_push(chunk, a);

    chunk_push(chunk, OP_IMM_32);
    chunk_push32(chunk, 16);
    chunk_push(chunk, OP_SET);
    chunk_push(chunk, b);

    chunk_push(chunk, OP_GET);
    chunk_push(chunk, a);
    
    chunk_push(chunk, OP_GET);
    chunk_push(chunk, b);

    chunk_push(chunk, OP_ADD_32);
    
    chunk_push(chunk, OP_RETURN);

    FILE* file = fopen("output.s", "w");
    
    chunk_compile(chunk, file);

    fclose(file);

    chunk_free(chunk);

    ast_node_free(root);
    
    return 0;
}
