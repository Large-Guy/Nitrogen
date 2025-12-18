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

struct ir* ir_module_find(struct ir_module* module, struct token symbol) {
    for (int i = 0; i < module->count; i++) {
        struct ir* chunk = module->chunks[i];
        if (symbol.length == strlen(chunk->symbol) && memcmp(chunk->symbol, symbol.start, symbol.length) == 0) {
            return module->chunks[i];
        }
    }
    return NULL;
}

void ir_add(struct ir* chunk, struct block* block) {
    assert(chunk != NULL);
    if (chunk->block_count >= chunk->block_capacity) {
        chunk->block_capacity *= 2;
        chunk->blocks = realloc(chunk->blocks, chunk->block_capacity * sizeof(struct ssa_instruction));
        assert(chunk->blocks);
    }
    chunk->blocks[chunk->block_count++] = block;
    block->id = chunk->block_count;
}

static char* type_code_name(enum ssa_type code) {
    switch (code) {
        case TYPE_VOID:
            return "void";
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
        case OP_BITWISE_AND:
            return "bitwise-and";
        case OP_BITWISE_OR:
            return "bitwise-or";
        case OP_BITWISE_XOR:
            return "bitwise-xor";
        case OP_BITWISE_NOT:
            return "bitwise-not";
        case OP_NEGATE:
            return "negate";
        case OP_NOT:
            return "not";
        case OP_AND:
            return "and";
        case OP_OR:
            return "or";
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
        case OP_CALL:
            return "call";
        case OP_ALLOC:
            return "alloc";
        case OP_STORE:
            return "store";
        case OP_LOAD:
            return "load";
        default:
            return "unsupported";
    }
}

static void operand_debug(struct operand operand, FILE* out) {
    switch (operand.type) {
        case OPERAND_TYPE_END:
            break;
        case OPERAND_TYPE_CONSTANT:
            fprintf(out, "[const #%llu] ", operand.value.integer);
            break;
        case OPERAND_TYPE_REGISTER:
            if (operand.value.integer == 0)
                fprintf(out, "[reg %%ret] "); // 0 should be return value of all chunks
            else
                fprintf(out, "[reg %%%llu] ", operand.value.integer);
            break;
        case OPERAND_TYPE_BLOCK:
            fprintf(out, "[block &%d] ", operand.value.block->id);
            break;
        case OPERAND_TYPE_NONE:
            break;
        case OPERAND_TYPE_IR:
            fprintf(out, "[func @%s] ", operand.value.ir->symbol);
    }
}

static void instruction_debug(struct ssa_instruction instruction, FILE* out) {
    operand_debug(instruction.result, out);

    if (instruction.result.type > OPERAND_TYPE_END)
        fprintf(out, "= ");
    
    fprintf(out, "%s ", operator_name(instruction.operator));

    for (int i = 0; i < MAX_OPERANDS; i++)
        operand_debug(instruction.operands[i], out);
    
    fprintf(out, "%s", type_code_name(instruction.type));
}

static void block_debug(struct block* block) {
    if (!block->entry) {
        printf("dominated by: ");
        for (int i = 0; i < block->parents_count; i++) {
            struct block* parent = block->parents[i];
            printf("BLOCK [%d] ", parent->id);
        }
        printf("---> ");
    }
    printf("BLOCK [%d] ---\n", block->id);
    for (int i = 0; i < block->instructions_count; i++) {
        instruction_debug(block->instructions[i], stdout);
        printf("\n");
    }
    if (block->children_count > 0) {
        printf("dominates: ");
        for (int i = 0; i < block->children_count; i++) {
            struct block* child = block->children[i];
            printf("BLOCK [%d] ", child->id);
        }
        printf("\n");
    }
    printf("\n");
}

void ir_debug(struct ir* chunk) {
    assert(chunk != NULL);
    assert(chunk->blocks != NULL);
    for (size_t i = 0; i < chunk->block_count; i++) {
        block_debug(chunk->blocks[i]);
    }
}

static void block_build_graph(char* name, struct block* block, FILE* out) {
    fprintf(out, "  %s_bb%d [label=\"", name, block->id);
    if (block->entry)
        fprintf(out, ".ENTRY\\l");
    else if (block->children_count == 0)
        fprintf(out, ".EXIT\\l");
    else
        fprintf(out, ".BLOCK %d\\l", block->id);
    for (int i = 0; i < block->instructions_count; i++) {
        instruction_debug(block->instructions[i], out);
        fprintf(out, "\\l");
    }
    fprintf(out, "\"];\n");
}

static void recursive_link(char* name, struct block* block, FILE* out) {
    for (int i = 0; i < block->children_count; i++) {
        struct block* child = block->children[i];
        fprintf(out, "    %s_bb%d -> %s_bb%d [color=black];\n", name, block->id, name, child->id);
        if (child->parents_count == 1 || child->parents[0] == block)
            recursive_link(name, child, out);
    }
}

void ir_build_graph(struct ir* chunk, FILE* out) {
    assert(chunk != NULL);
    assert(chunk->blocks != NULL);

    fprintf(out, "  subgraph cluster_%s {\n", chunk->symbol);
    fprintf(out, "    label=\"%s()\";\n", chunk->symbol);
    fprintf(out, "    style=filled;\n");
    fprintf(out, "    color=lightgrey;\n");
    fprintf(out, "    node [style=filled, color=white];\n");
    
    for (size_t i = 0; i < chunk->block_count; i++) {
        struct block* block = chunk->blocks[i];
        block_build_graph(chunk->symbol, block, out);
    }

    recursive_link(chunk->symbol, chunk->blocks[0], out);

    fprintf(out, "    }\n");
}

void ir_module_debug(struct ir_module* module) {
    for (int i = 0; i < module->count; i++)
    {
        printf("CHUNK [%s] ---\n", module->chunks[i]->symbol);
        ir_debug(module->chunks[i]);
        printf("\n");
    }
}

void ir_module_debug_graph(struct ir_module* module, FILE* out) {
    
    fprintf(out, "digraph \"SSA+CFG\" {\n");
    fprintf(out, "  node [shape=box, fontname=\"Maple Mono\"];\n");
    fprintf(out, "  compound=true;\n");
    
    for (int i = 0; i < module->count; i++)
    {
        ir_build_graph(module->chunks[i], out);
    }

    fprintf(out, "  }\n");
}

char* ir_compile(struct ir* chunk, FILE* out) {
    //TODO: compile to ARM64
}
