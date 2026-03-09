#include "x86_64.h"

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "block.h"
#include "ir.h"

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
    ASM_ADDRESS,
    ASM_ADD,
    ASM_SUB,
    ASM_IMUL,
    ASM_IDIV,
    ASM_MOV,
    ASM_PUSH,
    ASM_POP,
    ASM_LEAVE,
    ASM_RETURN,
    ASM_BYTE,
    ASM_SHORT,
    ASM_INT,
    ASM_LONG,
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

struct register_offset {
    uint32_t reg;
    uint32_t offset;
};

struct stack_usage {
    struct register_offset* offsets;
    uint32_t capacity;
    uint32_t count;
    uint32_t size;
};

static struct stack_usage* stack_usage_new() {
    struct stack_usage* us = malloc(sizeof(struct stack_usage));
    us->capacity = 1;
    us->count = 0;
    us->offsets = malloc(sizeof(struct register_offset) * us->capacity);
    us->size = 0;
    return us;
}

static void stack_usage_free(struct stack_usage* us) {
    free(us->offsets);
    free(us);
}

static struct register_offset* stack_usage_get(struct stack_usage* usage, uint32_t reg) {
    for (uint32_t i = 0; i < usage->count; i++) {
        if (usage->offsets[i].reg == reg) {
            return &usage->offsets[i];
        }
    }
    return NULL;
}

static void stack_usage_push(struct stack_usage* usage, uint32_t reg, uint32_t size) {
    struct register_offset offset = { reg, usage->size };
    if (usage->count >= usage->capacity) {
        usage->capacity *= 2;
        usage->offsets = realloc(usage->offsets, usage->capacity * sizeof(struct register_offset));
        assert(usage->offsets);
    }
    usage->offsets[usage->count++] = offset;
    usage->size += size;
}

static void compile_constant_register(struct ir* chunk, enum x86_64_registers register_id) {
    ir_write(chunk, ASM_REGISTER_64);
    ir_write(chunk, register_id);
}

static void compile_operand(struct backend* backend, struct operand operand) {
    struct ir* chunk = backend->out;
    switch (operand.type) {
        case OPERAND_TYPE_INTEGER: {
            ir_write(chunk, ASM_IMMEDIATE_64);
            uint8_t buffer[8] = {};
            *(uint64_t* )buffer = operand.value.integer;
            ir_bytes(chunk, buffer, 8);
            break;
        }
        default:
            break;
    }
}

static int error(int condition, const char* message) {
    if (!condition) {
        printf("%s", message);
    }
    return !condition;
}

uint64_t stack_align(uint64_t size) {
    return (uint64_t)((double)(int)((double)size / 8.0 + 0.5) * 8.0);
}

static void compile_instruction(struct stack_usage* usage, struct backend* backend, struct ssa_instruction instruction) {
    struct ir* chunk = backend->out;
    switch (instruction.operator) {
        case OP_SALLOC: {
            ir_write(chunk, ASM_SUB);
            compile_constant_register(chunk, RSP);
            if (error(instruction.operands[0].type == OPERAND_TYPE_INTEGER, "unexpected operand type"))
                return;
            
            if (error(instruction.result.type == OPERAND_TYPE_REGISTER, "unexpected operand type"))
                return;
            
            ir_write(chunk, ASM_IMMEDIATE_64);
            uint8_t buffer[8] = {};
            *(uint64_t* )buffer = stack_align(instruction.operands[0].value.integer);
            ir_bytes(chunk, buffer, 8);
            stack_usage_push(usage, (uint32_t)instruction.result.value.integer, stack_align(instruction.operands[0].value.integer));
            break;
        }
        case OP_ZERO: {
            if (error(instruction.operands[0].type == OPERAND_TYPE_REGISTER, "unexpected operand type"))
                return;
            struct register_offset* offset = stack_usage_get(usage, instruction.operands[0].value.integer);
            if (error(offset != NULL, "expected stack address register"))
                return;
            if (error(instruction.operands[1].type == OPERAND_TYPE_INTEGER, "unexpected operand type"))
                return;
            uint64_t value = instruction.operands[1].value.integer;
            if (error(instruction.operands[2].type == OPERAND_TYPE_INTEGER, "unexpected operand type"))
                return;
            uint64_t size = instruction.operands[2].value.integer;
            
            for (int i = 0; i < size; i++) {
                ir_write(chunk, ASM_MOV);
                
                ir_write(chunk, ASM_ADDRESS);
                
                ir_write(chunk, ASM_BYTE);
                
                ir_write(chunk, ASM_REGISTER_64);
                ir_write(chunk, RBP);
                ir_write(chunk, ASM_IMMEDIATE_64);
                uint8_t buffer[8] = {};
                *(uint64_t* )buffer = i + offset->offset;
                ir_bytes(chunk, buffer, 8);
                
                ir_write(chunk, ASM_IMMEDIATE_64);
                *(uint64_t* )buffer = value;
                ir_bytes(chunk, buffer, 8);
            }
            // else we have to break if up into parts
            
            //chunk_write(chunk, ASM_MOV);
            // NOTE: okay, so I'm pretty sure once a variable is allocated in the SSA, it's offset NEVER changes, this 
            // should mean that we can calculate stack offsets for these registers in specific, and avoid having to
            // actually store memory addresses in registers, and instead just calculate what their offsets will be.
            
            // Implementation:
            // int[int] registerOffsets; // SSA register key -> integer offset
            // in SALLOC:
            //  registerOffsets.add(ssaRegister, stackSize);
            // in STORE
            //  if(registerOffsets.has(ssaRegister))
            //      ... calculate offset, output x86_64 byte code for offset and MOV
            //  else
            //      ... we worry about it later
            break;
        }
        default:
            break;
    }
}

static void compile_block(struct stack_usage* usage, struct backend* backend, struct block* block) {
    struct ir* chunk = backend->out;
    ir_write(chunk, ASM_LABEL);
    char label[8] = {};
    sprintf(label, ".L%04d", block->id);
    ir_string(chunk, label);
    for (int i = 0; i < block->instructions_count; i++) {
        struct ssa_instruction instruction = block->instructions[i];
        compile_instruction(usage, backend, instruction);
    }
}

static int compile_function(struct backend* backend, struct unit* unit) {
    struct stack_usage* usage = stack_usage_new();
    struct ir* chunk = backend->out;
    ir_write(chunk, ASM_TEXT);
    if (unit->global) {
        ir_write(chunk, ASM_GLOBAL);
        ir_string(chunk, unit->symbol);
    }
    ir_write(chunk, ASM_LABEL);
    ir_string(chunk, unit->symbol);
    
    // save caller's base point
    ir_write(chunk, ASM_PUSH);
    
    ir_write(chunk, ASM_REGISTER_64);
    ir_write(chunk, RBP);
    
    // set our anchor to the current top
    ir_write(chunk, ASM_MOV);
    
    ir_write(chunk, ASM_REGISTER_64);
    ir_write(chunk, RBP);
    
    ir_write(chunk, ASM_REGISTER_64);
    ir_write(chunk, RSP);

    for (int i = 0; i < unit->block_count; i++) {
        compile_block(usage, backend, unit->blocks[i]);
    }
    
    ir_write(chunk, ASM_LEAVE);
    ir_write(chunk, ASM_RETURN);
    
    stack_usage_free(usage);
    return 0;
}

static int compile_variable(struct backend* backend, struct unit* unit) {
    return 0;
}

const struct backend x86_64_backend = {compile_function, compile_variable};

static void debug_x86_64_arg(FILE* out, struct ir* bytecode, int* i) {
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
            break;
        }
        case ASM_ADDRESS: {
            (*i)++;
            switch (bytecode->code[*i]) {
                case ASM_BYTE:
                    fprintf(out, "byte ");
                    break;
                case ASM_SHORT:
                    fprintf(out, "word ");
                    break;
                case ASM_INT:
                    fprintf(out, "int ");
                    break;
                case ASM_LONG:
                    fprintf(out, "qword ");
                    break;
                default:
                    fprintf(out, "invalid size");
            }
            fprintf(out, "[");
            debug_x86_64_arg(out, bytecode, i);
            int64_t offset = *(int64_t*)(&bytecode->code[*i + 2]);
            if (offset > 0) {
                fprintf(out, " + ");
                debug_x86_64_arg(out, bytecode, i);
            }
            else if (offset < 0) {
                fprintf(out, " - ");
                debug_x86_64_arg(out, bytecode, i);
            }
            else {
                *i += 9;
            }
            
            fprintf(out, "]");
            break;
        }
    }
}

static void debug_x86_64_label(FILE* out, struct ir* bytecode, int* i) {
    (*i)++;
    while (bytecode->code[*i] != 0) {
        fprintf(out, "%c", bytecode->code[(*i)++]);
    }
}

void debug_x86_64_bytecode(FILE* out, struct ir* bytecode) {
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
            case ASM_GLOBAL: {
                fprintf(out, "global ");
                debug_x86_64_label(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_LABEL: {
                debug_x86_64_label(out, bytecode, &i);
                fprintf(out, ":\n");
                break;
            }
            case ASM_ADD: {
                fprintf(out, "add ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_SUB: {
                fprintf(out, "sub ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_IMUL: {
                fprintf(out, "imul ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_IDIV: {
                fprintf(out, "idiv ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_MOV: {
                fprintf(out, "mov ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, ", ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_PUSH: {
                fprintf(out, "push ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_POP: {
                fprintf(out, "pop ");
                debug_x86_64_arg(out, bytecode, &i);
                fprintf(out, "\n");
                break;
            }
            case ASM_LEAVE: {
                fprintf(out, "leave\n");
                break;
            }
            case ASM_RETURN: {
                fprintf(out, "ret\n");
                break;
            }
            default: {
                fprintf(out, "invalid opcode: %d\n", bytecode->code[i]);
            }
        }
    }
}
