#include "ir.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "block.h"

struct ir* ir_new(char* symbol, bool global, enum chunk_type type) {
    struct ir* chunk = malloc(sizeof(struct ir));
    assert(chunk);
    
    chunk->symbol = malloc(strlen(symbol) + 1);
    assert(chunk->symbol);
    strcpy(chunk->symbol, symbol);
    chunk->symbol[strlen(symbol)] = '\0';
    
    chunk->type = type;
    
    chunk->global = global;

    chunk->blocks = malloc(sizeof(struct block*));
    assert(chunk->blocks);
    chunk->block_count = 0;
    chunk->block_capacity = 1;
    
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
    for (int i = 0; i < chunk->block_count; i++) {
        block_free(chunk->blocks[i]);
    }
    free(chunk->blocks);
    free(chunk);
}

void ir_add(struct ir* chunk, struct block* block) {
    assert(chunk != NULL);
    if (chunk->block_count >= chunk->block_capacity) {
        chunk->block_capacity *= 2;
        chunk->blocks = realloc(chunk->blocks, chunk->block_capacity * sizeof(struct ssa_instruction));
        assert(chunk->blocks);
    }
    chunk->blocks[chunk->block_count++] = block;
}

static char* type_code_name(enum ssa_type code) {
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

static char* operator_name(enum ssa_instruction_code code) {
    switch (code) {
        case OP_CONST:
            return "const";
        case OP_ADD:
            return "add";
        case OP_SUB:
            return "sub";
        case OP_MUL:
            return "mul";
        case OP_DIV:
            return "div";
        case OP_LESS:
            return "less";
        case OP_LESS_EQUAL:
            return "less_equal";
        case OP_GREATER:
            return "greater";
        case OP_GREATER_EQUAL:
            return "greater_equal";
        case OP_EQUAL:
            return "equal";
        case OP_NOT_EQUAL:
            return "not_equal";
        case OP_GOTO:
            return "goto";
        case OP_IF:
            return "if";
        case OP_RETURN:
            return "return";
        default:
            return "unsupported";
    }
}

static void operand_debug(struct operand operand) {
    switch (operand.type) {
        case OPERAND_UNUSED:
            break;
        case OPERAND_TYPE_CONSTANT:
            printf("#%llu ", operand.value.integer);
            break;
        case OPERAND_TYPE_REGISTER:
            printf("%%%llu ", operand.value.integer);
            break;
        case OPERAND_TYPE_BLOCK:
            printf("&%p ", operand.value.block);
    }
}

static void instruction_debug(struct ssa_instruction instruction) {
    operand_debug(instruction.result);

    printf("%s ", operator_name(instruction.operator));

    operand_debug(instruction.operand1);

    operand_debug(instruction.operand2);
    
    printf("%s\n", type_code_name(instruction.type));
}

static void block_debug(struct block* block) {
    if (!block->entry) {
        printf("---> ");
        for (int i = 0; i < block->parents_count; i++) {
            struct block* parent = block->parents[i];
            printf("[%p] ", parent);
        }
        printf("---> ");
    }
    printf("BLOCK [%p] ---\n", block);
    for (int i = 0; i < block->instructions_count; i++) {
        instruction_debug(block->instructions[i]);
    }
    if (block->instructions_count > 0) {
        printf("---> ");
        for (int i = 0; i < block->children_count; i++) {
            struct block* child = block->children[i];
            printf("[%p] ", child);
        }
        printf("\n\n");
    }
}

void ir_debug(struct ir* chunk) {
    assert(chunk != NULL);
    assert(chunk->blocks != NULL);
    for (size_t i = 0; i < chunk->block_count; i++) {
        block_debug(chunk->blocks[i]);
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
