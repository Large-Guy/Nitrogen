#include "module.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct module* module_new(struct token name) {
    struct module* module = malloc(sizeof(struct module));
    assert(module);
    module->name = name;
    module->root = ast_node_new(AST_NODE_TYPE_MODULE, name);
    module->lexers = malloc(sizeof(struct lexer*));
    assert(module->lexers);
    module->lexer_count = 0;
    module->lexer_capacity = 1;
    module->symbols = malloc(sizeof(struct ast_node*));
    assert(module->symbols);
    module->symbols_count = 0;
    module->symbols_capacity = 1;
    return module;
}

void module_free(struct module* module) {
    free(module->lexers);
    free(module->symbols);
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
    if (module->symbols_count >= module->symbols_capacity) {
        module->symbols_capacity *= 2;
        module->symbols = realloc(module->symbols, sizeof(struct ast_node*) * module->symbols_capacity);
        assert(module->symbols);
    }
    module->symbols[module->symbols_count++] = symbol;
}

struct ast_node* module_get_symbol(struct module* module, struct token name) {
    for (size_t i = 0; i < module->symbols_count; i++) {
        struct token symbol_name = (*module->symbols[i]->children)->token;
        if (name.length == symbol_name.length &&
            memcmp(name.start, symbol_name.start, symbol_name.length) == 0) {
            return module->symbols[i];
        }
    }
    return NULL;
}

void module_list_free(struct module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        module_free(list->modules[i]);
    }
    free(list->modules);
}