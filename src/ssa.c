#include "ssa.h"

#include "ast.h"
#include "unit.h"

struct operand operand_reg(uint32_t reg, struct ssa_type type)
{
    return (struct operand){OPERAND_TYPE_REGISTER, type, reg};
}

struct operand operand_block(struct block* block)
{
    return (struct operand){OPERAND_TYPE_BLOCK, {}, {.block = block}};
}

struct operand operand_end()
{
    return (struct operand){OPERAND_TYPE_END};
}

struct operand operand_none()
{
    return (struct operand){OPERAND_TYPE_NONE};
}

struct operand operand_unit(struct unit* unit)
{
    return (struct operand){OPERAND_TYPE_IR, {}, {.unit = unit}};
}
struct operand operand_const_i8(int8_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I8, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_INTEGER, get_node_type(NULL, &integer_node), {.integer = value}};
}

struct operand operand_const_i16(int16_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I16, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_INTEGER, get_node_type(NULL, &integer_node), {.integer = value}};
}

struct operand operand_const_i32(int32_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I32, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_INTEGER, get_node_type(NULL, &integer_node), {.integer = value}};
}

struct operand operand_const_i64(int64_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I64, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_INTEGER, get_node_type(NULL, &integer_node), {.integer = value}};
}

struct operand operand_const_f32(float value)
{
    static struct ast_node float_node = {AST_NODE_TYPE_F32, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_FLOAT, get_node_type(NULL, &float_node), {.floating = value}};
}

struct operand operand_const_f64(double value)
{
    static struct ast_node float_node = {AST_NODE_TYPE_F64, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_FLOAT, get_node_type(NULL, &float_node), {.floating = value}};
}
