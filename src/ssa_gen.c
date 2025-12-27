#include "ssa_gen.h"

#include <assert.h>
#include <float.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"
#include "parser.h"

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
    struct ast_module* ast_module;
    struct unit_module* ir_module;

    struct unit* ir;

    uint32_t* stack;
    uint32_t stack_count;
    uint32_t stack_capacity;

    struct register_table* regs;

    struct ssa_type return_type;
    struct operand return_value_ptr;
    //local variables
    struct block* entry;
    //body of the function
    struct block* body;
    //clean up
    struct block* exit;
};

static struct compiler* compiler_new(struct ast_module* ast_module, struct unit_module* ir_module, struct unit* ir,
                                     struct ssa_type return_type) {
    struct compiler* compiler = malloc(sizeof(struct compiler));
    assert(compiler);
    compiler->ast_module = ast_module;
    compiler->ir_module = ir_module;
    compiler->regs = register_table_new();
    assert(compiler->regs);
    compiler->ir = ir;
    compiler->return_type = return_type;
    compiler->entry = block_new(true, compiler->regs);
    unit_add(compiler->ir, compiler->entry);
    compiler->exit = block_new(false, compiler->regs);
    compiler->body = block_new(false, compiler->regs);
    block_link(compiler->entry, compiler->body);

    compiler->return_value_ptr = operand_none();

    unit_add(compiler->ir, compiler->body);

    compiler->stack = malloc(sizeof(uint32_t));
    compiler->stack_count = 0;
    compiler->stack_capacity = 1;

    return compiler;
}

static void compiler_begin(struct compiler* compiler) {
    if (compiler->return_type.type->type != AST_NODE_TYPE_VOID) {
        compiler->return_value_ptr = register_table_add(compiler->regs, (struct token){}, compiler->return_type)->
                pointer;
        struct ssa_instruction ret_val_variable = {};
        ret_val_variable.result = compiler->return_value_ptr;
        ret_val_variable.operator = OP_ALLOC;
        ret_val_variable.type = compiler->return_type;
        block_add(compiler->entry, ret_val_variable);

        //default it to zero
        //TODO: figure out proper ZII
        /*
        struct ssa_instruction ret_val_value = {};
        ret_val_value.operator = OP_STORE;
        ret_val_value.type = compiler->return_type;
        ret_val_value.operands[0] = compiler->return_value_ptr;
        ret_val_value.operands[1] = operand_const_int(0);
        block_add(compiler->body, ret_val_value);
        */
    }
}

static void compiler_end(struct compiler* compiler) {
    struct operand return_op = operand_none();

    if (compiler->return_type.type->type != AST_NODE_TYPE_VOID) {
        struct ssa_instruction load_ret_val = {};
        load_ret_val.operator = OP_LOAD;
        load_ret_val.type = compiler->return_type;
        load_ret_val.operands[0] = compiler->return_value_ptr;
        load_ret_val.result = register_table_alloc(compiler->regs, compiler->ir->return_type);
        block_add(compiler->exit, load_ret_val);

        return_op = load_ret_val.result;
    }

    struct ssa_instruction ret = {};
    ret.result = operand_end();
    ret.operator = OP_RETURN;
    ret.type = compiler->return_type;
    ret.operands[0] = return_op;
    block_add(compiler->exit, ret);

    unit_add(compiler->ir, compiler->exit);
}

static void compiler_free(struct compiler* compiler) {
    register_table_free(compiler->regs);
    free(compiler);
}

static struct operand statement(struct compiler* compiler, struct ast_node* node);

//casting rules

static struct operand cast_emit_reinterpret(struct compiler* compiler, struct operand operand, struct ssa_type type);

static struct operand cast_emit_static(struct compiler* compiler, struct operand operand, struct ssa_type type);

typedef struct operand (* cast_emit_fn)(struct compiler* compiler, struct operand operand, struct ssa_type);

enum cast_type {
    CAST_TYPE_INVALID, // cannot do this
    CAST_TYPE_IMPLICIT, // automatic
    CAST_TYPE_EXPLICIT, // T(value)
    CAST_TYPE_UNSAFE, // T!(value)
};

struct cast_rule {
    enum cast_type type;
    cast_emit_fn fn;
};

static struct cast_rule cast_rules[AST_NODE_TYPE_TYPE_COUNT][AST_NODE_TYPE_TYPE_COUNT] = {
    [AST_NODE_TYPE_VOID] = {},
    [AST_NODE_TYPE_REFERENCE] = {
        [AST_NODE_TYPE_REFERENCE] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_POINTER] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}
    },
    [AST_NODE_TYPE_POINTER] = {
        [AST_NODE_TYPE_BOOL] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret},
        [AST_NODE_TYPE_POINTER] = {CAST_TYPE_UNSAFE, cast_emit_reinterpret}
    },
    [AST_NODE_TYPE_ARRAY] = {}, //consider allowing explicit array casts?
    [AST_NODE_TYPE_SIMD] = {
        [AST_NODE_TYPE_SIMD] = {CAST_TYPE_EXPLICIT, cast_emit_static}
    },
    [AST_NODE_TYPE_BOOL] = {
        [AST_NODE_TYPE_BOOL] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_I8] = {
        [AST_NODE_TYPE_I8] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_I16] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_I16] = {
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_I32] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_I32] = {
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_I64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_I64] = {
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },

    [AST_NODE_TYPE_U8] = {
        [AST_NODE_TYPE_U8] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_U16] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_U16] = {
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_U32] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_U32] = {
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_U64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_U64] = {
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_F32] = {
        [AST_NODE_TYPE_F32] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_F64] = {CAST_TYPE_IMPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},

        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    },
    [AST_NODE_TYPE_F64] = {
        [AST_NODE_TYPE_F32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_F64] = {CAST_TYPE_IMPLICIT, cast_emit_reinterpret}, // aka no change
        [AST_NODE_TYPE_U8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_U64] = {CAST_TYPE_EXPLICIT, cast_emit_static},

        [AST_NODE_TYPE_I8] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I16] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I32] = {CAST_TYPE_EXPLICIT, cast_emit_static},
        [AST_NODE_TYPE_I64] = {CAST_TYPE_EXPLICIT, cast_emit_static},
    }
};

enum ast_node_type get_root_type(struct ast_node* node) {
    return node->type;
}

// implementations

static struct operand cast_emit_reinterpret(struct compiler* compiler, struct operand operand, struct ssa_type type) {
    operand.typename.type = ast_node_clone(type.type);
    return operand;
}

static struct operand cast_emit_static(struct compiler* compiler, struct operand operand, struct ssa_type type) {
    struct ssa_instruction instruction = {};
    instruction.type = type;
    instruction.operator = OP_CAST;
    instruction.operands[0] = operand;
    instruction.result = register_table_alloc(compiler->regs, type);
    block_add(compiler->body, instruction);
    return instruction.result;
}

static struct operand cast(struct compiler* compiler, struct operand operand, struct ssa_type type,
                           enum cast_type mode) {
    enum ast_node_type t = get_root_type(operand.typename.type);

    struct cast_rule rule = cast_rules[t][get_root_type(type.type)];
    if (rule.type == mode) {
        return rule.fn(compiler, operand, type);
    }

    assert(false); //TODO: more robust solution needed

    return operand_none();
}

static struct operand binary(struct compiler* compiler, struct ast_node* node, enum ssa_instruction_code type) {
    struct ast_node* left = node->children[0];
    struct ast_node* right = node->children[1];
    struct ssa_instruction instruction = {};
    instruction.operator = type;
    instruction.operands[0] = statement(compiler, left);
    instruction.operands[1] = cast(compiler, statement(compiler, right), instruction.operands[0].typename,
                                   CAST_TYPE_IMPLICIT);
    instruction.type = instruction.operands[0].typename;
    instruction.result = register_table_alloc(compiler->regs, instruction.type);
    block_add(compiler->body, instruction);
    return instruction.result;
}

static struct operand unary(struct compiler* compiler, struct ast_node* node, enum ssa_instruction_code type) {
    struct ast_node* x = node->children[0];
    struct ssa_instruction instruction = {};
    instruction.operator = type;
    instruction.operands[0] = statement(compiler, x);
    instruction.type = instruction.operands[0].typename;
    instruction.result = register_table_alloc(compiler->regs, instruction.type);
    block_add(compiler->body, instruction);
    return instruction.result;
}

struct operand get_int(int64_t value) {
    if (value >= INT8_MIN && value <= INT8_MAX) {
        return operand_const_i8((int8_t) value);
    }
    if (value >= INT16_MIN && value <= INT16_MAX) {
        return operand_const_i16((int16_t) value);
    }
    if (value >= INT32_MIN && value <= INT32_MAX) {
        return operand_const_i32((int32_t) value);
    }
    return operand_const_i64(value);
}

struct operand get_float(double value) {
    if (value >= FLT_MIN && value <= FLT_MAX) {
        return operand_const_f32((float) value);
    }
    return operand_const_f64(value);
}

static struct operand statement(struct compiler* compiler, struct ast_node* node) {
    struct block* current = compiler->body;
    struct register_table* regs = compiler->regs;
    switch (node->type) {
        case AST_NODE_TYPE_SEQUENCE: {
            struct operand last_operand = {};
            for (int i = 0; i < node->children_count; i++) {
                struct ast_node* child = node->children[i];
                last_operand = statement(compiler, child);
                if (last_operand.type == OPERAND_TYPE_END)
                    break;
            }
            return last_operand;
        }
        case AST_NODE_TYPE_INTEGER: {
            int64_t immediate = strtoll(node->token.start, NULL, 10);
            return get_int(immediate);
        }
        case AST_NODE_TYPE_FLOAT: {
            double immediate = strtod(node->token.start, NULL);
            return get_float(immediate);
        }
        case AST_NODE_TYPE_ADD: {
            return binary(compiler, node, OP_ADD);
        }
        case AST_NODE_TYPE_SUBTRACT: {
            return binary(compiler, node, OP_SUB);
        }
        case AST_NODE_TYPE_MULTIPLY: {
            return binary(compiler, node, OP_MUL);
        }
        case AST_NODE_TYPE_DIVIDE: {
            return binary(compiler, node, OP_DIV);
        }
        case AST_NODE_TYPE_LESS_THAN: {
            return binary(compiler, node, OP_LESS);
        }
        case AST_NODE_TYPE_LESS_THAN_EQUAL: {
            return binary(compiler, node, OP_LESS_EQUAL);
        }
        case AST_NODE_TYPE_GREATER_THAN: {
            return binary(compiler, node, OP_GREATER);
        }
        case AST_NODE_TYPE_GREATER_THAN_EQUAL: {
            return binary(compiler, node, OP_GREATER_EQUAL);
        }
        case AST_NODE_TYPE_EQUAL: {
            return binary(compiler, node, OP_EQUAL);
        }
        case AST_NODE_TYPE_NOT_EQUAL: {
            return binary(compiler, node, OP_NOT_EQUAL);
        }
        case AST_NODE_TYPE_BITWISE_AND: {
            return binary(compiler, node, OP_BITWISE_AND);
        }
        case AST_NODE_TYPE_BITWISE_OR: {
            return binary(compiler, node, OP_BITWISE_OR);
        }
        case AST_NODE_TYPE_BITWISE_XOR: {
            return binary(compiler, node, OP_BITWISE_XOR);
        }
        case AST_NODE_TYPE_BITWISE_NOT: {
            return binary(compiler, node, OP_BITWISE_NOT);
        }
        case AST_NODE_TYPE_BITWISE_LEFT: {
            return binary(compiler, node, OP_BITWISE_LEFT);
        }
        case AST_NODE_TYPE_BITWISE_RIGHT: {
            return binary(compiler, node, OP_BITWISE_RIGHT);
        }
        case AST_NODE_TYPE_AND: {
            return binary(compiler, node, OP_AND);
        }
        case AST_NODE_TYPE_OR: {
            return binary(compiler, node, OP_OR);
        }
        case AST_NODE_TYPE_NEGATE: {
            return unary(compiler, node, OP_NEGATE);
        }
        case AST_NODE_TYPE_NOT: {
            return unary(compiler, node, OP_NOT);
        }
        case AST_NODE_TYPE_STATIC_CAST: {
            struct ast_node* cast_type = node->children[0];
            struct ast_node* value = node->children[1];
            struct operand x = statement(compiler, value);
            return cast(compiler, x, get_node_type(compiler->ast_module, cast_type), CAST_TYPE_EXPLICIT);
        }
        case AST_NODE_TYPE_ADDRESS: {
            struct ast_node* x = node->children[0];
            struct variable* var = register_table_lookup(regs, x->token);
            if (var == NULL) {
                fprintf(stderr, "cannot reference a temporary value\n");
                return operand_none();
            }
            return var->pointer;
        }
        case AST_NODE_TYPE_VARIABLE: {
            struct ast_node* name = node->children[0];
            struct ast_node* type_node = node->children[1];
            struct ssa_type type = get_node_type(compiler->ast_module, type_node);
            struct ast_node* value = node->children_count > 2 ? node->children[2] : NULL;
            struct ssa_instruction instruction = {};
            instruction.operator = OP_ALLOC;
            instruction.type = type;

            instruction.result = register_table_add(current->symbol_table, name->token, type)->pointer;

            //node size
            instruction.operands[0] = operand_const_i64(type.size);

            block_add(compiler->entry, instruction);

            //ZII
            struct ssa_instruction store = {};
            store.operator = OP_STORE;
            store.operands[0] = instruction.result;

            //store the value
            if (value) {
                store.operands[1] = cast(compiler, statement(compiler, value), type, CAST_TYPE_IMPLICIT);
            } else {
                //TODO: allow lazy assignment
                if (type.type->type == AST_NODE_TYPE_REFERENCE) {
                    fprintf(stderr, "references MUST be assigned\n");
                    return operand_none();
                }
                store.operands[1] = operand_const_i64(0);
            }

            block_add(current, store);

            return instruction.result;
        }
        case AST_NODE_TYPE_ASSIGN: {
            struct ast_node* target = node->children[0];
            struct ast_node* value = node->children[1];
            struct ssa_instruction instruction = {};
            instruction.operator = OP_STORE;
            struct variable* symbol = register_table_lookup(current->symbol_table, target->token);

            instruction.type = symbol->pointer.typename;
            instruction.result = operand_none();
            instruction.operands[0] = symbol->pointer;
            instruction.operands[1] = cast(compiler, statement(compiler, value), instruction.type, CAST_TYPE_IMPLICIT);
            block_add(current, instruction);
            return instruction.result;
        }
        case AST_NODE_TYPE_NAME: {
            struct ssa_instruction instruction = {};
            instruction.operator = OP_LOAD;
            struct variable* var = register_table_lookup(current->symbol_table, node->token);
            instruction.type = var->type;
            instruction.operands[0] = var->pointer;
            instruction.result = register_table_alloc(current->symbol_table,
                                                      get_node_type(compiler->ast_module, *var->type.type->children));
            block_add(current, instruction);

            return instruction.result;
        }
        case AST_NODE_TYPE_CALL: {
            struct ast_node* name = node->children[0];
            struct ssa_instruction instruction = {};
            instruction.operator = OP_CALL;
            struct unit* call = unit_module_find(compiler->ir_module, name->token);
            assert(call);
            instruction.type = call->return_type;
            instruction.operands[0] = operand_ir(call);

            for (int i = 0; i < call->argument_count; i++) {
                struct operand arg = statement(compiler, node->children[i + 1]);
                instruction.operands[i + 1] = cast(compiler, arg,
                                                   call->arguments[i].typename, CAST_TYPE_IMPLICIT);
            }

            instruction.result = register_table_alloc(current->symbol_table, instruction.type);

            block_add(current, instruction);

            return instruction.result;
        }
        case AST_NODE_TYPE_RETURN_STATEMENT: {
            if (node->children_count) {
                struct ssa_instruction return_store = {};
                return_store.operator = OP_STORE;
                return_store.operands[0] = compiler->return_value_ptr;
                return_store.operands[1] = cast(compiler, statement(compiler, node->children[0]),
                                                compiler->return_type, CAST_TYPE_IMPLICIT);
                block_add(current, return_store);
            }

            struct ssa_instruction instruction = {};
            instruction.operator = OP_GOTO;
            instruction.result = operand_end();
            instruction.operands[0] = operand_block(compiler->exit);

            block_link(current, compiler->exit);

            block_add(current, instruction);
            return instruction.result;
        }
        case AST_NODE_TYPE_IF: {
            struct ast_node* condition = node->children[0];
            struct ssa_instruction instruction = {};

            struct block* after = block_new(false, compiler->regs);

            instruction.operator = OP_IF;
            instruction.result = operand_end();
            instruction.operands[0] = statement(compiler, condition);
            instruction.operands[2] = operand_block(after);

            struct ast_node* then = node->children[1];
            struct block* then_block = block_new(false, compiler->regs);
            instruction.operands[1] = operand_block(then_block);
            unit_add(compiler->ir, then_block);

            block_link(current, then_block);

            compiler->body = then_block;

            struct operand result = statement(compiler, then);

            if (result.type != OPERAND_TYPE_END) {
                struct ssa_instruction end = {};
                end.operator = OP_GOTO;
                end.result = operand_end();
                end.operands[0] = operand_block(after);
                block_add(compiler->body, end);
                block_link(compiler->body, after);
            }

            if (node->children_count > 2) {
                struct block* else_block = block_new(false, compiler->regs);
                unit_add(compiler->ir, else_block);
                block_link(current, else_block);
                struct ast_node* else_node = node->children[2];
                instruction.operands[2] = operand_block(else_block);
                compiler->body = else_block;
                result = statement(compiler, else_node);
                if (result.type != OPERAND_TYPE_END) {
                    struct ssa_instruction goto_instruction = {};
                    goto_instruction.operator = OP_GOTO;
                    goto_instruction.result = operand_end();
                    goto_instruction.operands[0] = operand_block(after);
                    block_add(compiler->body, goto_instruction);
                    block_link(compiler->body, after);
                }
            } else {
                block_link(current, after);
            }

            block_add(current, instruction);

            unit_add(compiler->ir, after);
            compiler->body = after;

            return operand_none();
        }
        case AST_NODE_TYPE_WHILE: {
            struct ast_node* condition = node->children[0];
            struct ast_node* body = node->children[1];
            struct block* body_block = block_new(false, compiler->regs);
            struct block* loop_block = block_new(false, compiler->regs);
            struct block* after_block = block_new(false, compiler->regs);

            unit_add(compiler->ir, body_block);
            unit_add(compiler->ir, loop_block);
            unit_add(compiler->ir, after_block);

            //first we jump to the loop block
            struct ssa_instruction jump = {};
            jump.result = operand_end();
            jump.operator = OP_GOTO;
            jump.operands[0] = operand_block(loop_block);
            block_add(current, jump);

            //link body to the loop
            block_link(current, loop_block);

            //build the loop block conditions
            compiler->body = loop_block;
            struct ssa_instruction instruction = {};
            instruction.operator = OP_IF;
            instruction.result = operand_end();
            instruction.operands[0] = statement(compiler, condition);
            instruction.operands[1] = operand_block(body_block);
            instruction.operands[2] = operand_block(after_block);
            block_add(loop_block, instruction);

            //link loop to body and after
            block_link(loop_block, body_block);
            block_link(loop_block, after_block);

            //build the body of the loop
            compiler->body = body_block;
            struct operand result = statement(compiler, body);
            if (result.type != OPERAND_TYPE_END) {
                block_link(body_block, loop_block);
                block_add(body_block, jump);
            }

            compiler->body = after_block;
            return operand_none();
        }
        default: {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
            return operand_none();
        }
    }
}

static struct operand argument(struct compiler* compiler, struct ast_node* node) {
    switch (node->type) {
        case AST_NODE_TYPE_SEQUENCE: {
            for (int i = 0; i < node->children_count; i++) {
                struct ast_node* child = node->children[i];
                argument(compiler, child);
            }
            return operand_none();
        }
        case AST_NODE_TYPE_VARIABLE: {
            struct ast_node* name = node->children[0];
            struct ast_node* type = node->children[1];

            //this is special and does not get alloc
            struct operand variable = register_table_alloc(compiler->regs,
                                                           get_node_type(compiler->ast_module, type));

            unit_arg(compiler->ir, variable);

            //make a local copy pointer to a variable
            struct ssa_instruction instruction = {};
            instruction.operator = OP_ALLOC;
            instruction.type = variable.typename;
            instruction.result = register_table_add(compiler->regs, name->token, variable.typename)->pointer;
            instruction.operands[0] = operand_const_i64(variable.typename.size);

            block_add(compiler->entry, instruction);

            struct ssa_instruction store = {};
            store.operator = OP_STORE;
            //location
            store.operands[0] = instruction.result;
            store.operands[1] = variable;
            block_add(compiler->body, store);

            return operand_none();
        }
        default: {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
        }
    }
    return operand_none();
}

static void definition(struct unit_module* ir_module, struct ast_module* module, struct ast_node* node) {
    switch (node->type) {
        case AST_NODE_TYPE_FUNCTION: {
            struct unit* ir = unit_module_find(ir_module, node->children[0]->token);
            struct ast_node* type = node->children[1]; //type
            struct ast_node* args = node->children[2]; // args sequence
            struct ast_node* body = node->children[3]; //function body
            struct compiler* compiler = compiler_new(module, ir_module, ir, ir->return_type);
            compiler_begin(compiler);

            argument(compiler, args);

            struct operand operand = statement(compiler, body);

            if (operand.type != OPERAND_TYPE_END) {
                struct ssa_instruction goto_instruction = {};
                goto_instruction.operator = OP_GOTO;
                goto_instruction.result = operand_end();
                goto_instruction.operands[0] = operand_block(compiler->exit);
                block_add(compiler->body, goto_instruction);
                block_link(compiler->body, compiler->exit);
            }

            compiler_end(compiler);

            compiler_free(compiler);
            break;
        }
        case AST_NODE_TYPE_VARIABLE: {
            struct unit* ir = unit_module_find(ir_module, node->children[0]->token);
            struct ast_node* type = node->children[0];
            struct ast_node* value = node->children[1];
            //TODO: implement IR instructions for generating this stuff
            break;
        }
        default: {
            fprintf(stderr, "unexpected node type: %d\n", node->type);
            break;
        }
    }
}

void unit_module_build(struct unit_module* module) {
    for (int i = 0; i < module->ast->root->children_count; i++) {
        definition(module, module->ast, module->ast->root->children[i]);
    }
}
