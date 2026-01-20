#ifndef NCC_CHUNK_H
#define NCC_CHUNK_H
#include <stdint.h>

struct chunk {
    uint8_t* code;
    uint32_t capacity;
    uint32_t count;
};

struct chunk* chunk_new();

void chunk_free(struct chunk* chunk);

void chunk_write(struct chunk* chunk, uint8_t byte);

void chunk_string(struct chunk* chunk, const char* string);

void chunk_bytes(struct chunk* chunk, const uint8_t* bytes, uint32_t count);

#endif //NCC_CHUNK_H