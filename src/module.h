#ifndef COMPILER_MODULE_H
#define COMPILER_MODULE_H
#include <stdbool.h>

#include "lexer.h"
#include "ast.h"

enum symbol_type {
    SYMBOL_TYPE_STRUCT,
    SYMBOL_TYPE_UNION,
    SYMBOL_TYPE_INTERFACE,
    SYMBOL_TYPE_FUNCTION,
    SYMBOL_TYPE_VARIABLE
};

struct module {
    struct token name;

    struct ast_node* root;
    
    struct lexer** lexers;
    size_t lexer_count;
    size_t lexer_capacity;
    
    struct ast_node** symbols;
    size_t symbols_count;
    size_t symbols_capacity;
};

struct module* module_new(struct token name);

void module_free(struct module* module);

void module_add_source(struct module* module, struct lexer* lexer);

void module_add_symbol(struct module* module, struct ast_node* symbol);

struct ast_node* module_get_symbol(struct module* module, struct token name);

struct module_list {
    struct module** modules;
    uint32_t module_count;
};

void module_list_free(struct module_list* list);


#endif //COMPILER_MODULE_H