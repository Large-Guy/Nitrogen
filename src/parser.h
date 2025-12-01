#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "lexer.h"

enum ast_node_type {
    AST_NODE_BLOCK,
    AST_NODE_TYPE_SEQUENCE,

    //statements
    AST_NODE_TYPE_RETURN_STATEMENT,

    //constants
    AST_NODE_TYPE_INTEGER,
    AST_NODE_TYPE_FLOAT,

    //types
    AST_NODE_TYPE_I8,
    AST_NODE_TYPE_I16,
    AST_NODE_TYPE_I32,
    AST_NODE_TYPE_I64,
    
    AST_NODE_TYPE_U8,
    AST_NODE_TYPE_U16,
    AST_NODE_TYPE_U32,
    AST_NODE_TYPE_U64,

    //declarations
    AST_NODE_TYPE_I8_DECLARE,
    AST_NODE_TYPE_I16_DECLARE,
    AST_NODE_TYPE_I32_DECLARE,
    AST_NODE_TYPE_I64_DECLARE,
    
    AST_NODE_TYPE_U8_DECLARE,
    AST_NODE_TYPE_U16_DECLARE,
    AST_NODE_TYPE_U32_DECLARE,
    AST_NODE_TYPE_U64_DECLARE,

    //operators
    AST_NODE_TYPE_ASSIGN,
    AST_NODE_TYPE_NEGATE,
    AST_NODE_TYPE_INVERSE,
    AST_NODE_TYPE_ADD,
    AST_NODE_TYPE_SUBTRACT,
    AST_NODE_TYPE_MULTIPLY,
    AST_NODE_TYPE_DIVIDE,
    
    AST_NODE_TYPE_EQUAL,
    AST_NODE_TYPE_NOT_EQUAL,
    AST_NODE_TYPE_GREATER_THAN,
    AST_NODE_TYPE_GREATER_THAN_EQUAL,
    AST_NODE_TYPE_LESS_THAN,
    AST_NODE_TYPE_LESS_THAN_EQUAL,

    AST_NODE_TYPE_AND,
    AST_NODE_TYPE_OR,

    //control flow
    AST_NODE_TYPE_IF,
    AST_NODE_TYPE_WHILE,
    AST_NODE_TYPE_DO_WHILE,
};

struct ast_node {
    enum ast_node_type type;
    struct token token;
    struct ast_node *parent;
    struct ast_node** children;
    size_t children_count;
    size_t children_capacity;
};

struct ast_node* ast_node_new(enum ast_node_type type, struct token token);

void ast_node_free(struct ast_node* node);

void ast_node_append_child(struct ast_node* node, struct ast_node* child);

struct ast_node* ast_node_build(struct lexer* lexer);

struct chunk* ast_node_compile_sequence(struct ast_node* node);

void ast_node_debug(struct ast_node* node);

#endif //COMPILER_PARSER_H