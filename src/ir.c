#include "ir.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ir* ir_new(char* symbol) {
    struct ir* chunk = malloc(sizeof(struct ir));
    assert(chunk);
    chunk->symbol = malloc(strlen(symbol) + 1);
    assert(chunk->symbol);
    strcpy(chunk->symbol, symbol);
    chunk->code = malloc(1);
    chunk->capacity = 1;
    chunk->size = 0;
    chunk->locals = malloc(sizeof(size_t));
    assert(chunk->locals);
    chunk->local_capacity = 1;
    chunk->local_size = 0;
    return chunk;
}

struct ir_module* ir_module_new(char* name) {
    struct ir_module* list = malloc(sizeof(struct ir_module));
    assert(list);
    list->name = malloc(strlen(name) + 1);
    memcpy(list->name, name, strlen(name) + 1);
    list->chunks = malloc(sizeof(struct ir*));
    list->count = 0;
    list->capacity = 1;
    return list;
}

void ir_module_free(struct ir_module* list) {
    assert(list);
    free(list->name);
    for (int i = 0; i < list->count; i++)
    {
        ir_free(list->chunks[i]);
    }
    free(list->chunks);
    free(list);
}

void ir_module_append(struct ir_module* list, struct ir* chunk) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->chunks = realloc(list->chunks, list->capacity * sizeof(struct ir*));
        assert(list->chunks);
    }
    list->chunks[list->count++] = chunk;
}

void ir_free(struct ir* chunk) {
    assert(chunk != NULL);
    free(chunk->symbol);
    free(chunk->code);
    free(chunk->locals);
    free(chunk);
}

void ir_push(struct ir* chunk, uint8_t byte) {
    if (chunk->size >= chunk->capacity) {
        chunk->capacity *= 2;
        chunk->code = realloc(chunk->code, chunk->capacity);
        assert(chunk->code);
    }
    chunk->code[chunk->size++] = byte;
}

void ir_push32(struct ir* chunk, uint32_t value) {
    ir_push(chunk, (value) & 0xff);
    ir_push(chunk, (value >> 8) & 0xff);
    ir_push(chunk, (value >> 16) & 0xff);
    ir_push(chunk, (value >> 24) & 0xff);
}

uint8_t ir_declare(struct ir* chunk, uint8_t size) {
    if (chunk->local_size > chunk->local_capacity) {
        chunk->capacity *= 2;
        chunk->code = realloc(chunk->code, chunk->capacity);
        assert(chunk->code);
    }

    chunk->locals[chunk->local_size++] = size;
    return chunk->local_size - 1;
}

void ir_debug(struct ir* chunk) {
    assert(chunk != NULL);
    assert(chunk->code != NULL);
    for (size_t i = 0; i < chunk->size; i++) {
        printf("Byte: %lu, %d\n", i, chunk->code[i]);
    }
}

void ir_module_debug(struct ir_module* module) {
    for (int i = 0; i < module->count; i++)
    {
        printf("CHUNK [%s] ---\n", module->chunks[i]->symbol);
        ir_debug(module->chunks[i]);
        printf("\n");
    }
}

struct compiler {
    uint32_t* locals; //offsets
};

static uint32_t round_up_16(uint32_t size) {
    return (uint32_t)(ceilf((float)size / 16.0f) * 16.0f);
}

static uint8_t as_imm8(struct ir* chunk, uint32_t ip) {
    return chunk->code[ip];
}

static uint32_t as_imm32(struct ir* chunk, uint32_t ip) {
    return *(uint32_t*)&chunk->code[ip];
}

char* ir_compile(struct ir* chunk, FILE* out) {
    assert(chunk != NULL);
    assert(chunk->code != NULL);

    struct compiler compiler;
    compiler.locals = malloc(sizeof(size_t) * chunk->local_size);

    assert(compiler.locals != NULL);

    //function header stuff
    uint32_t ip = 0;
    fprintf(out, "%s:\n", chunk->symbol);

    //allocate locals
    uint32_t total_local_size = 0;
    for (size_t i = 0; i < chunk->local_size; i++) {
        compiler.locals[i] = total_local_size;
        total_local_size += chunk->locals[i];
    }
    fprintf(out, "  sub sp, sp, #%d // allocate space for local variables\n", round_up_16(total_local_size));
    fprintf(out, "  mov x20, sp\n"); //end of locals

    fprintf(out, "\n"); //space after header
    
    while (ip < chunk->size) {
        uint8_t byte = chunk->code[ip];

        switch (byte) {
            case OP_NONE: {
                fprintf(out, "  nop\n");
                break;
            }
            case OP_RETURN: {
                goto end_function;
            }
            case OP_IMM_32: {
                //push an 32-bit integer number to the stack
                fprintf(out, "  mov w1, #%d\n", as_imm32(chunk, ++ip)); //imm32 value in register x0
                ip += 3;
                fprintf(out, "  str w1, [x20, #-16]!\n"); //push to stack
                break;
            }
            case OP_ADD_32: {
                fprintf(out, "  ldr w1, [x20], #16\n"); //pop a
                fprintf(out, "  ldr w2, [x20], #16\n"); //pop b
                fprintf(out, "  add w0, w1, w2\n"); //a + b to return register w0
                fprintf(out, "  str w0, [x20, #-16]!\n"); //push onto stack
                break;
            }
            case OP_SUB_32: {
                fprintf(out, "  ldr w2, [x20], #16\n"); //pop b
                fprintf(out, "  ldr w1, [x20], #16\n"); //pop a
                fprintf(out, "  sub w0, w1, w2\n"); //a - b to return register w0
                fprintf(out, "  str w0, [x20, #-16]!\n"); //push onto stack
                break;
            }
            case OP_MUL_32: {
                fprintf(out, "  ldr w1, [x20], #16\n"); //pop a
                fprintf(out, "  ldr w2, [x20], #16\n"); //pop b
                fprintf(out, "  mul w0, w1, w2\n"); //a * b to return register w0
                fprintf(out, "  str w0, [x20, #-16]!\n"); //push onto stack
                break;
            }
            case OP_DIV_32: { //TODO: signed vs unsigned
                fprintf(out, "  ldr w2, [x20], #16\n"); //pop b
                fprintf(out, "  ldr w1, [x20], #16\n"); //pop a
                fprintf(out, "  div w0, w1, w2\n"); //a / b to return register w0
                fprintf(out, "  str w0, [x20, #-16]!\n"); //push onto stack
                break;
            }
            case OP_SET: { //TODO: handle different sizes
                const uint8_t id = as_imm8(chunk, ++ip); //variable id
                fprintf(out, "  ldr w1, [x20], #16\n"); //pop value from stack
                fprintf(out, "  str w1, [sp, #%d]\n", compiler.locals[id]); //store that at the variables offset in the pre-allocated stack
                break;
            }
            case OP_GET: {
                const uint8_t id = as_imm8(chunk, ++ip);
                fprintf(out, "  ldr w1, [sp, #%d]\n", compiler.locals[id]); //load the variable into the 1st register
                fprintf(out, "  str w1, [x20, #-16]!\n"); //store that at the top of the stack
                break;
            }
            default: {
                //Invalid OP
            }
        }

        fprintf(out, "\n"); //space between commands

        ip++;
    }

    end_function:

    //cleanup
    fprintf(out, "  add sp, sp, #%d\n", round_up_16(total_local_size)); //deallocate locals

    //top of stack is return
    fprintf(out, "  ldr w0, [x20], #16\n"); //pop a
    
    fprintf(out, "  ret\n"); //ret, value should be in the stack

    free(compiler.locals);
}
