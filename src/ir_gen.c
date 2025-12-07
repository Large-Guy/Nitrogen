#include "ir_gen.h"

#include <stdlib.h>

static struct ir* statement(struct ast_node* node)
{
    switch (node->type) {
        case AST_NODE_TYPE_FUNCTION: {

        }
        default: {
            return NULL;
        }
    }
}

struct ir_module* ir_gen_module(struct module* module) {
    struct ir_module* chunks = ir_module_new(module->name);

    for (int i = 0; i < module->root->children_count; i++) {
        ir_module_append(chunks, statement(module->root->children[i]));
    }
    
    return chunks;
}
