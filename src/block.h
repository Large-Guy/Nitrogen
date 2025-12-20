#ifndef COMPILER_CFG_H
#define COMPILER_CFG_H
#include <stdbool.h>
#include <stdint.h>
#include "ssa.h"
#include "register_table.h"

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