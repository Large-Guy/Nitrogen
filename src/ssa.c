#include "ssa.h"

#include "ast.h"
#include "unit.h"

struct ssa_type ssa_type_from_ast(struct ast_module* module, struct ast_node* node)
{
    return (struct ssa_type){ast_node_symbol_size(module, node), module, node};
}

struct ssa_operand operand_reg(uint32_t reg, struct ssa_type type)
{
    return (struct ssa_operand){OPERAND_TYPE_REGISTER, type, reg};
}

struct ssa_operand operand_block(struct block* block)
{
    return (struct ssa_operand){OPERAND_TYPE_BLOCK, {}, {.block = block}};
}

struct ssa_operand operand_end()
{
    return (struct ssa_operand){OPERAND_TYPE_END};
}

struct ssa_operand operand_none()
{
    return (struct ssa_operand){OPERAND_TYPE_NONE};
}

struct ssa_operand operand_unit(struct unit* unit)
{
    return (struct ssa_operand){OPERAND_TYPE_IR, {}, {.list = {unit, 0}}};
}
struct ssa_operand operand_const_i8(int8_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I8, {}, NULL, NULL, 0, 0};

    return (struct ssa_operand){OPERAND_TYPE_INTEGER, ssa_type_from_ast(NULL, &integer_node), {.integer = value}};
}

struct ssa_operand operand_const_i16(int16_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I16, {}, NULL, NULL, 0, 0};

    return (struct ssa_operand){OPERAND_TYPE_INTEGER, ssa_type_from_ast(NULL, &integer_node), {.integer = value}};
}

struct ssa_operand operand_const_i32(int32_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I32, {}, NULL, NULL, 0, 0};

    return (struct ssa_operand){OPERAND_TYPE_INTEGER, ssa_type_from_ast(NULL, &integer_node), {.integer = value}};
}

struct ssa_operand operand_const_i64(int64_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I64, {}, NULL, NULL, 0, 0};

    return (struct ssa_operand){OPERAND_TYPE_INTEGER, ssa_type_from_ast(NULL, &integer_node), {.integer = value}};
}

struct ssa_operand operand_const_f32(float value)
{
    static struct ast_node float_node = {AST_NODE_TYPE_F32, {}, NULL, NULL, 0, 0};

    return (struct ssa_operand){OPERAND_TYPE_FLOAT, ssa_type_from_ast(NULL, &float_node), {.floating = value}};
}

struct ssa_operand operand_const_f64(double value)
{
    static struct ast_node float_node = {AST_NODE_TYPE_F64, {}, NULL, NULL, 0, 0};

    return (struct ssa_operand){OPERAND_TYPE_FLOAT, ssa_type_from_ast(NULL, &float_node), {.floating = value}};
}

struct ssa_operand operand_null() {
    static struct ast_node null_node = {AST_NODE_TYPE_NULL, {}, NULL, NULL, 0, 0};
    
    return (struct ssa_operand){OPERAND_TYPE_NULL, ssa_type_from_ast(NULL, &null_node), {.integer = 0}};
}

struct ssa_operand operand_list(struct unit* unit, struct ssa_operand* operands, uint32_t count, struct ssa_type type) {
    uint32_t list = unit_add_data(unit, operands, count);
    return (struct ssa_operand){OPERAND_TYPE_LIST, type, {.list = {unit, list, count}}};
}
