#ifndef COMPILER_CFG_H
#define COMPILER_CFG_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "ssa.h"
#include "lexer.h"

struct variable {
    struct token name;
    uint64_t size;
    uint32_t scope;
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

struct variable* register_table_add(struct register_table* table, struct token name, uint64_t size);

struct operand register_table_alloc(struct register_table* table);

struct block {
    uint32_t id;
    bool entry;
    
    struct register_table* symbol_table;

    struct block** parents;
    uint32_t parents_count;
    uint32_t parents_capacity;
    
    struct block** children;
    uint32_t children_count;
    uint32_t children_capacity;

    struct ssa_instruction* instructions;
    uint32_t instructions_count;
    uint32_t instructions_capacity;

    struct ssa_instruction* exit;
    bool branches;
};

struct block* block_new(bool entry, struct register_table* symbol_table);

void block_free(struct block* node);

void block_link(struct block* parent, struct block* child);

void block_add(struct block* block, struct ssa_instruction instruction);

#endif //COMPILER_CFG_H