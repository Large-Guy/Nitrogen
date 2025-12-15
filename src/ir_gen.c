#include "ir_gen.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"

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

struct compiler {
    struct ir* ir;

    uint32_t* stack;
    uint32_t stack_count;
    uint32_t stack_capacity;
    
    struct register_table* regs;
    struct block* block;
};

static struct compiler* compiler_new(struct ir* ir) {
    struct compiler* compiler = malloc(sizeof(struct compiler));
    compiler->regs = register_table_new();
    compiler->ir = ir;
    compiler->block = block_new(true, compiler->regs);

    compiler->stack = malloc(sizeof(uint32_t));
    compiler->stack_count = 0;
    compiler->stack_capacity = 1;
    
    return compiler;
}

static void compiler_free(struct compiler* compiler) {
    register_table_free(compiler->regs);
    free(compiler);
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

static struct operand statement(struct compiler* compiler, struct module* module, struct ast_node* node);

static struct operand binary(struct compiler* compiler, struct module* module, struct ast_node* node, enum ssa_instruction_code type) {
    struct ast_node* left = node->children[0];
    struct ast_node* right = node->children[1];
    struct ssa_instruction instruction = {};
    instruction.operator = type;
    instruction.type = TYPE_I32;
    instruction.operands[0] = statement(compiler, module, left);
    instruction.operands[1] = statement(compiler, module, right);
    instruction.result = operand_reg(register_table_alloc(compiler->regs));
    block_add(compiler->block, instruction);
    return instruction.result;
}

static struct operand unary(struct compiler* compiler, struct module* module, struct ast_node* node, enum ssa_instruction_code type) {
    struct ast_node* x = node->children[0];
    struct ssa_instruction instruction = {};
    instruction.operator = type;
    instruction.type = TYPE_I32;
    instruction.operands[0] = statement(compiler, module, x);
    instruction.result = operand_reg(register_table_alloc(compiler->regs));
    block_add(compiler->block, instruction);
    return instruction.result;
}

//TODO: everything is currently an i32 for simplicity, implement types properly.
static struct operand statement(struct compiler* compiler, struct module* module, struct ast_node* node) {
    struct block* current = compiler->block;
    struct register_table* regs = compiler->regs;
    switch (node->type) {
        case AST_NODE_TYPE_SEQUENCE: {
            struct operand last_operand = {};
            for (int i = 0; i < node->children_count; i++) {
                struct ast_node* child = node->children[i];
                last_operand = statement(compiler, module, child);
                if (last_operand.type == OPERAND_TYPE_END)
                    break;
            }
            return last_operand;
        }
        case AST_NODE_TYPE_IF: {
            struct ast_node* condition = node->children[0];
            struct ast_node* then = node->children[1];
            
            struct ast_node* else_node = node->children_count > 2 ? node->children[2] : NULL;

            struct block* pass_block = block_new(true, regs);

            struct ssa_instruction pass = {};
            pass.operator = OP_GOTO;
            pass.type = TYPE_VOID;
            pass.operands[0] = operand_block(pass_block);
            
            struct ssa_instruction instruction = {};
            //instruction.result = operand_branch();
            instruction.operator = OP_IF;
            instruction.type = TYPE_I32;
            instruction.operands[0] = statement(compiler, module, condition);
            
            compiler->block = block_new(false, regs);
            ir_add(compiler->ir, compiler->block);
            block_link(current, compiler->block);
            instruction.operands[1] = operand_block(compiler->block);
            struct operand operand = statement(compiler, module, then);

            if (operand.type != OPERAND_TYPE_END) {
                block_add(compiler->block, pass);
                block_link(compiler->block, pass_block);
            }
            
            
            if (else_node) {
                compiler->block = block_new(false, regs);
                ir_add(compiler->ir, compiler->block);
                block_link(current, compiler->block);
                instruction.operands[2] = operand_block(compiler->block);
                operand = statement(compiler, module, else_node);
                if (operand.type != OPERAND_TYPE_END) {
                    block_add(compiler->block, pass);
                    block_link(compiler->block, pass_block);
                }
            }
            else {
                instruction.operands[2] = operand_block(pass_block);
                block_link(current, pass_block);
            }

            block_add(current, instruction);

            if (pass_block->parents_count > 0) {
                compiler->block = pass_block;
                ir_add(compiler->ir, pass_block);
            }
            else {
                block_free(pass_block);
                compiler->block = NULL;
                return operand_end();
            }
            
            return instruction.result;
        }
        case AST_NODE_TYPE_INTEGER: {
            int64_t immediate = strtoll(node->token.start, NULL, 10);
            return operand_const(immediate);
        }
        case AST_NODE_TYPE_ADD: {
            return binary(compiler, module, node, OP_ADD);
        }
        case AST_NODE_TYPE_SUBTRACT: {
            return binary(compiler, module, node, OP_SUB);
        }
        case AST_NODE_TYPE_MULTIPLY: {
            return binary(compiler, module, node, OP_MUL);
        }
        case AST_NODE_TYPE_DIVIDE: {
            return binary(compiler, module, node, OP_DIV);
        }
        case AST_NODE_TYPE_LESS_THAN: {
            return binary(compiler, module, node, OP_LESS);
        }
        case AST_NODE_TYPE_LESS_THAN_EQUAL: {
            return binary(compiler, module, node, OP_LESS_EQUAL);
        }
        case AST_NODE_TYPE_GREATER_THAN: {
            return binary(compiler, module, node, OP_GREATER);
        }
        case AST_NODE_TYPE_GREATER_THAN_EQUAL: {
            return binary(compiler, module, node, OP_GREATER_EQUAL);
        }
        case AST_NODE_TYPE_EQUAL: {
            return binary(compiler, module, node, OP_EQUAL);
        }
        case AST_NODE_TYPE_NOT_EQUAL: {
            return binary(compiler, module, node, OP_NOT_EQUAL);
        }
        case AST_NODE_TYPE_BITWISE_AND: {
            return binary(compiler, module, node, OP_BITWISE_AND);
        }
        case AST_NODE_TYPE_BITWISE_OR: {
            return binary(compiler, module, node, OP_BITWISE_OR);
        }
        case AST_NODE_TYPE_BITWISE_XOR: {
            return binary(compiler, module, node, OP_BITWISE_XOR);
        }
        case AST_NODE_TYPE_BITWISE_NOT: {
            return binary(compiler, module, node, OP_BITWISE_NOT);
        }
        case AST_NODE_TYPE_BITWISE_LEFT: {
            return binary(compiler, module, node, OP_BITWISE_LEFT);
        }
        case AST_NODE_TYPE_BITWISE_RIGHT: {
            return binary(compiler, module, node, OP_BITWISE_RIGHT);
        }
        case AST_NODE_TYPE_AND: {
            return binary(compiler, module, node, OP_AND);
        }
        case AST_NODE_TYPE_OR: {
            return binary(compiler, module, node, OP_OR);
        }
        case AST_NODE_TYPE_NEGATE: {
            return unary(compiler, module, node, OP_NEGATE);
        }
        case AST_NODE_TYPE_NOT: {
            return unary(compiler, module, node, OP_NOT);
        }
        case AST_NODE_TYPE_VARIABLE: {
            struct ast_node* name = node->children[0];
            struct ssa_instruction instruction = {};
            instruction.operator = OP_CONST;
            instruction.type = TYPE_I32;
            instruction.operands[0] = operand_const(0);

            if (node->children_count > 2) {
                instruction.operands[0] = statement(compiler, module, node->children[2]);
            }

            instruction.result = operand_reg(register_table_add(current->symbol_table, name->token, TYPE_I32)->v_reg); //NOTE: assumed i32
            
            block_add(current, instruction);
            return instruction.result;
        }
        case AST_NODE_TYPE_ASSIGN: {
            struct ast_node* target = node->children[0];
            struct ast_node* value = node->children[1];
            struct ssa_instruction instruction = {};
            instruction.operator = OP_CONST;
            instruction.type = TYPE_I32;
            struct symbol* symbol = register_table_lookup(current->symbol_table, target->token);
            
            instruction.result = operand_reg(symbol->v_reg);
            instruction.operands[0] = statement(compiler, module, value);
            block_add(current, instruction);
            return instruction.result;
        }
        case AST_NODE_TYPE_NAME: {
            return operand_reg(register_table_lookup(current->symbol_table, node->token)->v_reg);
        }
        case AST_NODE_TYPE_RETURN_STATEMENT: {
            struct ssa_instruction instruction = {};
            instruction.operator = OP_RETURN;
            instruction.result = operand_end();
            instruction.type = TYPE_I32;

            if (node->children_count > 0) {
                struct ast_node* value = node->children[0];
                instruction.operands[0] = statement(compiler, module, value);
            }
            
            block_add(current, instruction);
            return instruction.result;
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
            struct compiler* compiler = compiler_new(ir);

            ir_add(ir, compiler->block);
            
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
