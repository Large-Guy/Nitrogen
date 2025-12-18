#include "block.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct register_table* register_table_new() {
    struct register_table* table = malloc(sizeof(struct register_table));
    assert(table);
    table->symbols = malloc(sizeof(struct variable));
    assert(table->symbols);
    table->symbol_count = 0;
    table->symbol_capacity = 1;

    table->current_scope = 0;
   
    table->register_count = 0;
    return table;
}

void register_table_free(struct register_table* table) {
    free(table->symbols);
    free(table);
}

void register_table_begin(struct register_table* table) {
    table->current_scope++;
}

void register_table_end(struct register_table* table) {
    table->current_scope--;
}

struct variable* register_table_lookup(struct register_table* table, struct token name) {
    struct variable* result = NULL;
    for (int i = 0; i < table->symbol_count; i++) {
        struct variable* symbol = &table->symbols[i];
        if (name.length == symbol->name.length &&
            memcmp(name.start, symbol->name.start, name.length) == 0 &&
            symbol->scope <= table->current_scope) {
            if (result != NULL && symbol->scope <= result->scope) 
                continue;
            result = symbol;
        }
    }
    return result;
    
}

struct variable* register_table_add(struct register_table* table, struct token name, enum ssa_type type) {
    if (table->symbol_count >= table->symbol_capacity) {
        table->symbol_capacity *= 2;
        table->symbols = realloc(table->symbols, sizeof(struct variable) * table->symbol_capacity);
        assert(table->symbols);
    }
    struct variable symbol = {};
    symbol.name = name;
    symbol.scope = table->current_scope;
    symbol.pointer = register_table_alloc(table, type);
    table->symbols[table->symbol_count] = symbol;
    return &table->symbols[table->symbol_count++];
}

struct operand register_table_alloc(struct register_table* table, enum ssa_type type) {
    return operand_reg(table->register_count++, type);
}


struct block* block_new(bool entry, struct register_table* symbol_table) {
    struct block* node = malloc(sizeof(struct block));
    assert(node);
    node->id = 0;
    node->entry = entry;

    node->symbol_table = symbol_table;

    node->parents = malloc(sizeof(struct block*));
    assert(node->parents);
    node->parents_count = 0;
    node->parents_capacity = 1;
    
    node->children = malloc(sizeof(struct block*));
    assert(node->children);
    node->children_count = 0;
    node->children_capacity = 1;

    node->instructions = malloc(sizeof(struct ssa_instruction));
    assert(node->instructions);
    node->instructions_count = 0;
    node->instructions_capacity = 1;

    node->exit = NULL;
    
    return node;
}

void block_free(struct block* node) {
    assert(node);
    free(node->children);
    free(node);
}

void block_link(struct block* parent, struct block* child) {
    assert(parent);
    assert(child);

    if (parent->children_count >= parent->children_capacity) {
        parent->children_capacity *= 2;
        parent->children = realloc(parent->children, parent->children_capacity * sizeof(struct block*));
        assert(parent->children);
    }
    parent->children[parent->children_count++] = child;

    if (child->parents_count >= child->parents_capacity) {
        child->parents_capacity *= 2;
        child->parents = realloc(child->parents, child->parents_capacity * sizeof(struct block*));
        assert(child->parents);
    }

    child->parents[child->parents_count++] = parent;
}

void block_add(struct block* block, struct ssa_instruction instruction) {
    assert(block);

    if (block->instructions_count >= block->instructions_capacity) {
        block->instructions_capacity *= 2;
        block->instructions = realloc(block->instructions, block->instructions_capacity * sizeof(struct ssa_instruction));
        assert(block->instructions);
    }
    block->instructions[block->instructions_count++] = instruction;
    block->exit = &block->instructions[block->instructions_count-1];
    block->branches = instruction.result.type == OPERAND_TYPE_END;
}

