#include "unit.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "block.h"

struct unit* unit_new(char* symbol, bool global, enum unit_type type)
{
    struct unit* chunk = malloc(sizeof(struct unit));
    assert(chunk);

    chunk->arguments = malloc(sizeof(struct operand));
    assert(chunk->arguments);
    chunk->argument_count = 0;
    chunk->argument_capacity = 1;

    chunk->symbol = malloc(strlen(symbol) + 1);
    assert(chunk->symbol);
    strcpy(chunk->symbol, symbol);
    chunk->symbol[strlen(symbol)] = '\0';

    chunk->type = type;

    chunk->global = global;

    chunk->blocks = malloc(sizeof(struct block*));
    assert(chunk->blocks);
    chunk->block_count = 0;
    chunk->block_capacity = 1;

    return chunk;
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

void unit_module_append(struct unit_module* list, struct unit* chunk)
{
    if (list->unit_count >= list->unit_capacity)
    {
        list->unit_capacity *= 2;
        list->units = realloc(list->units, list->unit_capacity * sizeof(struct unit*));
        assert(list->units);
    }
    list->units[list->unit_count++] = chunk;
}

void unit_free(struct unit* chunk)
{
    assert(chunk != NULL);
    free(chunk->symbol);
    for (int i = 0; i < chunk->block_count; i++)
    {
        block_free(chunk->blocks[i]);
    }
    free(chunk->blocks);
    free(chunk);
}

struct unit* unit_module_find(struct unit_module* module, struct token symbol)
{
    for (int i = 0; i < module->unit_count; i++)
    {
        struct unit* chunk = module->units[i];
        if (symbol.length == strlen(chunk->symbol) && memcmp(chunk->symbol, symbol.start, symbol.length) == 0)
        {
            return module->units[i];
        }
    }
    return NULL;
}

void unit_add(struct unit* chunk, struct block* block)
{
    assert(chunk != NULL);
    if (chunk->block_count >= chunk->block_capacity)
    {
        chunk->block_capacity *= 2;
        chunk->blocks = realloc(chunk->blocks, chunk->block_capacity * sizeof(struct block*));
        assert(chunk->blocks);
    }
    chunk->blocks[chunk->block_count++] = block;
    block->id = chunk->block_count;
}

void unit_arg(struct unit* chunk, struct operand arg)
{
    assert(chunk != NULL);
    if (chunk->argument_count >= chunk->argument_capacity)
    {
        chunk->argument_capacity *= 2;
        chunk->arguments = realloc(chunk->arguments, chunk->argument_capacity * sizeof(struct operand));
        assert(chunk->arguments);
    }
    chunk->arguments[chunk->argument_count++] = arg;
}

char* unit_compile(struct unit* chunk, FILE* out)
{
    //TODO: compile to ARM64
}
