#include "ir.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct ir* ir_new() {
    struct ir* chunk = malloc(sizeof(struct ir));
    chunk->code = malloc(1);
    chunk->count = 0;
    chunk->capacity = 1;
    return chunk;
}

void ir_free(struct ir* chunk) {
    free(chunk);
}

void ir_write(struct ir* chunk, uint8_t byte) {
    if (chunk->count >= chunk->capacity) {
        chunk->capacity *= 2;
        chunk->code = realloc(chunk->code, chunk->capacity);
        assert(chunk->code);
    }
    chunk->code[chunk->count++] = byte;
}

void ir_string(struct ir* chunk, const char* string) {
    for (int i = 0; string[i]; i++) {
        ir_write(chunk, string[i]);
    }
    ir_write(chunk, '\0');
}

void ir_bytes(struct ir* chunk, const uint8_t* bytes, uint32_t count) {
    for (int i = 0; i < count; i++) {
        ir_write(chunk, bytes[i]);
    }
}

void ir_debug(struct ir* chunk) {
    int ip = 0;
    while (ip < chunk->count) {
        switch (chunk->code[ip++]) {
            case IR_OP_GLOBAL: {
                printf("[ir] global\n");
                break;
            }
            case IR_OP_LABEL: {
                printf("[ir] label: ");
                while (chunk->code[ip])
                    printf("%c", chunk->code[ip++]);
                ip++;
                printf("\n");
                break;
            }
            case IR_OP_IMMEDIATE_I8: {
                int8_t i = chunk->code[ip++];
                printf("[ir] immediate_i8: %d\n", i);
                break;
            }
            case IR_OP_IMMEDIATE_I16: {
                int16_t i = *(int16_t*) &chunk->code[ip += 2];
                ip += 2;
                printf("[ir] immediate_i16: %d\n", i);
                break;
            }
            case IR_OP_IMMEDIATE_I32: {
                int32_t i = *(int32_t*) &chunk->code[ip];
                ip += 4;
                printf("[ir] immediate_i32: %d\n", i);
                break;
            }
            case IR_OP_IMMEDIATE_I64: {
                int64_t i = *(int64_t*) &chunk->code[ip];
                ip += 8;
                printf("[ir] immediate_i64: %lld\n", i);
                break;
            }
            case IR_OP_IMMEDIATE_F32: {
                float f = *(float*) &chunk->code[ip += 4];
                printf("[ir] immediate_f32: %f\n", f);
                break;
            }
            case IR_OP_IMMEDIATE_F64: {
                double f = *(double*) &chunk->code[ip += 8];
                printf("[ir] immediate_f32: %f\n", f);
                break;
            }
            case IR_OP_REGISTER_I8: {
                int8_t r = chunk->code[ip++];
                printf("[ir] reg(i8): %d\n", r);
                break;
            }
            case IR_OP_REGISTER_I16: {
                int8_t r = chunk->code[ip++];
                printf("[ir] reg(i16): %d\n", r);
                break;
            }
            case IR_OP_REGISTER_I32: {
                int8_t r = chunk->code[ip++];
                printf("[ir] reg(i32): %d\n", r);
                break;
            }
            case IR_OP_REGISTER_I64: {
                int8_t r = chunk->code[ip++];
                printf("[ir] reg(i64): %d\n", r);
                break;
            }
            case IR_OP_REGISTER_F32: {
                int8_t r = chunk->code[ip++];
                printf("[ir] reg(f32): %d\n", r);
                break;
            }
            case IR_OP_REGISTER_F64: {
                int8_t r = chunk->code[ip++];
                printf("[ir] reg(f64): %d\n", r);
                break;
            }
            case IR_OP_ADDRESS: {
                printf("[ir] address-of\n");
                break;
            }
            case IR_OP_MOV: {
                printf("[ir] mov\n");
                break;
            }
            case IR_OP_IADD: {
                printf("[ir] iadd\n");
                break;
            }
            case IR_OP_ISUB: {
                printf("[ir] isub\n");
                break;
            }
            case IR_OP_IMUL: {
                printf("[ir] imul\n");
                break;
            }
            case IR_OP_IDIV: {
                printf("[ir] idiv\n");
                break;
            }
            case IR_OP_PUSH: {
                printf("[ir] push\n");
                break;
            }
            case IR_OP_POP: {
                printf("[ir] pop\n");
                break;
            }
            case IR_OP_RETURN: {
                printf("[ir] return\n");
                break;
            }
            case IR_OP_JUMP: {
                printf("[ir] jump\n");
                break;
            }
            case IR_OP_ENTER: {
                printf("[ir] enter\n");
                break;
            }
            case IR_OP_LEAVE: {
                printf("[ir] leave\n");
                break;
            }
            case IR_OP_SALLOC: {
                printf("[ir] salloc\n");
                break;
            }
            default: {
                printf("[ir] unknown %d\n", chunk->code[ip - 1]);
            }
        }
    }
}
