#ifndef NCC_CHUNK_H
#define NCC_CHUNK_H
#include <stdint.h>

enum ir_ops {
    IR_OP_GLOBAL,
    IR_OP_LABEL,
    IR_OP_IMMEDIATE_I8,
    IR_OP_IMMEDIATE_I16,
    IR_OP_IMMEDIATE_I32,
    IR_OP_IMMEDIATE_I64,
    IR_OP_IMMEDIATE_F32,
    IR_OP_IMMEDIATE_F64,
    IR_OP_REGISTER_I8,
    IR_OP_REGISTER_I16,
    IR_OP_REGISTER_I32,
    IR_OP_REGISTER_I64,
    IR_OP_REGISTER_F8,
    IR_OP_REGISTER_F16,
    IR_OP_REGISTER_F32,
    IR_OP_REGISTER_F64,
    IR_OP_ADDRESS, // [ADDRESS] [CONST I64] [N] [REG I64] [N] [OFFSET]
    IR_OP_MOV,
    IR_OP_LEA,
    IR_OP_IADD,
    IR_OP_ISUB,
    IR_OP_IMUL,
    IR_OP_IDIV,
    IR_OP_PUSH,
    IR_OP_POP,
    IR_OP_RETURN,
    IR_OP_JUMP,
    IR_OP_ENTER,
    IR_OP_LEAVE,
    IR_OP_SALLOC,
};

enum ir_reg {
    IR_REG_0,
    IR_REG_1,
    IR_REG_2,
    IR_REG_3,
    IR_REG_4,
    IR_REG_5,
    IR_REG_6,
    IR_REG_7,
    IR_REG_8,
    IR_REG_9,
    IR_REG_10,
    IR_REG_11,
    IR_REG_12,
    IR_REG_13,
    IR_REG_14,
    IR_REG_15,
    IR_REG_16,
    IR_REG_17,
    IR_REG_18,
    IR_REG_19,
    IR_REG_20,
    IR_REG_21,
    IR_REG_22,
    IR_REG_23,
    IR_REG_24,
    IR_REG_25,
    IR_REG_26,
    IR_REG_27,
    IR_REG_28,
    IR_REG_29,
    IR_REG_30,
    IR_REG_31,
    
    IR_REG_RBP,
    IR_REG_RSP,
    
    IR_REG_RET0,
    IR_REG_RET1,
    IR_REG_RET2,
    IR_REG_RET3,
    IR_REG_RET4,
    IR_REG_RET5,
    IR_REG_RET6,
    IR_REG_RET7,
    IR_REG_RET8,
};

struct ir {
    uint8_t* code;
    uint32_t capacity;
    uint32_t count;
};

struct ir* ir_new();

void ir_free(struct ir* chunk);

void ir_write(struct ir* chunk, uint8_t byte);

void ir_string(struct ir* chunk, const char* string);

void ir_bytes(struct ir* chunk, const uint8_t* bytes, uint32_t count);

void ir_debug(struct ir* chunk);

#endif //NCC_CHUNK_H