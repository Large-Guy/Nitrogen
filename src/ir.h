#ifndef COMPILER_CHUNK_H
#define COMPILER_CHUNK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "lexer.h"
#include "ssa.h"

enum chunk_type {
    CHUNK_TYPE_FUNCTION,
    CHUNK_TYPE_VARIABLE,
};


struct ir {
    char* symbol;
    enum chunk_type type;
    enum ssa_type return_type;
    bool global;

    struct block** blocks;
    uint32_t block_count;
    uint32_t block_capacity;
};

struct ir_module {
    char* name;

    struct ir** chunks;
    size_t count;
    size_t capacity;
};

struct ir_module* ir_module_new(char* name);

void ir_module_free(struct ir_module* list);

void ir_module_append(struct ir_module* list, struct ir* chunk);

struct ir* ir_module_find(struct ir_module* list, struct token symbol);

struct ir* ir_new(char* symbol, bool global, enum chunk_type type);

void ir_free(struct ir* chunk);

void ir_add(struct ir* chunk, struct block* block);

void ir_debug(struct ir* chunk);

void ir_build_graph(struct ir* chunk, FILE* out);

void ir_module_debug(struct ir_module* module);

void ir_module_debug_graph(struct ir_module* module, FILE* out);

char* ir_compile(struct ir* chunk, FILE* file);

#endif //COMPILER_CHUNK_H