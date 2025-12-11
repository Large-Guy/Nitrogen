#include "ir_gen.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct local {
    uint8_t reg;
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


bool scope_get_local(struct scope* scope, struct token token, struct local** out) {
    for (size_t i = 0; i < scope->locals_count; i++) {
        struct local* local = &scope->locals[i];
        if (local->name.length == token.length &&
            memcmp(local->name.start, token.start, token.length) == 0) {
            *out = local;
            return true;
        }
    }
    return scope->previous ? scope_get_local(scope->previous, token, out) : false;
}

void scope_update_local(struct scope* scope, struct token token, int new_reg) {
    struct local* loc;
    bool found = scope_get_local(scope, token, &loc);
    if (found) {
        loc->reg = new_reg;
    }
}

#define MAX_STACK 512;

struct compiler {
    struct ir* target;
    uint32_t stack[512];
    int stack_top;
    
    struct scope* scope;

    
};

void push(struct compiler* compiler, uint32_t reg) {
    compiler->stack[++compiler->stack_top] = reg;
}

int pop(struct compiler* compiler) {
    return compiler->stack[compiler->stack_top--];
}

struct compiler* compiler_new() {
    struct compiler* compiler = (struct compiler*)malloc(sizeof(struct compiler));
    compiler->scope = scope_new();
    compiler->stack_top = -1;
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

static struct ir* ir_symbol_new(struct token symbol, enum chunk_type type)
{
    assert(symbol.length != 0);
    char* symbol_name = malloc(symbol.length + 1);
    memcpy(symbol_name, symbol.start, symbol.length);
    symbol_name[symbol.length] = '\0';
    struct ir* ir = ir_new(symbol_name, symbol.start[0] != '_', type);
    free(symbol_name);
    return ir;
}

//NOTE: consider returning a virtual register?
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
            break;
        }
        case AST_NODE_TYPE_INTEGER: {
            int64_t immediate = strtoll(node->token.start, NULL, 10);
            push(compiler, ir_constant(ir, TYPE_I32, immediate));
            break;
        }
        case AST_NODE_TYPE_ASSIGN: {
            struct ast_node* a = node->children[0];
            struct token name = a->token;
            struct ast_node* b = node->children[1];
            struct scope* scope = compiler->scope;
            // a = b
            statement(compiler, module, b);
            
            scope_update_local(scope, name, pop(compiler));
            
            //ir_push(ir, info.id);
            
            break;
        }
        case AST_NODE_TYPE_ADD: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a + b
            statement(compiler, module, a);
            statement(compiler, module, b);
            //pop a and b off the virtual stack
            int right = pop(compiler);
            int left = pop(compiler);
            push(compiler, ir_add(ir, OP_ADD, TYPE_I32, left, right));
            break;
        }
        case AST_NODE_TYPE_SUBTRACT: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a - b
            statement(compiler, module, a);
            statement(compiler, module, b);
            int right = pop(compiler);
            int left = pop(compiler);
            push(compiler, ir_add(ir, OP_SUB, TYPE_I32, left, right));
            break;
        }
        case AST_NODE_TYPE_MULTIPLY: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a * b
            statement(compiler, module, a);
            statement(compiler, module, b);
            int right = pop(compiler);
            int left = pop(compiler);
            push(compiler, ir_add(ir, OP_MUL, TYPE_I32, left, right));
            break;
        }
        case AST_NODE_TYPE_DIVIDE: {
            struct ast_node* a = node->children[0];
            struct ast_node* b = node->children[1];
            // a / b
            statement(compiler, module, a);
            statement(compiler, module, b);
            int right = pop(compiler);
            int left = pop(compiler);
            push(compiler, ir_add(ir, OP_DIV, TYPE_I32, left, right));
            break;
        }
        case AST_NODE_TYPE_RETURN_STATEMENT: {
            int reg = 0;
            if (node->children_count) {
                struct ast_node* a = node->children[0];
                statement(compiler, module, a);
                reg = pop(compiler);
            }
            //TODO: determine type
            ir_add(ir, OP_RETURN, TYPE_I32, reg, 0);
            break;
        }
        case AST_NODE_TYPE_VARIABLE: {
            struct ast_node* name = node->children[0];
            struct ast_node* type = node->children[1];
            struct scope* scope = compiler->scope;
            //semi-optimization, skips ZII
            if (node->children_count > 2) {
                struct ast_node* value = node->children[2];
                statement(compiler, module, value);
                struct local loc = {pop(compiler), name->token};
                scope_add_local(scope, loc);
                break;
            }
            //TODO: symbol table needed
            uint32_t reg = ir_add(ir, OP_CONST, TYPE_I32, 0, 0);
            struct local loc = {reg, name->token};
            scope_add_local(scope, loc);
            break;
        }
        case AST_NODE_TYPE_NAME: {
            struct token name = node->token;
            struct local* info;
            struct scope* scope = compiler->scope;
            //TODO: symbol table needed
            assert(scope_get_local(scope, name, &info));
            push(compiler, info->reg);
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
            struct ir* ir = ir_symbol_new(node->children[0]->token, CHUNK_TYPE_FUNCTION);
            struct ast_node* type = node->children[1]; //type
            struct ast_node* body = node->children[2]; //function body
            struct compiler* compiler = compiler_new();
            
            compiler->target = ir;
            statement(compiler, module, body);
            
            compiler_free(compiler);
            return ir;
        }
        case AST_NODE_TYPE_VARIABLE:
        {
            struct ir* ir = ir_symbol_new(node->children[0]->token, CHUNK_TYPE_VARIABLE);
            struct ast_node* type = node->children[0];
            struct ast_node* value = node->children[1];
            //TODO: implement IR instructions for generating this stuff
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
