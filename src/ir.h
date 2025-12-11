#ifndef COMPILER_CHUNK_H
#define COMPILER_CHUNK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum op_code {
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

enum type_code {
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

enum chunk_type {
    CHUNK_TYPE_FUNCTION,
    CHUNK_TYPE_VARIABLE,
};

struct instruction {
    enum op_code operator;

    enum type_code type;
    
    // this should always be a register
    uint32_t result;
    
    // these could be a register or a constant depending on the operator
    uint64_t operand1;
    uint64_t operand2;
};

struct ir {
    char* symbol;
    enum chunk_type type;
    bool global;

    struct instruction* instructions;
    uint32_t instructions_size;
    uint32_t instructions_capacity;

    uint32_t registers;
};

struct ir_module {
    char* name;

    struct ir** chunks;
    size_t count;
    size_t capacity;
};

struct ir_module* ir_module_new(char* name);

void ir_module_free(struct ir_module* list);

void ir_module_append(struct ir_module* list, struct ir* chunk);

struct ir* ir_new(char* symbol, bool global, enum chunk_type type);

void ir_free(struct ir* chunk);

uint32_t ir_constant(struct ir* chunk, enum type_code type, int64_t value);

uint32_t ir_add(struct ir* chunk, enum op_code code, enum type_code type, int operand1, int operand2);

void ir_debug(struct ir* chunk);

void ir_module_debug(struct ir_module* module);

char* ir_compile(struct ir* chunk, FILE* file);

#endif //COMPILER_CHUNK_H