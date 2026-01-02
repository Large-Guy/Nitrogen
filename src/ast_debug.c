#include "ast_debug.h"

#include "ast.h"

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
    [AST_NODE_TYPE_FIELD] = "field",
    [AST_NODE_TYPE_VARIABLE] = "variable",
    [AST_NODE_TYPE_METHOD] = "method",
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
    
    if (depth > 16) {
        printf("{...}\n");
        return;
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

//TODO: because of how symbols are stored this is making some pretty messy debug
void ast_node_debug(FILE* out, struct ast_node* node)
{
    debug(node, 0);
}