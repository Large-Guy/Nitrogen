#ifndef COMPILER_SSA_H
#define COMPILER_SSA_H
#include <stdint.h>

enum ssa_instruction_code {
    OP_NONE,
    OP_RETURN,

    //32-bit
    OP_CONST,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,

    //variables
    OP_LOAD,
    OP_STORE
};

enum ssa_type {
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_F32,
    TYPE_F64,
};

struct ssa_instruction {
    enum ssa_instruction_code operator;

    enum ssa_type type;
    
    // this should always be a register
    uint32_t result;
    
    // these could be a register or a constant depending on the operator
    uint64_t operand1;
    uint64_t operand2;
};

#endif //COMPILER_SSA_H