#ifndef COMPILER_SSA_H
#define COMPILER_SSA_H
#include <stdbool.h>
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

    OP_BITWISE_AND,
    OP_BITWISE_OR,
    OP_BITWISE_XOR,
    OP_BITWISE_NOT,
    OP_BITWISE_LEFT,
    OP_BITWISE_RIGHT,
    
    OP_NEGATE,
    OP_NOT,

    OP_AND,
    OP_OR,
    
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_EQUAL,
    OP_NOT_EQUAL,

    OP_GOTO,
    OP_IF,

    //variables
    OP_CALL,
    OP_ALLOC,
    OP_LOAD,
    OP_STORE,
    OP_CAST,
};

enum ssa_type {
    TYPE_VOID,
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

enum operand_type {
    OPERAND_TYPE_NONE,
    OPERAND_TYPE_END,
    OPERAND_TYPE_REGISTER,
    OPERAND_TYPE_INTEGER,
    OPERAND_TYPE_FLOAT,
    OPERAND_TYPE_BLOCK,
    OPERAND_TYPE_IR,
};

struct operand {
    enum operand_type type;
    enum ssa_type typename;
    union {
        uint64_t integer;
        double floating;
        struct block* block;
        struct ir* ir;
    } value;
    
    bool pointer;
};

struct operand operand_none();

struct operand operand_reg(uint32_t reg, enum ssa_type type);

struct operand operand_const_int(uint64_t constant);

struct operand operand_const_float(double constant);

struct operand operand_block(struct block* block);

struct operand operand_end();

struct operand operand_ir(struct ir* ir);

#define MAX_OPERANDS 16

struct ssa_instruction {
    enum ssa_instruction_code operator;

    enum ssa_type type;
    
    // this should always be a register
    struct operand result;
    
    // these could be a register or a constant depending on the operator
    struct operand operands[MAX_OPERANDS];
};

#endif //COMPILER_SSA_H