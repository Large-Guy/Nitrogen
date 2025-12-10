#include "ir_gen.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct local {
    uint8_t id;
    struct token name;
};

struct scope {
    struct local* locals;
    size_t locals_count;
    size_t locals_capacity;

    struct scope* previous;
};

struct scope* scope_new() {
    struct scope* scope = malloc(sizeof(struct scope));
    assert(scope);
    scope->locals = malloc(sizeof(struct local));
    assert(scope->locals);
    scope->locals_count = 0;
    scope->locals_capacity = 1;
    scope->previous = NULL;
    return scope;
}

void scope_free(struct scope* scope) {
    free(scope->locals);
    free(scope);
}

void scope_add_local(struct scope* scope, struct local local) {
    if (scope->locals_count >= scope->locals_capacity) {
        scope->locals_capacity *= 2;
        scope->locals = realloc(scope->locals, scope->locals_capacity * sizeof(struct local));
        assert(scope->locals);
    }
    scope->locals[scope->locals_count++] = local;
}

bool scope_get_local(struct scope* scope, struct token token, struct local* out) {
    for (size_t i = 0; i < scope->locals_count; i++) {
        struct local local = scope->locals[i];
        if (local.name.length == token.length &&
            memcmp(local.name.start, token.start, token.length) == 0) {
            *out = local;
            return true;
        }
    }
    return scope->previous ? scope_get_local(scope->previous, token, out) : false;
}

struct compiler {
    struct ir* target;
    
    struct scope* scope;
};

struct compiler* compiler_new() {
    struct compiler* compiler = (struct compiler*)malloc(sizeof(struct compiler));
    compiler->scope = scope_new();
    return compiler;
}

void compiler_free(struct compiler* compiler) {
    free(compiler->scope);
    free(compiler);
}

void begin_scope(struct compiler* compiler) {
    struct scope* new_scope = scope_new();
    new_scope->previous = compiler->scope;
    compiler->scope = new_scope;
}

void end_scope(struct compiler* compiler) {
    struct scope* old_scope = compiler->scope;
    compiler->scope = old_scope->previous;
    free(old_scope);
}

static size_t get_node_size(struct module* module, struct ast_node* node) {
    switch (node->type) {
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
            
        case AST_NODE_TYPE_TYPE: {
            return 0; //unknown type
        }
        default:
            fprintf(stderr, "unexpected node type\n");
            return 0;
    }
}

static struct ir* ir_symbol_new(struct token symbol)
{
    assert(symbol.length != 0);
    char* symbol_name = malloc(symbol.length + 1);
    memcpy(symbol_name, symbol.start, symbol.length);
    symbol_name[symbol.length] = '\0';
    struct ir* ir = ir_new(symbol_name, symbol.start[0] != '_');
    free(symbol_name);
    return ir;
}

static void statement(struct compiler* compiler, struct module* module, struct ast_node* node) {
    struct ir* ir = compiler->target;
    switch (node->type) {
        case AST_NODE_TYPE_SEQUENCE: {
            begin_scope(compiler);
            for (int i = 0; i < node->children_count; i++) {
                struct ast_node* child = node->children[i];
                statement(compiler, module, child);
            }
            end_scope(compiler);
        }
        case AST_NODE_TYPE_INTEGER: {
            ir_push(ir, OP_IMM_32);
            int64_t immediate = strtoll(node->token.start, NULL, 10);
            ir_push32(ir, immediate);
            break;
        }
        case AST_NODE_TYPE_ASSIGN: {
            struct ast_node* a = node->children[0];
            struct token name = a->token;
            struct ast_node* b = node->children[1];
            // a = b
            statement(compiler, module, b);
            ir_push(ir, OP_SET);
            
            struct local info;
            assert(scope_get_local(compiler->scope, name, &info));
            ir_push(ir, info.id);
            
            break;
        }
        case AST_NODE_TYPE_ADD: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a + b
            statement(compiler, module, a);
            statement(compiler, module, b);
            ir_push(ir, OP_IADD);
            break;
        }
        case AST_NODE_TYPE_SUBTRACT: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a - b
            statement(compiler, module, a);
            statement(compiler, module, b);
            ir_push(ir, OP_ISUB);
            break;
        }
        case AST_NODE_TYPE_MULTIPLY: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a * b
            statement(compiler, module, a);
            statement(compiler, module, b);
            ir_push(ir, OP_IMUL);
            break;
        }
        case AST_NODE_TYPE_DIVIDE: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a / b
            statement(compiler, module, a);
            statement(compiler, module, b);
            ir_push(ir, OP_ISDIV);
            break;
        }
        case AST_NODE_TYPE_RETURN_STATEMENT: {
            if (node->children_count) {
                struct ast_node* a = node->children[0];
                statement(compiler, module, a);
            }
            ir_push(ir, OP_RETURN);
            break;
        }
        case AST_NODE_TYPE_VARIABLE: {
            struct ast_node* name = node->children[0];
            struct ast_node* type = node->children[1];
            struct scope* scope = compiler->scope;
            struct local local = {ir_declare(ir, get_node_size(module, type)), name->token};
            scope_add_local(scope, local);
            if (node->children_count > 2) {
                struct ast_node* value = node->children[2];
                statement(compiler, module, value);
                
                ir_push(ir, OP_SET);
                ir_push(ir, local.id);
            }
            break;
        }
        case AST_NODE_TYPE_NAME: {
            struct token name = node->token;
            ir_push(ir, OP_GET);
            struct local info;
            assert(scope_get_local(compiler->scope, name, &info));
            ir_push(ir, info.id);
            break;
        }
        default: {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
        }
    }
}

static struct ir* definition(struct module* module, struct ast_node* node)
{
    switch (node->type)
    {
    case AST_NODE_TYPE_FUNCTION:
        {
            struct ir* ir = ir_symbol_new(node->children[0]->token);
            struct ast_node* type = node->children[1]; //type
            struct ast_node* body = node->children[2]; //function body
            struct compiler* compiler = compiler_new();
            
            compiler->target = ir;
            statement(compiler, module, body);
            
            compiler_free(compiler);
            return ir;
        }
    default:
        {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
            return NULL;
        }
    }
}

struct ir_module* ir_gen_module(struct module* module)
{
    struct ir_module* chunks = ir_module_new(module->name);

    for (int i = 0; i < module->root->children_count; i++)
    {
        ir_module_append(chunks, definition(module, module->root->children[i]));
    }

    return chunks;
}
