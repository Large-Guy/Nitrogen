#include "unit_module_gen.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "parser.h"

static struct unit* unit_symbol_new(struct token symbol, enum unit_type type)
{
    assert(symbol.length != 0);
    char* symbol_name = malloc(symbol.length + 1);
    memcpy(symbol_name, symbol.start, symbol.length);
    symbol_name[symbol.length] = '\0';
    struct unit* unit = unit_new(symbol_name, symbol.start[0] != '_', type);
    free(symbol_name);
    return unit;
}

static struct unit* forward(struct ast_module* module, struct ast_node* node) {
    switch (node->type)
    {
        case AST_NODE_TYPE_FUNCTION:
        {
            struct unit* unit = unit_symbol_new(node->children[0]->token, CHUNK_TYPE_FUNCTION);
            struct ast_node* type = node->children[1]; //type
            unit->global = node->children[1]->token.start[0] != '_';
            unit->return_type = ssa_type_from_ast(module, type);
            return unit;
        }
        case AST_NODE_TYPE_VARIABLE:
        {
            struct unit* unit = unit_symbol_new(node->children[0]->token, CHUNK_TYPE_VARIABLE);
            return unit;
        }
        default:
        {
            fprintf(stderr, "unexpected node type: %s\n", ast_node_get_name(node));
            return NULL;
        }
    }
}

struct unit_module* unit_module_forward(struct ast_module* module)
{
    struct unit_module* unit_module = unit_module_new(module->name);

    unit_module->ast = module;

    for (int i = 0; i < module->root->children_count; i++) {
        struct unit* unit = forward(module, module->root->children[i]);
        if (unit == NULL)
        {
            //TODO: error out
            continue; //NOTE: skip for now
        }
        unit_module_append(unit_module, unit);
    }

    return unit_module;
}
