#ifndef COMPILER_CHUNK_H
#define COMPILER_CHUNK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "lexer.h"
#include "ssa.h"

enum unit_type {
    CHUNK_TYPE_FUNCTION,
    CHUNK_TYPE_VARIABLE,
};

struct unit {
    char* symbol;
    enum unit_type type;
    struct ssa_type return_type;
    bool global;

    struct operand* arguments;
    uint32_t argument_count;
    uint32_t argument_capacity;

    struct block** blocks;
    uint32_t block_count;
    uint32_t block_capacity;
};

struct unit_module {
    char* name;

    struct unit** units;
    size_t unit_count;
    size_t unit_capacity;

    struct ast_module* ast;
};

struct unit_module* unit_module_new(char* name);

void unit_module_free(struct unit_module* list);

void unit_module_append(struct unit_module* list, struct unit* chunk);

struct unit* unit_module_find(struct unit_module* list, struct token symbol);

struct unit* unit_new(char* symbol, bool global, enum unit_type type);

void unit_free(struct unit* chunk);

void unit_add(struct unit* chunk, struct block* block);

void unit_arg(struct unit* chunk, struct operand arg);

void unit_debug(struct unit* chunk);

void unit_build_graph(struct unit* chunk, FILE* out);

void unit_module_debug(struct unit_module* module);

void unit_module_debug_graph(struct unit_module* module, FILE* out);

char* unit_compile(struct unit* chunk, FILE* file);

#endif //COMPILER_CHUNK_H