#include "ast_module.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct ast_module* ast_module_new(struct token name) {
    struct ast_module* module = malloc(sizeof(struct ast_module));
    assert(module);
    module->name = malloc(name.length + 1);
    memcpy(module->name, name.start, name.length);
    module->name[name.length] = '\0';
    module->root = ast_node_new(AST_NODE_TYPE_MODULE, name);
    module->definitions = ast_node_new(AST_NODE_TYPE_MODULE, name);
    module->lexers = malloc(sizeof(struct lexer*));
    assert(module->lexers);
    module->lexer_count = 0;
    module->lexer_capacity = 1;
    return module;
}

void ast_module_free(struct ast_module* module) {
    free(module->lexers);
    free(module->name);
    ast_node_free(module->root);
    ast_node_free(module->definitions);
    free(module);
}

void ast_module_add_source(struct ast_module* module, struct lexer* lexer) {
    if (module->lexer_count >= module->lexer_capacity) {
        module->lexer_capacity *= 2;
        module->lexers = realloc(module->lexers, module->lexer_capacity * sizeof(struct lexer*));
        assert(module->lexers);
    }

    module->lexers[module->lexer_count++] = lexer;
}

void ast_module_add_symbol(struct ast_module* module, struct ast_node* symbol) {
    ast_node_append_child(module->definitions, symbol);
}

struct ast_node* ast_module_get_symbol(struct ast_node* scope, struct token name) {
    for (size_t i = 0; i < scope->children_count; i++) {
        struct ast_node* child = scope->children[i];
        if (child->type != AST_NODE_TYPE_VARIABLE &&
            child->type != AST_NODE_TYPE_FUNCTION &&
            child->type != AST_NODE_TYPE_INTERFACE &&
            child->type != AST_NODE_TYPE_STRUCT) {
            continue;
        }
        struct token symbol_name = (*child->children)->token;
        if (name.length == symbol_name.length &&
                memcmp(name.start, symbol_name.start, symbol_name.length) == 0) {
            return scope->children[i];
        }
        
    }
    return scope->parent ? ast_module_get_symbol(scope->parent, name) : NULL;
}

struct ast_module_list* ast_module_list_new() {
    struct ast_module_list* list = malloc(sizeof(struct ast_module_list));
    assert(list);
    list->modules = malloc(sizeof(struct ast_module*));
    assert(list->modules);
    list->module_count = 0;
    list->module_capacity = 1;
    return list;
}

void ast_module_list_add(struct ast_module_list* list, struct ast_module* module) {
    if (list->module_count >= list->module_capacity) {
        list->module_capacity *= 2;
        list->modules = realloc(list->modules, list->module_capacity * sizeof(struct ast_module*));
        assert(list->modules);
    }
    list->modules[list->module_count++] = module;
}

void ast_module_list_free(struct ast_module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        ast_module_free(list->modules[i]);
    }
    free(list->modules);
    free(list);
}
