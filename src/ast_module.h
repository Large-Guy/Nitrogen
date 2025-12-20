#ifndef COMPILER_MODULE_H
#define COMPILER_MODULE_H
#include <stdbool.h>

#include "lexer.h"
#include "ast.h"

struct ast_module {
    char* name;

    struct ast_node* root;
    struct ast_node* definitions;
    
    struct lexer** lexers;
    size_t lexer_count;
    size_t lexer_capacity;
};

struct ast_module* ast_module_new(struct token name);

void ast_module_free(struct ast_module* module);

void ast_module_add_source(struct ast_module* module, struct lexer* lexer);

void ast_module_add_symbol(struct ast_module* module, struct ast_node* symbol);

struct ast_node* ast_module_get_symbol(struct ast_node* scope, struct token name);

struct ast_module_list {
    struct ast_module** modules;
    uint32_t module_count;
    uint32_t module_capacity;
};

struct ast_module_list* ast_module_list_new();

void ast_module_list_add(struct ast_module_list* list, struct ast_module* module);

void ast_module_list_free(struct ast_module_list* list);


#endif //COMPILER_MODULE_H