#ifndef COMPILER_REGISTER_TABLE_H
#define COMPILER_REGISTER_TABLE_H

#include <stdint.h>
#include "ssa.h"
#include "lexer.h"


struct variable {
    struct token name;
    uint64_t size;
    uint32_t scope;
    struct ssa_type type;
    struct operand pointer;
};

struct register_table {
    struct variable* symbols;
    uint32_t symbol_count;
    uint32_t symbol_capacity;

    uint32_t symbol_stack_size;

    uint32_t current_scope;
   
    uint32_t register_count;
};

struct register_table* register_table_new();

void register_table_free(struct register_table* table);

void register_table_begin(struct register_table* table);

void register_table_end(struct register_table* table);

struct variable* register_table_lookup(struct register_table* table, struct token name);

struct variable* register_table_add(struct register_table* table, struct token name, struct ssa_type type);

struct operand register_table_alloc(struct register_table* table, struct ssa_type type);

#endif //COMPILER_REGISTER_TABLE_H