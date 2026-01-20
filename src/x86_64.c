#include "x86_64.h"

#include "block.h"
#include "chunk.h"

enum x86_64_ops {
    ASM_TEXT,
    ASM_DATA,
    ASM_GLOBAL,
    ASM_LABEL,
    ASM_CONSTANT,
    ASM_REGISTER,
    ASM_ADD,
    ASM_SUB,
    ASM_IMUL,
    ASM_IDIV,
};

enum x86_64_registers {
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,
    RBP,
    RSP,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15
};

static void compile_constant_register(struct chunk* chunk, enum x86_64_registers register_id) {
    chunk_write(chunk, ASM_REGISTER);
    chunk_write(chunk, register_id);
}

static void compile_operand(struct backend* backend, struct operand operand) {
    struct chunk* chunk = backend->out;
    switch (operand.type) {
        case OPERAND_TYPE_INTEGER: {
            chunk_write(chunk, ASM_CONSTANT);
            uint8_t buffer[8];
            *(uint64_t* )buffer = operand.value.integer;
            chunk_bytes(chunk, buffer, 8);
            break;
        }
        default:
            break;
    }
}

static void compile_instruction(struct backend* backend, struct ssa_instruction instruction) {
    struct chunk* chunk = backend->out;
    switch (instruction.operator) {
        case OP_SALLOC: {
            chunk_write(chunk, ASM_ADD);
            compile_constant_register(chunk, RSP);
            compile_operand(backend, instruction.operands[0]);
            break;
        }
        default:
            break;
    }
}

static void compile_block(struct backend* backend, struct block* block) {
    struct chunk* chunk = backend->out;
    chunk_write(chunk, ASM_LABEL);
    char label[8];
    sprintf(label, ".L%04d", block->id);
    chunk_string(chunk, label);
    for (int i = 0; i < block->instructions_count; i++) {
        struct ssa_instruction instruction = block->instructions[i];
        compile_instruction(backend, instruction);
    }
}

static int compile_function(struct backend* backend, struct unit* unit) {
    struct chunk* chunk = backend->out;
    chunk_write(chunk, ASM_TEXT);
    if (unit->global) {
        chunk_write(chunk, ASM_GLOBAL);
        chunk_string(chunk, unit->symbol);
    }
    chunk_write(chunk, ASM_LABEL);
    chunk_string(chunk, unit->symbol);

    for (int i = 0; i < unit->block_count; i++) {
        compile_block(backend, unit->blocks[i]);
    }
    return 0;
}

static int compile_variable(struct backend* backend, struct unit* unit) {
    return 0;
}

const struct backend x86_64_backend = {compile_function, compile_variable};