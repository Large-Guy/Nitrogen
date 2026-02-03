#ifndef COMPILER_REGISTER_TABLE_H
#define COMPILER_REGISTER_TABLE_H

#include <stdint.h>
#include "ssa.h"
#include "lexer.h"


struct ssa_variable {
    struct token name;
    uint64_t size;
    uint32_t scope;
    struct ssa_type type;
    struct ssa_operand pointer;
};

struct ssa_register_table {
    struct ssa_variable* symbols;
    uint32_t symbol_count;
    uint32_t symbol_capacity;

    uint32_t symbol_stack_size;

    uint32_t current_scope;
   
    uint32_t register_count;
};

struct ssa_register_table* register_table_new();

void register_table_free(struct ssa_register_table* table);

void register_table_begin(struct ssa_register_table* table);

void register_table_end(struct ssa_register_table* table);

struct ssa_variable* register_table_lookup(struct ssa_register_table* table, struct token name);

struct ssa_variable* register_table_add(struct ssa_register_table* table, struct token name, struct ssa_type type);

struct ssa_operand register_table_alloc(struct ssa_register_table* table, struct ssa_type type);

#endif //COMPILER_REGISTER_TABLE_H