#include "ast.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


struct ast_node* ast_node_new(enum ast_node_type type, struct token token)
{
    struct ast_node* node = malloc(sizeof(struct ast_node));
    assert(node);
    node->type = type;
    node->token = token;
    node->parent = NULL;
    node->children_count = 0;
    node->children_capacity = 1;
    node->children = malloc(sizeof(struct ast_node*) * node->children_capacity);
    assert(node->children);
    return node;
}

void ast_node_free(struct ast_node* node)
{
    if (node->parent != NULL)
    {
        ast_node_remove_child(node->parent, node);
    }
    for (size_t i = 0; i < node->children_count; i++)
    {
        ast_node_free(node->children[i]);
    }
    free(node->children);
    free(node);
}

struct ast_node* ast_node_clone(struct ast_node* node)
{
    struct ast_node* copy = ast_node_new(node->type, node->token);

    for (size_t i = 0; i < node->children_count; i++)
    {
        ast_node_append_child(copy, ast_node_clone(node->children[i]));
    }

    return copy;
}

void ast_node_append_child(struct ast_node* node, struct ast_node* child)
{
    if (child == NULL)
    {
        return;
    }
    assert(node->children_capacity != 0); // if this occurs, the node was not initialized properly
    if (node->children_count >= node->children_capacity)
    {
        node->children_capacity *= 2;
        node->children = realloc(node->children, node->children_capacity * sizeof(struct ast_node*));
        assert(node->children);
    }
    node->children[node->children_count++] = child;
    child->parent = node;
}

void ast_node_remove_child(struct ast_node* node, struct ast_node* child)
{
    for (size_t i = 0; i < node->children_count; i++)
    {
        if (node->children[i] == child)
        {
            for (size_t j = i; j < node->children_count - 1; j++)
            {
                node->children[j] = node->children[j + 1];
            }
            node->children_count--;
            child->parent = NULL;
            break;
        }
    }
}

struct array_header
{
    size_t length;
    size_t capacity;
};

size_t to_power_of_two(size_t x)
{
    if (x <= 1) return 1;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    return x + 1;
}

size_t ast_node_symbol_size(struct ast_module* module, struct ast_node* node)
{
    switch (node->type)
    {
        case AST_NODE_TYPE_BOOL:
        case AST_NODE_TYPE_U8:
        case AST_NODE_TYPE_I8: return 1;
        case AST_NODE_TYPE_U16:
        case AST_NODE_TYPE_I16: return 2;
        case AST_NODE_TYPE_U32:
        case AST_NODE_TYPE_I32: return 4;
        case AST_NODE_TYPE_U64:
        case AST_NODE_TYPE_I64: return 8;

        case AST_NODE_TYPE_F32: return 4;
        case AST_NODE_TYPE_F64: return 8;

        case AST_NODE_TYPE_VOID: return 0;

        case AST_NODE_TYPE_POINTER:
        case AST_NODE_TYPE_REFERENCE: return sizeof(void*); //TODO: research if you can really just assume this

        case AST_NODE_TYPE_ARRAY: return sizeof(void*) + sizeof(struct array_header);

        case AST_NODE_TYPE_SIMD: return ast_node_symbol_size(module, node->children[0]) * to_power_of_two(strtol(
                node->children[1]->token.start, NULL, 10));
        case AST_NODE_TYPE_STRUCT: {
            size_t total = 0;
            for (size_t i = 0; i < node->children_count; i++) {
                struct ast_node* child = node->children[i];
                if (child->type == AST_NODE_TYPE_FIELD) {
                    struct ast_node* type = child->children[1];
                    total += ast_node_symbol_size(module, type);
                }
            }
            return total;
        }
            
        default:
            fprintf(stderr, "expected a built-in type node\n");
            return 0;
    }
}

struct ast_node* ast_node_symbol_sub(struct ast_node* parent_symbol, struct token name) {
    for (int i = 0; i < parent_symbol->children_count; i++)
    {
        struct ast_node* child = parent_symbol->children[i];
        if (child->type != AST_NODE_TYPE_STRUCT)
            continue;
        struct token symbol = (*child->children)->token;
        if (symbol.length == name.length &&
            memcmp(symbol.start, name.start, name.length) == 0)
        {
            return child;
        }
    }
    return NULL;
}