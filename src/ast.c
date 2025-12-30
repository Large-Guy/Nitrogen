#include "ast.h"

#include <assert.h>
#include <stdlib.h>


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

static const char* ast_node_type_names[] = {
    [AST_NODE_TYPE_VOID] = "void",
    [AST_NODE_TYPE_REFERENCE] = "reference",
    [AST_NODE_TYPE_POINTER] = "pointer",
    [AST_NODE_TYPE_ARRAY] = "array",
    [AST_NODE_TYPE_SIMD] = "simd",
    [AST_NODE_TYPE_BOOL] = "bool",
    [AST_NODE_TYPE_I8] = "i8",
    [AST_NODE_TYPE_I16] = "i16",
    [AST_NODE_TYPE_I32] = "i32",
    [AST_NODE_TYPE_I64] = "i64",
    [AST_NODE_TYPE_U8] = "u8",
    [AST_NODE_TYPE_U16] = "u16",
    [AST_NODE_TYPE_U32] = "u32",
    [AST_NODE_TYPE_U64] = "u64",
    [AST_NODE_TYPE_F32] = "f32",
    [AST_NODE_TYPE_F64] = "f64",
    [AST_NODE_TYPE_TYPE_COUNT] = "",
    [AST_NODE_TYPE_TREE] = "ast",
    [AST_NODE_TYPE_SCOPE] = "scope",
    [AST_NODE_TYPE_SEQUENCE] = "sequence",
    [AST_NODE_TYPE_MODULE_STATEMENT] = "module",
    [AST_NODE_TYPE_RETURN_STATEMENT] = "return",
    [AST_NODE_TYPE_INTEGER] = "integer",
    [AST_NODE_TYPE_FLOAT] = "float",
    [AST_NODE_TYPE_MODULE_NAME] = "module-name",
    [AST_NODE_TYPE_NAME] = "name",
    [AST_NODE_TYPE_TYPE] = "type",
    [AST_NODE_TYPE_VARIABLE] = "variable",
    [AST_NODE_TYPE_FUNCTION] = "function",
    [AST_NODE_TYPE_STRUCT] = "struct",
    [AST_NODE_TYPE_INTERFACE] = "interface",
    [AST_NODE_TYPE_ASSIGN] = "assign",
    [AST_NODE_TYPE_NEGATE] = "negate",
    [AST_NODE_TYPE_ADDRESS] = "address",
    [AST_NODE_TYPE_LOCK] = "lock",
    [AST_NODE_TYPE_NOT] = "not",
    [AST_NODE_TYPE_ADD] = "add",
    [AST_NODE_TYPE_SUBTRACT] = "sub",
    [AST_NODE_TYPE_MULTIPLY] = "mul",
    [AST_NODE_TYPE_DIVIDE] = "div",
    [AST_NODE_TYPE_MODULO] = "mod",
    [AST_NODE_TYPE_POWER] = "pow",
    [AST_NODE_TYPE_BITWISE_AND] = "bitwise-and",
    [AST_NODE_TYPE_BITWISE_OR] = "bitwise-or",
    [AST_NODE_TYPE_BITWISE_XOR] = "bitwise-xor",
    [AST_NODE_TYPE_BITWISE_NOT] = "bitwise-not",
    [AST_NODE_TYPE_BITWISE_LEFT] = "bitwise-left",
    [AST_NODE_TYPE_BITWISE_RIGHT] = "bitwise-right",
    [AST_NODE_TYPE_CALL] = "call",
    [AST_NODE_TYPE_GET_FIELD] = "get",
    [AST_NODE_TYPE_SET_FIELD] = "set",
    [AST_NODE_TYPE_EQUAL] = "equal",
    [AST_NODE_TYPE_NOT_EQUAL] = "not-equal",
    [AST_NODE_TYPE_GREATER_THAN] = "greater-than",
    [AST_NODE_TYPE_GREATER_THAN_EQUAL] = "greater-equal",
    [AST_NODE_TYPE_LESS_THAN] = "less-than",
    [AST_NODE_TYPE_LESS_THAN_EQUAL] = "less-equal",
    [AST_NODE_TYPE_AND] = "and",
    [AST_NODE_TYPE_OR] = "or",
    [AST_NODE_TYPE_STATIC_CAST] = "static-cast",
    [AST_NODE_TYPE_REINTERPRET_CAST] = "reinterpret-cast",
    [AST_NODE_TYPE_IF] = "if",
    [AST_NODE_TYPE_WHILE] = "while",
    [AST_NODE_TYPE_DO_WHILE] = "do-while",
};

const char* ast_node_get_name(struct ast_node* node) {
    return ast_node_type_names[node->type];
}


static void debug(struct ast_node* node, int32_t depth)
{
    // Generate a color based on the type number
    const char* colors[] = {
        "\033[31m", // red
        "\033[32m", // green
        "\033[33m", // yellow
        "\033[34m", // blue
        "\033[35m", // magenta
        "\033[36m", // cyan
        "\033[37m", // white
    };

    for (int i = 0; i < depth; i++)
    {
        printf("%s| ", colors[i % (sizeof(colors) / sizeof(colors[0]))]);
    }

    int color_index = node->type % (sizeof(colors) / sizeof(colors[0]));

    // Print node with type-colored bracket
    printf("%snode\033[0m: [%s] %s %.*s\n",
           "\033[1;37m", // "node" in bright white
           ast_node_get_name(node),
           colors[color_index], // color based on type
           (int32_t)node->token.length,
           node->token.start);

    for (int i = 0; i < node->children_count; i++)
    {
        debug(node->children[i], depth + 1);
    }
}

void ast_node_debug(FILE* out, struct ast_node* node)
{
    debug(node, 0);
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

size_t get_node_size(struct ast_module* module, struct ast_node* node)
{
    switch (node->type)
    {
        case AST_NODE_TYPE_BOOL: return 1;
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

        case AST_NODE_TYPE_SIMD: return get_node_size(module, node->children[0]) * to_power_of_two(strtol(
                node->children[1]->token.start, NULL, 10));
        default:
            fprintf(stderr, "expected a built-in type node\n");
            return 0;
    }
}

struct ssa_type get_node_type(struct ast_module* module, struct ast_node* node)
{
    return (struct ssa_type){get_node_size(module, node), module, node};
}
