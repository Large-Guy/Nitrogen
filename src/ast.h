#ifndef COMPILER_AST_H
#define COMPILER_AST_H
#include <stddef.h>

#include "lexer.h"


enum ast_node_type
{
    //types
    AST_NODE_TYPE_VOID,
    AST_NODE_TYPE_REFERENCE,
    AST_NODE_TYPE_POINTER,
    AST_NODE_TYPE_ARRAY,
    AST_NODE_TYPE_SIMD,

    AST_NODE_TYPE_BOOL,
    
    AST_NODE_TYPE_I8,
    AST_NODE_TYPE_I16,
    AST_NODE_TYPE_I32,
    AST_NODE_TYPE_I64,

    AST_NODE_TYPE_U8,
    AST_NODE_TYPE_U16,
    AST_NODE_TYPE_U32,
    AST_NODE_TYPE_U64,

    AST_NODE_TYPE_F32,
    AST_NODE_TYPE_F64,
    
    AST_NODE_TYPE_TYPE_COUNT, // used for implicit cast table
    
    AST_NODE_TYPE_MODULE,
    AST_NODE_TYPE_SEQUENCE,

    //statements
    AST_NODE_TYPE_MODULE_STATEMENT,
    AST_NODE_TYPE_RETURN_STATEMENT,

    //constants
    AST_NODE_TYPE_INTEGER,
    AST_NODE_TYPE_FLOAT,

    //names
    AST_NODE_TYPE_MODULE_NAME,
    AST_NODE_TYPE_NAME,
    AST_NODE_TYPE_TYPE,

    //declarations OR implementations, depending on context
    AST_NODE_TYPE_VARIABLE,
    AST_NODE_TYPE_FUNCTION,
    AST_NODE_TYPE_STRUCT,
    AST_NODE_TYPE_INTERFACE,

    //operators
    AST_NODE_TYPE_ASSIGN,
    AST_NODE_TYPE_NEGATE,
    AST_NODE_TYPE_ADDRESS,
    AST_NODE_TYPE_NOT,
    AST_NODE_TYPE_ADD,
    AST_NODE_TYPE_SUBTRACT,
    AST_NODE_TYPE_MULTIPLY,
    AST_NODE_TYPE_DIVIDE,
    AST_NODE_TYPE_MODULO,
    AST_NODE_TYPE_POWER,
    AST_NODE_TYPE_BITWISE_AND,
    AST_NODE_TYPE_BITWISE_OR,
    AST_NODE_TYPE_BITWISE_XOR,
    AST_NODE_TYPE_BITWISE_NOT,
    AST_NODE_TYPE_BITWISE_LEFT,
    AST_NODE_TYPE_BITWISE_RIGHT,

    AST_NODE_TYPE_CALL,

    AST_NODE_TYPE_GET_FIELD,
    AST_NODE_TYPE_SET_FIELD,

    AST_NODE_TYPE_EQUAL,
    AST_NODE_TYPE_NOT_EQUAL,
    AST_NODE_TYPE_GREATER_THAN,
    AST_NODE_TYPE_GREATER_THAN_EQUAL,
    AST_NODE_TYPE_LESS_THAN,
    AST_NODE_TYPE_LESS_THAN_EQUAL,

    AST_NODE_TYPE_AND,
    AST_NODE_TYPE_OR,
    
    AST_NODE_TYPE_CAST,

    //control flow
    AST_NODE_TYPE_IF,
    AST_NODE_TYPE_WHILE,
    AST_NODE_TYPE_DO_WHILE,
};

struct ast_node
{
    enum ast_node_type type;
    struct token token;
    struct ast_node* parent;
    struct ast_node** children;
    size_t children_count;
    size_t children_capacity;
};

struct ast_node* ast_node_new(enum ast_node_type type, struct token token);

void ast_node_free(struct ast_node* node);

struct ast_node* ast_node_clone(struct ast_node* node);

void ast_node_append_child(struct ast_node* node, struct ast_node* child);

void ast_node_remove_child(struct ast_node* node, struct ast_node* child);

void ast_node_debug(struct ast_node* node);

size_t get_node_size(struct ast_module* module, struct ast_node* node);

struct ssa_type get_node_type(struct ast_module* module, struct ast_node* node);

#endif //COMPILER_AST_H
