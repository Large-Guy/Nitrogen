#include "x86_64.h"

#include "block.h"

static void compile_operand(struct backend* backend, struct operand operand) {
    switch (operand.type) {
        case OPERAND_TYPE_INTEGER:
            break;
    }
}

static void compile_block(struct backend* backend, struct block* block) {
    fprintf(backend->out, ".L%04d:\n", block->id);
    for (int i = 0; i < block->instructions_count; i++) {
        struct ssa_instruction instruction = block->instructions[i];
        switch (instruction.operator) {
            case OP_SALLOC: {
                break;
            }
        }
    }
}

static int compile_function(struct backend* backend, struct unit* unit) {
    fprintf(backend->out, "section .text\n");
    if (unit->global) {
        fprintf(backend->out, "  global %s\n", unit->symbol);
    }
    fprintf(backend->out, "\n%s:\n", unit->symbol);

    for (int i = 0; i < unit->block_count; i++) {
        struct block* block = unit->blocks[i];
        compile_block(backend, block);
    }

    fprintf(backend->out, "  ret\n");
    return 0;
}

static int compile_variable(struct backend* backend, struct unit* unit) {
    fprintf(backend->out, "section .data\n");
    fprintf(backend->out, "%s", unit->symbol);
}

const struct backend x86_64_backend = {compile_function, compile_variable};