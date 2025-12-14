#include "ssa.h"

struct operand operand_reg(uint32_t reg) {
    return (struct operand){OPERAND_TYPE_REGISTER, reg};
}

struct operand operand_const(uint64_t constant) {
    return (struct operand){OPERAND_TYPE_CONSTANT, constant};
}
