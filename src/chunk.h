#ifndef COMPILER_CHUNK_H
#define COMPILER_CHUNK_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum op_codes {
    OP_NONE,
    OP_RETURN,

    //8-bit
    OP_IMM_32,
    OP_ADD_32,
    OP_SUB_32,
    OP_MUL_32,
    OP_DIV_32,
    OP_MOD_32,

    
    //variables
    OP_SET,
    OP_GET,
};

struct chunk {
    char* symbol;
    uint8_t* code;
    size_t capacity;
    size_t size;
    uint8_t* locals; //sizes of locals in bytes
    size_t local_capacity;
    size_t local_size;
};

struct chunk_list {
    struct chunk** chunks;
    size_t count;
    size_t capacity;
};

struct chunk_list* chunk_list_new();

void chunk_list_free(struct chunk_list* list);

void chunk_list_append(struct chunk_list* list, struct chunk* chunk);

struct chunk* chunk_new(char* symbol);

void chunk_free(struct chunk* chunk);

void chunk_push(struct chunk* chunk, uint8_t byte);

void chunk_push32(struct chunk* chunk, uint32_t value);

uint8_t chunk_declare(struct chunk* chunk, uint8_t size);

void chunk_debug(struct chunk* chunk);

char* chunk_compile(struct chunk* chunk, FILE* file);

#endif //COMPILER_CHUNK_H