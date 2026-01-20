#include "chunk.h"

#include <assert.h>
#include <stdlib.h>

struct chunk * chunk_new() {
    struct chunk* chunk = malloc(sizeof(struct chunk));
    chunk->code = malloc(1);
    chunk->count = 0;
    chunk->capacity = 1;
    return chunk;
}

void chunk_free(struct chunk* chunk) {
    free(chunk);
}

void chunk_write(struct chunk* chunk, uint8_t byte) {
    if (chunk->count >= chunk->capacity) {
        chunk->capacity *= 2;
        chunk->code = realloc(chunk->code, chunk->capacity);
        assert(chunk->code);
    }
    chunk->code[chunk->count++] = byte;
}

void chunk_string(struct chunk *chunk, const char *string) {
    for (int i = 0; string[i]; i++) {
        chunk_write(chunk, string[i]);
    }
    chunk_write(chunk, '\0');
}

void chunk_bytes(struct chunk *chunk, const uint8_t *bytes, uint32_t count) {
    for (int i = 0; i < count; i++) {
        chunk_write(chunk, bytes[i]);
    }
}
