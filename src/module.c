#include "module.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct module* module_new(struct token name) {
    struct module* module = malloc(sizeof(struct module));
    assert(module);
    module->name = malloc(name.length + 1);
    memcpy(module->name, name.start, name.length);
    module->name[name.length] = '\0';
    module->root = ast_node_new(AST_NODE_TYPE_MODULE, name);
    module->symbols = ast_node_new(AST_NODE_TYPE_MODULE, name);
    module->lexers = malloc(sizeof(struct lexer*));
    assert(module->lexers);
    module->lexer_count = 0;
    module->lexer_capacity = 1;
    return module;
}

void module_free(struct module* module) {
    free(module->lexers);
    free(module->name);
    ast_node_free(module->root);
    ast_node_free(module->symbols);
    free(module);
}

void module_add_source(struct module* module, struct lexer* lexer) {
    if (module->lexer_count >= module->lexer_capacity) {
        module->lexer_capacity *= 2;
        module->lexers = realloc(module->lexers, module->lexer_capacity * sizeof(struct lexer*));
        assert(module->lexers);
    }

    module->lexers[module->lexer_count++] = lexer;
}

void module_add_symbol(struct module* module, struct ast_node* symbol) {
    ast_node_append_child(module->symbols, symbol);
}

struct ast_node* module_get_symbol(struct ast_node* scope, struct token name) {
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
    return scope->parent ? module_get_symbol(scope->parent, name) : NULL;
}

void module_list_free(struct module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        module_free(list->modules[i]);
    }
    free(list->modules);
}