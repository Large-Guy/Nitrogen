#include "x86_64.h"

#include "block.h"
#include "chunk.h"

enum x86_64_ops {
    ASM_TEXT,
    ASM_DATA,
    ASM_GLOBAL,
    ASM_LABEL,
    ASM_IMMEDIATE_64,
    ASM_IMMEDIATE_32,
    ASM_IMMEDIATE_16,
    ASM_IMMEDIATE_8,
    ASM_REGISTER_64,
    ASM_REGISTER_32,
    ASM_REGISTER_16,
    ASM_REGISTER_8,
    ASM_ADD,
    ASM_SUB,
    ASM_IMUL,
    ASM_IDIV,
    ASM_MOV,
};

enum x86_64_registers {
    
    // return
    RAX,
    
    // not clobbered
    RBX,
    
    // clobbered
    RCX,
    RDX,
    RSI,
    RDI,
    
    // stack, not clobbered
    RBP,
    RSP,
    
    R8,
    R9,
    R10,
    R11,
    
    // not clobbered
    R12,
    R13,
    R14,
    R15
};

static void compile_constant_register(struct chunk* chunk, enum x86_64_registers register_id) {
    chunk_write(chunk, ASM_REGISTER_64);
    chunk_write(chunk, register_id);
}

static void compile_operand(struct backend* backend, struct operand operand) {
    struct chunk* chunk = backend->out;
    switch (operand.type) {
        case OPERAND_TYPE_INTEGER: {
            chunk_write(chunk, ASM_IMMEDIATE_64);
            uint8_t buffer[8] = {};
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
            chunk_write(chunk, ASM_SUB);
            compile_constant_register(chunk, RSP);
            compile_operand(backend, instruction.operands[0]);
            break;
        }
        case OP_STORE: {
            
        }
        default:
            break;
    }
}

static void compile_block(struct backend* backend, struct block* block) {
    struct chunk* chunk = backend->out;
    chunk_write(chunk, ASM_LABEL);
    char label[8] = {};
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

static void debug_x86_64_arg(FILE* out, struct chunk* bytecode, int* i) {
    (*i)++;
    switch (bytecode->code[*i]) {
        case ASM_IMMEDIATE_64: {
            uint8_t* code = &bytecode->code[++*i];
            fprintf(out, "%llu", *(int64_t*)code);
            *i += 7;
            break;
        }
        case ASM_IMMEDIATE_32: {
            uint8_t* code = &bytecode->code[++*i];
            fprintf(out, "%d", *(int32_t*)code);
            *i += 3;
            break;
        }
        case ASM_IMMEDIATE_16: {
            uint8_t* code = &bytecode->code[++*i];
            fprintf(out, "%d", *(int16_t*)code);
            *i += 1;
            break;
        }
        case ASM_IMMEDIATE_8: {
            uint8_t* code = &bytecode->code[++*i];
            fprintf(out, "%d ", *(int8_t*)code);
            break;
        }
        case ASM_REGISTER_64: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case 0: {
                    fprintf(out, "rax");
                    break;
                }
                case 1: {
                    fprintf(out, "rbx");
                    break;
                }
                case 2: {
                    fprintf(out, "rcx");
                    break;
                }
                case 3: {
                    fprintf(out, "rdx");
                    break;
                }
                case 4: {
                    fprintf(out, "rsi");
                    break;
                }
                case 5: {
                    fprintf(out, "rdi");
                    break;
                }
                case 6: {
                    fprintf(out, "rbp");
                    break;
                }
                case 7: {
                    fprintf(out, "rsp");
                    break;
                }
                case 8: {
                    fprintf(out, "r8");
                    break;
                }
                case 9: {
                    fprintf(out, "r9");
                    break;
                }
                case 10: {
                    fprintf(out, "r10");
                    break;
                }
                case 11: {
                    fprintf(out, "r11");
                    break;
                }
                case 12: {
                    fprintf(out, "r12");
                    break;
                }
                case 13: {
                    fprintf(out, "r13");
                    break;
                }
                case 14: {
                    fprintf(out, "r14");
                    break;
                }
                case 15: {
                    fprintf(out, "r15");
                    break;
                }
                default: {
                    fprintf(out, "invalid register ");
                    break;
                }
            }
            
            break;
        }
        case ASM_REGISTER_32: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case 0: {
                    fprintf(out, "eax");
                    break;
                }
                case 1: {
                    fprintf(out, "ebx");
                    break;
                }
                case 2: {
                    fprintf(out, "ecx");
                    break;
                }
                case 3: {
                    fprintf(out, "edx");
                    break;
                }
                case 4: {
                    fprintf(out, "esi");
                    break;
                }
                case 5: {
                    fprintf(out, "edi");
                    break;
                }
                case 6: {
                    fprintf(out, "ebp");
                    break;
                }
                case 7: {
                    fprintf(out, "esp");
                    break;
                }
                case 8: {
                    fprintf(out, "r8d");
                    break;
                }
                case 9: {
                    fprintf(out, "r9d");
                    break;
                }
                case 10: {
                    fprintf(out, "r10d");
                    break;
                }
                case 11: {
                    fprintf(out, "r11d");
                    break;
                }
                case 12: {
                    fprintf(out, "r12d");
                    break;
                }
                case 13: {
                    fprintf(out, "r13d");
                    break;
                }
                case 14: {
                    fprintf(out, "r14d");
                    break;
                }
                case 15: {
                    fprintf(out, "r15d");
                    break;
                }
                default: {
                    fprintf(out, "invalid register");
                    break;
                }
            }
            
            (*i)++;
            break;
        }
        case ASM_REGISTER_16: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case 0: {
                    fprintf(out, "ax");
                    break;
                }
                case 1: {
                    fprintf(out, "bx");
                    break;
                }
                case 2: {
                    fprintf(out, "cx");
                    break;
                }
                case 3: {
                    fprintf(out, "dx");
                    break;
                }
                case 4: {
                    fprintf(out, "si");
                    break;
                }
                case 5: {
                    fprintf(out, "di");
                    break;
                }
                case 6: {
                    fprintf(out, "bp");
                    break;
                }
                case 7: {
                    fprintf(out, "sp");
                    break;
                }
                case 8: {
                    fprintf(out, "r8w");
                    break;
                }
                case 9: {
                    fprintf(out, "r9w");
                    break;
                }
                case 10: {
                    fprintf(out, "r10w");
                    break;
                }
                case 11: {
                    fprintf(out, "r11w");
                    break;
                }
                case 12: {
                    fprintf(out, "r12w");
                    break;
                }
                case 13: {
                    fprintf(out, "r13w");
                    break;
                }
                case 14: {
                    fprintf(out, "r14w");
                    break;
                }
                case 15: {
                    fprintf(out, "r15w");
                    break;
                }
                default: {
                    fprintf(out, "invalid register");
                    break;
                }
            }
            
            (*i)++;
            break;
        }
        case ASM_REGISTER_8: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case 0: {
                    fprintf(out, "al");
                    break;
                }
                case 1: {
                    fprintf(out, "bl");
                    break;
                }
                case 2: {
                    fprintf(out, "cl");
                    break;
                }
                case 3: {
                    fprintf(out, "dl");
                    break;
                }
                case 4: {
                    fprintf(out, "sil");
                    break;
                }
                case 5: {
                    fprintf(out, "dil");
                    break;
                }
                case 6: {
                    fprintf(out, "bpl");
                    break;
                }
                case 7: {
                    fprintf(out, "spl");
                    break;
                }
                case 8: {
                    fprintf(out, "r8b");
                    break;
                }
                case 9: {
                    fprintf(out, "r9b");
                    break;
                }
                case 10: {
                    fprintf(out, "r10b");
                    break;
                }
                case 11: {
                    fprintf(out, "r11b");
                    break;
                }
                case 12: {
                    fprintf(out, "r12b");
                    break;
                }
                case 13: {
                    fprintf(out, "r13b");
                    break;
                }
                case 14: {
                    fprintf(out, "r14b");
                    break;
                }
                case 15: {
                    fprintf(out, "r15b");
                    break;
                }
                default: {
                    fprintf(out, "invalid register");
                    break;
                }
            }
            (*i)++;
            break;
        }
    }
}

static void debug_x86_64_label(FILE* out, struct chunk* bytecode, int* i) {
    (*i)++;
    while (bytecode->code[*i] != 0) {
        fprintf(out, "%c", bytecode->code[(*i)++]);
    }
}

void debug_x86_64_bytecode(FILE* out, struct chunk* bytecode) {
    for (int i = 0; i < bytecode->count; i++) {
        switch (bytecode->code[i]) {
            case ASM_TEXT: {
                fprintf(out, "section .text\n");
                break;
            }
            case ASM_DATA: {
                fprintf(out, "section .data\n");
                break;
            }
            case ASM_GLOBAL:
                fprintf(out, "global ");
                debug_x86_64_label(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            case ASM_LABEL:
                debug_x86_64_label(out, bytecode, &i);
                fprintf(out, ":\n");
                break;
            case ASM_ADD:
                fprintf(out, "add ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            case ASM_SUB:
                fprintf(out, "sub ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            case ASM_IMUL:
                fprintf(out, "imul ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            case ASM_IDIV:
                fprintf(out, "idiv ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
        }
    }
}
