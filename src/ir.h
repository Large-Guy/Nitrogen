#ifndef COMPILER_CHUNK_H
#define COMPILER_CHUNK_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum op_codes {
    OP_NONE,
    OP_RETURN,

    //32-bit
    OP_IMM_8,
    OP_IMM_16,
    OP_IMM_32,
    OP_IMM_64,
    OP_IADD,
    OP_ISUB,
    OP_IMUL,
    OP_ISDIV,
    OP_IUDIV,
    OP_IMOD,

    //variables
    OP_SET,
    OP_GET,
};

struct ir {
    char* symbol;
    bool global;
    uint8_t* code;
    size_t capacity;
    size_t size;
    uint8_t* locals; //sizes of locals in bytes
    size_t local_capacity;
    size_t local_size;
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

struct ir* ir_new(char* symbol, bool global);

void ir_free(struct ir* chunk);

void ir_push(struct ir* chunk, uint8_t byte);

void ir_push32(struct ir* chunk, uint32_t value);

uint8_t ir_declare(struct ir* chunk, uint8_t size);

void ir_debug(struct ir* chunk);

void ir_module_debug(struct ir_module* module);

char* ir_compile(struct ir* chunk, FILE* file);

#endif //COMPILER_CHUNK_H