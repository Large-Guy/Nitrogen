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
            chunk_write(chunk, ASM_SUB);
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

static void debug_x86_64_arg(struct chunk* bytecode, int* i) {
    (*i)++;
    switch (bytecode->code[*i]) {
        case ASM_IMMEDIATE_64: {
            uint8_t* code = &bytecode->code[++*i];
            printf("%llu", *(int64_t*)code);
            *i += 8;
            break;
        }
        case ASM_IMMEDIATE_32: {
            uint8_t* code = &bytecode->code[++*i];
            printf("%d", *(int32_t*)code);
            *i += 4;
            break;
        }
        case ASM_IMMEDIATE_16: {
            uint8_t* code = &bytecode->code[++*i];
            printf("%d", *(int16_t*)code);
            *i += 2;
            break;
        }
        case ASM_IMMEDIATE_8: {
            uint8_t* code = &bytecode->code[++*i];
            printf("%d ", *(int8_t*)code);
            *i += 1;
            break;
        }
        case ASM_REGISTER_64: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case 0: {
                    printf("rax");
                    break;
                }
                case 1: {
                    printf("rbx");
                    break;
                }
                case 2: {
                    printf("rcx");
                    break;
                }
                case 3: {
                    printf("rdx");
                    break;
                }
                case 4: {
                    printf("rsi");
                    break;
                }
                case 5: {
                    printf("rdi");
                    break;
                }
                case 6: {
                    printf("rbp");
                    break;
                }
                case 7: {
                    printf("rsp");
                    break;
                }
                case 8: {
                    printf("r8");
                    break;
                }
                case 9: {
                    printf("r9");
                    break;
                }
                case 10: {
                    printf("r10");
                    break;
                }
                case 11: {
                    printf("r11");
                    break;
                }
                case 12: {
                    printf("r12");
                    break;
                }
                case 13: {
                    printf("r13");
                    break;
                }
                case 14: {
                    printf("r14");
                    break;
                }
                case 15: {
                    printf("r15");
                    break;
                }
                default: {
                    printf("invalid register ");
                    break;
                }
            }
            
            break;
        }
        case ASM_REGISTER_32: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case 0: {
                    printf("eax");
                    break;
                }
                case 1: {
                    printf("ebx");
                    break;
                }
                case 2: {
                    printf("ecx");
                    break;
                }
                case 3: {
                    printf("edx");
                    break;
                }
                case 4: {
                    printf("esi");
                    break;
                }
                case 5: {
                    printf("edi");
                    break;
                }
                case 6: {
                    printf("ebp");
                    break;
                }
                case 7: {
                    printf("esp");
                    break;
                }
                case 8: {
                    printf("r8d");
                    break;
                }
                case 9: {
                    printf("r9d");
                    break;
                }
                case 10: {
                    printf("r10d");
                    break;
                }
                case 11: {
                    printf("r11d");
                    break;
                }
                case 12: {
                    printf("r12d");
                    break;
                }
                case 13: {
                    printf("r13d");
                    break;
                }
                case 14: {
                    printf("r14d");
                    break;
                }
                case 15: {
                    printf("r15d");
                    break;
                }
                default: {
                    printf("invalid register");
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
                    printf("ax");
                    break;
                }
                case 1: {
                    printf("bx");
                    break;
                }
                case 2: {
                    printf("cx");
                    break;
                }
                case 3: {
                    printf("dx");
                    break;
                }
                case 4: {
                    printf("si");
                    break;
                }
                case 5: {
                    printf("di");
                    break;
                }
                case 6: {
                    printf("bp");
                    break;
                }
                case 7: {
                    printf("sp");
                    break;
                }
                case 8: {
                    printf("r8w");
                    break;
                }
                case 9: {
                    printf("r9w");
                    break;
                }
                case 10: {
                    printf("r10w");
                    break;
                }
                case 11: {
                    printf("r11w");
                    break;
                }
                case 12: {
                    printf("r12w");
                    break;
                }
                case 13: {
                    printf("r13w");
                    break;
                }
                case 14: {
                    printf("r14w");
                    break;
                }
                case 15: {
                    printf("r15w");
                    break;
                }
                default: {
                    printf("invalid register");
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
                    printf("al");
                    break;
                }
                case 1: {
                    printf("bl");
                    break;
                }
                case 2: {
                    printf("cl");
                    break;
                }
                case 3: {
                    printf("dl");
                    break;
                }
                case 4: {
                    printf("sil");
                    break;
                }
                case 5: {
                    printf("dil");
                    break;
                }
                case 6: {
                    printf("bpl");
                    break;
                }
                case 7: {
                    printf("spl");
                    break;
                }
                case 8: {
                    printf("r8b");
                    break;
                }
                case 9: {
                    printf("r9b");
                    break;
                }
                case 10: {
                    printf("r10b");
                    break;
                }
                case 11: {
                    printf("r11b");
                    break;
                }
                case 12: {
                    printf("r12b");
                    break;
                }
                case 13: {
                    printf("r13b");
                    break;
                }
                case 14: {
                    printf("r14b");
                    break;
                }
                case 15: {
                    printf("r15b");
                    break;
                }
                default: {
                    printf("invalid register");
                    break;
                }
            }
            (*i)++;
            break;
        }
    }
}

static void debug_x86_64_label(struct chunk* bytecode, int* i) {
    (*i)++;
    while (bytecode->code[*i] != 0) {
        printf("%c", bytecode->code[(*i)++]);
    }
}

void debug_x86_64_bytecode(struct chunk* bytecode) {
    for (int i = 0; i < bytecode->count; i++) {
        switch (bytecode->code[i]) {
            case ASM_TEXT: {
                printf("section .text\n");
                break;
            }
            case ASM_DATA: {
                printf("section .data\n");
                break;
            }
            case ASM_GLOBAL:
                printf("global ");
                debug_x86_64_label(bytecode, &i);
                printf("\n");
                break;
            case ASM_LABEL:
                debug_x86_64_label(bytecode, &i);
                printf(":\n");
                break;
            case ASM_ADD:
                printf("add ");
                debug_x86_64_arg(bytecode, &i);
                printf(", ");
                debug_x86_64_arg(bytecode, &i);
                printf("\n");
                break;
            case ASM_SUB:
                printf("sub ");
                debug_x86_64_arg(bytecode, &i);
                printf(", ");
                debug_x86_64_arg(bytecode, &i);
                printf("\n");
                break;
            case ASM_IMUL:
                printf("imul ");
                debug_x86_64_arg(bytecode, &i);
                printf(", ");
                debug_x86_64_arg(bytecode, &i);
                printf("\n");
                break;
            case ASM_IDIV:
                printf("idiv ");
                debug_x86_64_arg(bytecode, &i);
                printf(", ");
                debug_x86_64_arg(bytecode, &i);
                printf("\n");
                break;
        }
    }
}
