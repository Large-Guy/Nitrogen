#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "lexer.h"
#include "ast_module.h"

enum parser_stage {
    PARSER_STAGE_MODULE_GENERATION,
    PARSER_STAGE_DEPENDENCY_GRAPH,
    PARSER_STAGE_TYPE_DECLARATION,
    PARSER_STAGE_SYMBOL_RESOLUTION_PASS,
    PARSER_STAGE_TREE_GENERATION,
};

struct parser {
    enum parser_stage stage;
    struct ast_module* module;
    struct lexer* lexer;
    struct token current;
    struct token previous;
    uint32_t tp;

    struct ast_node** scope_stack;
    size_t scope_stack_count;
    size_t scope_stack_capacity;

    bool error;
};

struct parser* parser_new(enum parser_stage stage, struct ast_module* module, struct lexer* lexer);

void parser_free(struct parser* parser);

struct ast_node* parser_scope(struct parser* parser);

void parser_push_scope(struct parser* parser, struct ast_node* node);

void parser_pop_scope(struct parser* parser);

void parser_error(struct parser* parser, struct token at, const char* message);

void parser_advance(struct parser* parser);

struct token parser_peek(struct parser* parser, uint32_t offset);

bool parser_type_exists(struct parser* parser, struct token name);

void parser_declare_type(struct parser* parser, struct ast_node* symbol);

void parser_consume(struct parser* parser, enum token_type type, const char* error_message);

bool parser_check(struct parser* parser, enum token_type type);

bool parser_match(struct parser* parser, enum token_type type);

bool parser_match_type(struct parser* parser);


#endif //COMPILER_PARSER_H