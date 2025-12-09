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

static void statement(struct ir* ir, struct ast_node* node) {
    switch (node->type) {
        case AST_NODE_TYPE_SEQUENCE: {
            for (int i = 0; i < node->children_count; i++) {
                struct ast_node* child = node->children[i];
                statement(ir, child);
            }
        }
        case AST_NODE_TYPE_INTEGER: {
            ir_push(ir, OP_IMM_32);
            int64_t immediate = strtoll(node->token.start, NULL, 10);
            ir_push32(ir, immediate);
            break;
        }
        case AST_NODE_TYPE_ADD: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            statement(ir, a);
            statement(ir, b);
            ir_push(ir, OP_IADD);
            break;
        }
        case AST_NODE_TYPE_SUBTRACT: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            statement(ir, a);
            statement(ir, b);
            ir_push(ir, OP_ISUB);
            break;
        }
        case AST_NODE_TYPE_MULTIPLY: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            statement(ir, a);
            statement(ir, b);
            ir_push(ir, OP_IMUL);
            break;
        }
        case AST_NODE_TYPE_DIVIDE: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            statement(ir, a);
            statement(ir, b);
            ir_push(ir, OP_ISDIV);
            break;
        }
        case AST_NODE_TYPE_RETURN_STATEMENT: {
            struct ast_node* a = node->children[0];
            statement(ir, a);
            ir_push(ir, OP_RETURN);
            break;
        }
        default: {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
        }
    }
}

static struct ir* definition(struct ast_node* node)
{
    switch (node->type)
    {
    case AST_NODE_TYPE_FUNCTION:
        {
            struct ir* ir = ir_symbol_new(node->children[0]->token);
            struct ast_node* type = node->children[1]; //type
            struct ast_node* body = node->children[2]; //function body
            statement(ir, body);
            return ir;
        }
    default:
        {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
            return NULL;
        }
    }
}

struct ir_module* ir_gen_module(struct module* module)
{
    struct ir_module* chunks = ir_module_new(module->name);

    for (int i = 0; i < module->root->children_count; i++)
    {
        ir_module_append(chunks, definition(module->root->children[i]));
    }

    return chunks;
}
