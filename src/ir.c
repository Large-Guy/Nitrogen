#include "ir.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ir* ir_new(char* symbol, bool global, enum chunk_type type) {
    struct ir* chunk = malloc(sizeof(struct ir));
    assert(chunk);
    
    chunk->symbol = malloc(strlen(symbol) + 1);
    assert(chunk->symbol);
    strcpy(chunk->symbol, symbol);
    chunk->symbol[strlen(symbol)] = '\0';
    
    chunk->type = type;
    
    chunk->global = global;

    chunk->instructions = malloc(sizeof(struct instruction));
    assert(chunk->instructions);
    chunk->instructions_size = 0;
    chunk->instructions_capacity = 1;

    chunk->registers = 0;
    
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
    free(chunk->instructions);
    free(chunk);
}

uint32_t ir_constant(struct ir* chunk, enum type_code type, int64_t value) {
    assert(chunk != NULL);
    if (chunk->instructions_size >= chunk->instructions_capacity) {
        chunk->instructions_capacity *= 2;
        chunk->instructions = realloc(chunk->instructions, chunk->instructions_capacity * sizeof(struct instruction));
        assert(chunk->instructions);
    }
    struct instruction instruction;
    instruction.operator = OP_CONST;
    instruction.type = type;
    instruction.result = chunk->registers;
    instruction.operand1 = value;
    instruction.operand2 = 0;
    chunk->instructions[chunk->instructions_size++] = instruction;
    return chunk->registers++;
}

uint32_t ir_add(struct ir* chunk, enum op_code code, enum type_code type, int operand1, int operand2) {
    assert(chunk != NULL);
    if (chunk->instructions_size >= chunk->instructions_capacity) {
        chunk->instructions_capacity *= 2;
        chunk->instructions = realloc(chunk->instructions, chunk->instructions_capacity * sizeof(struct instruction));
        assert(chunk->instructions);
    }
    struct instruction instruction;
    instruction.operator = code;
    instruction.type = type;
    instruction.result = chunk->registers;
    instruction.operand1 = operand1;
    instruction.operand2 = operand2;
    chunk->instructions[chunk->instructions_size++] = instruction;
    return chunk->registers++;
}

static char* type_code_name(enum type_code code) {
    switch (code) {
        case TYPE_U8:
            return "U8";
        case TYPE_U16:
            return "U16";
        case TYPE_U32:
            return "U32";
        case TYPE_U64:
            return "U64";
        case TYPE_I8:
            return "I8";
        case TYPE_I16:
            return "I16";
        case TYPE_I32:
            return "I32";
        case TYPE_I64:
            return "I64";
        case TYPE_F32:
            return "F32";
        case TYPE_F64:
            return "F64";
    }
    return "UNRECOGNIZED TYPE";
}

static char* operator_name(enum op_code code) {
    switch (code) {
        case OP_ADD:
            return "add";
        case OP_SUB:
            return "sub";
        case OP_MUL:
            return "mul";
        case OP_DIV:
            return "div";
        case OP_RETURN:
            return "return";
        default:
            return "unsupported";
    }
}

static void instruction_debug(struct instruction instruction) {
    printf("%%%d = ", instruction.result);
    if (instruction.operator == OP_CONST) {
        printf("const %llu ", instruction.operand1);
        printf("%s\n", type_code_name(instruction.type));
        return;
    }

    printf("%s ", operator_name(instruction.operator));

    printf("%%%llu, %%%llu ", instruction.operand1, instruction.operand2);

    printf("%s\n", type_code_name(instruction.type));
}

void ir_debug(struct ir* chunk) {
    assert(chunk != NULL);
    assert(chunk->instructions != NULL);
    for (size_t i = 0; i < chunk->instructions_size; i++) {
        instruction_debug(chunk->instructions[i]);
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

char* ir_compile(struct ir* chunk, FILE* out) {
    //TODO: compile to ARM64
}
