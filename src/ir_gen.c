#include "ir_gen.h"

#include "block.h"

static void build_operand(struct ir* ir, struct operand operand) {
    switch (operand.type) {
        case OPERAND_TYPE_INTEGER: {
            ir_write(ir, IR_OP_IMMEDIATE_I64);
            ir_bytes(ir, (uint8_t*)&operand.value.integer, 8);
            break;
        }
        case OPERAND_TYPE_FLOAT: {
            ir_write(ir, IR_OP_IMMEDIATE_F64);
            ir_bytes(ir, (uint8_t*)&operand.value.floating, 8);
            break;
        }
        case OPERAND_TYPE_REGISTER: {
            ir_write(ir, IR_OP_REGISTER_I64);
            ir_write(ir, (uint8_t)operand.value.integer);
            break;
        }
        case OPERAND_TYPE_BLOCK: {
            ir_write(ir, IR_OP_LABEL);
            char buffer[7] = {};
            sprintf(buffer, ".L%.4d", (int16_t)operand.value.integer);
            ir_string(ir, buffer);
            break;
        }
        default: {
            fprintf(stderr, "[ir] unsupported operand type\n");
            break;
        }
    }
}

static void consume_operand(struct ir* ir, struct operand operand, enum operand_type expected) {
    if (operand.type != expected) {
        fprintf(stderr, "[ir] unsupported operand type\n");
        return;
    }
    build_operand(ir, operand);
}

static bool match_operand(struct ir* ir, struct operand operand, enum operand_type expected) {
    if (operand.type != expected) {
        return false;
    }
    return true;
}

static void build_constant(struct ir* ir, int64_t value) {
    ir_write(ir, IR_OP_IMMEDIATE_I64);
    ir_bytes(ir, (uint8_t*)&value, 8);
}

static void build_register(struct ir* ir, enum ir_reg value) {
    ir_write(ir, IR_OP_REGISTER_I64);
    ir_write(ir, (uint8_t)value);
}

static void build_block(struct ir* ir, struct block* block) {
    ir_write(ir, IR_OP_LABEL);
    char buffer[7] = {};
    sprintf(buffer, ".L%.4d", (int16_t)block->id);
    ir_string(ir, buffer);
    
    for (int i = 0; i < block->instructions_count; i++) {
        struct ssa_instruction inst = block->instructions[i];
        switch (inst.operator) {
            case OP_SALLOC: {
                ir_write(ir, IR_OP_SALLOC);
                build_constant(ir, inst.type.size);
                break;
            }
            case OP_STORE: {
                ir_write(ir, IR_OP_MOV);
                
                if (!match_operand(ir, inst.operands[0], OPERAND_TYPE_REGISTER)) {
                    fprintf(stderr, "[ir] unexpected operand type\n");
                    return;
                }
                
                ir_write(ir, IR_OP_ADDRESS);
                build_constant(ir, inst.type.size);
                ir_write(ir, IR_OP_REGISTER_I64);
                ir_write(ir, IR_REG_RBP);
                ir_write(ir, IR_OP_IMMEDIATE_I64);
                ir_bytes(ir, (uint8_t*)&inst.operands[0].offset, 8);
                
                build_operand(ir, inst.operands[1]);
                
                break;
            }
            case OP_LOAD: {
                break;
            }
            case OP_RETURN: {
                int j = 0;
                while (inst.operands[j].type != OPERAND_TYPE_NONE) {
                    ir_write(ir, IR_OP_MOV);
                    build_register(ir, IR_REG_RET0 + j);
                    build_operand(ir, inst.operands[j]);
                    j++;
                }
                return;
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
    ir_write(ir, IR_OP_RETURN);
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