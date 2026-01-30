#include "ir_gen.h"

#include "block.h"

static void build_operand(struct ir* ir, struct operand operand) {
    switch (operand.type) {
        case OPERAND_TYPE_INTEGER: {
            ir_write(ir, IR_OP_IMMEDIATE_I64);
            ir_bytes(ir, (uint8_t*)&operand.value.integer, 8);
        }
        case OPERAND_TYPE_FLOAT: {
            ir_write(ir, IR_OP_IMMEDIATE_F64);
            ir_bytes(ir, (uint8_t*)&operand.value.floating, 8);
        }
        case OPERAND_TYPE_REGISTER: {
            ir_write(ir, IR_OP_REGISTER_I64);
            ir_write(ir, (uint8_t)operand.value.integer);
        }
        default: {
            fprintf(stderr, "[ir] unsupported operand type\n");
        }
    }
}

static void build_constant(struct ir* ir, int64_t value) {
    ir_write(ir, IR_OP_IMMEDIATE_I64);
    ir_bytes(ir, (uint8_t*)&value, 8);
}

static void build_block(struct ir* ir, struct block* block) {
    for (int i = 0; i < block->instructions_count; i++) {
        struct ssa_instruction inst = block->instructions[i];
        switch (inst.operator) {
            case OP_SALLOC: {
                ir_write(ir, IR_OP_SALLOC);
                build_constant(ir, inst.type.size);
            }
            default: {
                fprintf(stderr, "[ir] unsupported operand type\n");
            }
        }
    }
}

static void build_function(struct ir* ir, struct unit* unit) {
    ir_write(ir, IR_OP_GLOBAL);
    ir_write(ir, IR_OP_LABEL);
    ir_string(ir, unit->symbol);
    
    ir_write(ir, IR_OP_ENTER);
    
    for (int i = 0; i < unit->block_count; i++) {
        struct block* block = unit->blocks[i];
        build_block(ir, block);
    }
    
    ir_write(ir, IR_OP_LEAVE);
}

void ir_build(struct ir* ir, struct unit* unit) {
    switch (unit->type) {
        case UNIT_TYPE_FUNCTION: {
            build_function(ir, unit);
        }
        case UNIT_TYPE_VARIABLE: {
            return;
        }
    }
}