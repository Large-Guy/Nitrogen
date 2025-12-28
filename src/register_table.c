#include "register_table.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"

struct register_table* register_table_new()
{
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

void register_table_free(struct register_table* table)
{
    free(table->symbols);
    free(table);
}

void register_table_begin(struct register_table* table)
{
    table->current_scope++;
}

void register_table_end(struct register_table* table)
{
    table->current_scope--;
}

struct variable* register_table_lookup(struct register_table* table, struct token name)
{
    struct variable* result = NULL;
    for (int i = 0; i < table->symbol_count; i++)
    {
        struct variable* symbol = &table->symbols[i];
        if (name.length == symbol->name.length &&
            memcmp(name.start, symbol->name.start, name.length) == 0 &&
            symbol->scope <= table->current_scope)
        {
            if (result != NULL && symbol->scope <= result->scope)
                continue;
            result = symbol;
        }
    }
    return result;
}

struct variable* register_table_add(struct register_table* table, struct token name, struct ssa_type type)
{
    if (table->symbol_count >= table->symbol_capacity)
    {
        table->symbol_capacity *= 2;
        table->symbols = realloc(table->symbols, sizeof(struct variable) * table->symbol_capacity);
        assert(table->symbols);
    }
    struct variable symbol = {};
    symbol.name = name;
    symbol.scope = table->current_scope;
    struct ast_node* reference = ast_node_new(AST_NODE_TYPE_REFERENCE, token_null);
    ast_node_append_child(reference, ast_node_clone(type.type));
    symbol.type = get_node_type(type.module, ast_node_clone(type.type));
    symbol.pointer = register_table_alloc(table, get_node_type(type.module, reference));
    table->symbols[table->symbol_count] = symbol;
    return &table->symbols[table->symbol_count++];
}

struct operand register_table_alloc(struct register_table* table, struct ssa_type type)
{
    return operand_reg(table->register_count++, type);
}
