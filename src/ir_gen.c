#include "ir_gen.h"

#include <stdlib.h>
#include <string.h>

static struct ir* ir_symbol_new(struct token symbol)
{
    char* symbol_name = malloc(symbol.length + 1);
    memcpy(symbol_name, symbol.start, symbol.length);
    symbol_name[symbol.length] = '\0';
    struct ir* ir = ir_new(symbol_name);
    free(symbol_name);
    return ir;
}

static struct ir* statement(struct ast_node* node)
{
    switch (node->type)
    {
    case AST_NODE_TYPE_FUNCTION:
        {
            struct ir* ir = ir_symbol_new(node->children[0]->token);
            return ir;
        }
    default:
        {
            return NULL;
        }
    }
}

struct ir_module* ir_gen_module(struct module* module)
{
    struct ir_module* chunks = ir_module_new(module->name);

    for (int i = 0; i < module->root->children_count; i++)
    {
        ir_module_append(chunks, statement(module->root->children[i]));
    }

    return chunks;
}
