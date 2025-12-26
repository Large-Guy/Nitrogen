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

struct operand operand_ir(struct unit* ir)
{
    return (struct operand){OPERAND_TYPE_IR, {}, {.ir = ir}};
}

struct operand operand_const_int(uint64_t value)
{
    static struct ast_node integer_node = {AST_NODE_TYPE_I64, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_INTEGER, get_node_type(NULL, &integer_node), {.integer = value}};
}

struct operand operand_const_float(double value)
{
    static struct ast_node float_node = {AST_NODE_TYPE_F64, {}, NULL, NULL, 0, 0};

    return (struct operand){OPERAND_TYPE_INTEGER, get_node_type(NULL, &float_node), {.floating = value}};
}
