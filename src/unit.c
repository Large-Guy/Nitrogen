#include "unit.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "block.h"

struct unit* unit_new(char* symbol, bool global, enum unit_type type)
{
    struct unit* unit = malloc(sizeof(struct unit));
    assert(unit);

    unit->arguments = malloc(sizeof(struct ssa_operand));
    assert(unit->arguments);
    unit->argument_count = 0;
    unit->argument_capacity = 1;

    unit->symbol = malloc(strlen(symbol) + 1);
    assert(unit->symbol);
    strcpy(unit->symbol, symbol);
    unit->symbol[strlen(symbol)] = '\0';

    unit->type = type;

    unit->global = global;

    unit->blocks = malloc(sizeof(struct block*));
    assert(unit->blocks);
    unit->block_count = 0;
    unit->block_capacity = 1;
    
    unit->data = malloc(sizeof(struct ssa_operand));
    assert(unit->data);
    unit->data_count = 0;
    unit->data_capacity = 1;

    return unit;
}

struct unit_module* unit_module_new(char* name)
{
    struct unit_module* list = malloc(sizeof(struct unit_module));
    assert(list);
    list->name = malloc(strlen(name) + 1);
    memcpy(list->name, name, strlen(name) + 1);
    list->units = malloc(sizeof(struct unit*));
    list->unit_count = 0;
    list->unit_capacity = 1;
    return list;
}

void unit_module_free(struct unit_module* list)
{
    assert(list);
    free(list->name);
    for (int i = 0; i < list->unit_count; i++)
    {
        unit_free(list->units[i]);
    }
    free(list->units);
    free(list);
}

void unit_module_append(struct unit_module* list, struct unit* unit)
{
    if (list->unit_count >= list->unit_capacity)
    {
        list->unit_capacity *= 2;
        list->units = realloc(list->units, list->unit_capacity * sizeof(struct unit*));
        assert(list->units);
    }
    list->units[list->unit_count++] = unit;
}

void unit_free(struct unit* unit)
{
    assert(unit != NULL);
    free(unit->symbol);
    for (int i = 0; i < unit->block_count; i++)
    {
        block_free(unit->blocks[i]);
    }
    free(unit->blocks);
    free(unit);
}

struct unit* unit_module_find(struct unit_module* module, struct token symbol)
{
    for (int i = 0; i < module->unit_count; i++)
    {
        struct unit* unit = module->units[i];
        if (symbol.length == strlen(unit->symbol) && memcmp(unit->symbol, symbol.start, symbol.length) == 0)
        {
            return module->units[i];
        }
    }
    return NULL;
}

void unit_add(struct unit* unit, struct block* block)
{
    assert(unit != NULL);
    if (unit->block_count >= unit->block_capacity)
    {
        unit->block_capacity *= 2;
        unit->blocks = realloc(unit->blocks, unit->block_capacity * sizeof(struct block*));
        assert(unit->blocks);
    }
    unit->blocks[unit->block_count++] = block;
    block->id = unit->block_count;
}

void unit_arg(struct unit* unit, struct ssa_operand arg)
{
    assert(unit != NULL);
    if (unit->argument_count >= unit->argument_capacity)
    {
        unit->argument_capacity *= 2;
        unit->arguments = realloc(unit->arguments, unit->argument_capacity * sizeof(struct ssa_operand));
        assert(unit->arguments);
    }
    unit->arguments[unit->argument_count++] = arg;
}

void unit_compile(struct backend *backend, struct unit *unit) {
    switch (unit->type) {
        case UNIT_TYPE_FUNCTION:
            backend->compile_function(backend, unit);
            break;
        case UNIT_TYPE_VARIABLE:
            backend->compile_variable(backend, unit);
            break;
    }
}

uint32_t unit_add_data(struct unit* unit, struct ssa_operand* data, uint32_t size) {
    assert(unit != NULL);
    uint32_t start = unit->data_count;
    
    unit->data_count += size;
    
    while (unit->data_count >= unit->data_capacity)
        unit->data_capacity *= 2;
    
    unit->data = realloc(unit->data, unit->data_capacity * sizeof(struct ssa_operand));
    
    for (uint32_t i = 0; i < size; i++) {
        unit->data[i + start] = data[i];
    }
    
    return start;
}
