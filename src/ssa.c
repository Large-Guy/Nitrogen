#include "ssa.h"
#include "ir.h"

struct operand operand_reg(uint32_t reg, enum ssa_type type) {
    return (struct operand){OPERAND_TYPE_REGISTER, type, reg};
}

struct operand operand_const_int(uint64_t constant) {
    return (struct operand){OPERAND_TYPE_INTEGER, TYPE_I32, constant};
}

struct operand operand_const_float(double constant) {
    return (struct operand){OPERAND_TYPE_FLOAT, TYPE_F32, {.floating = constant}};
}

struct operand operand_block(struct block* block) {
    return (struct operand){OPERAND_TYPE_BLOCK, TYPE_VOID, {.block = block}};
}

struct operand operand_end() {
    return (struct operand){OPERAND_TYPE_END};
}

struct operand operand_none() {
    return (struct operand){OPERAND_TYPE_NONE};
}

struct operand operand_ir(struct ir* ir) {
    return (struct operand){OPERAND_TYPE_IR, TYPE_VOID, {.ir = ir}};
}
