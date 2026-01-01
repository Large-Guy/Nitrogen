#include "unit_debug.h"

#include <assert.h>

#include "ast.h"
#include "block.h"


static void ast_node_type_debug(FILE* out, struct ast_node* node)
{
    if (node == NULL)
    {
        return;
    }
    switch (node->type)
    {
        case AST_NODE_TYPE_VOID:
            fprintf(out, "void");
            break;
        case AST_NODE_TYPE_BOOL:
            fprintf(out, "bool");
            break;
        case AST_NODE_TYPE_U8:
            fprintf(out, "u8");
            break;
        case AST_NODE_TYPE_U16:
            fprintf(out, "u16");
            break;
        case AST_NODE_TYPE_U32:
            fprintf(out, "u32");
            break;
        case AST_NODE_TYPE_U64:
            fprintf(out, "u64");
            break;
        case AST_NODE_TYPE_I8:
            fprintf(out, "i8");
            break;
        case AST_NODE_TYPE_I16:
            fprintf(out, "i16");
            break;
        case AST_NODE_TYPE_I32:
            fprintf(out, "i32");
            break;
        case AST_NODE_TYPE_I64:
            fprintf(out, "i64");
            break;
        case AST_NODE_TYPE_F32:
            fprintf(out, "f32");
            break;
        case AST_NODE_TYPE_F64:
            fprintf(out, "f64");
            break;
        case AST_NODE_TYPE_REFERENCE:
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, "*");
            break;
        case AST_NODE_TYPE_POINTER:
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, "*?");
            break;
        case AST_NODE_TYPE_ARRAY:
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, "[]");
            break;
        case AST_NODE_TYPE_SIMD:
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, "<%.*s", (int)node->children[1]->token.length, node->children[1]->token.start);
            fprintf(out, ">");
            break;
        default:
            fprintf(out, "unknown");
            break;
    }
}

static void type_code_name(FILE* out, struct ssa_type code)
{
    ast_node_type_debug(out, code.type);
}

static char* operator_name(enum ssa_instruction_code code)
{
    switch (code)
    {
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
        case OP_BITWISE_LEFT:
            return "bitwise-left";
        case OP_BITWISE_RIGHT:
            return "bitwise-right";
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
        case OP_CAST:
            return "cast";
        default:
            return "unsupported";
    }
}

static void operand_debug(FILE* out, struct operand operand)
{
    switch (operand.type)
    {
        case OPERAND_TYPE_END:
            return;
        case OPERAND_TYPE_INTEGER:
            fprintf(out, "[const #%llu: ", operand.value.integer);
            break;
        case OPERAND_TYPE_FLOAT:
            fprintf(out, "[const #%ff: ", operand.value.floating);
            break;
        case OPERAND_TYPE_REGISTER:
            fprintf(out, "[");
            fprintf(out, "reg ");

            fprintf(out, "%%%lu: ", operand.value.integer);
            break;
        case OPERAND_TYPE_BLOCK:
            fprintf(out, "[block &%d", operand.value.block->id);
            break;
        case OPERAND_TYPE_NONE:
            return;
        case OPERAND_TYPE_IR:
            fprintf(out, "[func @%s", operand.value.unit->symbol);
            break;
    }
    type_code_name(out, operand.typename);
    fprintf(out, "] ");
}

static void instruction_debug(FILE* out, struct ssa_instruction instruction)
{
    operand_debug(out, instruction.result);

    if (instruction.result.type > OPERAND_TYPE_END)
        fprintf(out, "= ");

    fprintf(out, "%s ", operator_name(instruction.operator));

    for (int i = 0; i < MAX_OPERANDS; i++)
        operand_debug(out, instruction.operands[i]);

    type_code_name(out, instruction.type);
}

static void block_debug(struct block* block)
{
    if (!block->entry)
    {
        printf("dominated by: ");
        for (int i = 0; i < block->parents_count; i++)
        {
            struct block* parent = block->parents[i];
            printf("BLOCK [%d] ", parent->id);
        }
        printf("---> ");
    }
    printf("BLOCK [%d] ---\n", block->id);
    for (int i = 0; i < block->instructions_count; i++)
    {
        instruction_debug(stdout, block->instructions[i]);
        printf("\n");
    }
    if (block->children_count > 0)
    {
        printf("dominates: ");
        for (int i = 0; i < block->children_count; i++)
        {
            struct block* child = block->children[i];
            printf("BLOCK [%d] ", child->id);
        }
        printf("\n");
    }
    printf("\n");
}

void unit_debug(struct unit* chunk)
{
    assert(chunk != NULL);
    assert(chunk->blocks != NULL);
    for (size_t i = 0; i < chunk->block_count; i++)
    {
        block_debug(chunk->blocks[i]);
    }
}

static void block_build_graph(char* name, struct block* block, FILE* out)
{
    fprintf(out, "  %s_bb%d [label=\"", name, block->id);
    if (block->entry)
        fprintf(out, ".ENTRY");
    else if (block->children_count == 0)
        fprintf(out, ".EXIT");
    else
        fprintf(out, ".BLOCK %d", block->id);
    fprintf(out, "\\l");
    for (int i = 0; i < block->instructions_count; i++)
    {
        instruction_debug(out, block->instructions[i]);
        fprintf(out, "\\l");
    }
    fprintf(out, "\"];\n");
}

static void recursive_link(char* name, struct block* block, FILE* out)
{
    for (int i = 0; i < block->children_count; i++)
    {
        struct block* child = block->children[i];
        fprintf(out, "    %s_bb%d -> %s_bb%d [color=black];\n", name, block->id, name, child->id);
        if (child->parents_count == 1 || child->parents[0] == block)
            recursive_link(name, child, out);
    }
}

void unit_build_graph(struct unit* chunk, FILE* out)
{
    assert(chunk != NULL);
    assert(chunk->blocks != NULL);

    fprintf(out, "  subgraph cluster_%s {\n", chunk->symbol);
    fprintf(out, "    label=\"%s(", chunk->symbol);
    for (int i = 0; i < chunk->argument_count; i++)
    {
        if (i > 0)
        {
            fprintf(out, ", ");
        }
        operand_debug(out, chunk->arguments[i]);
    }
    fprintf(out, ")\";\n");
    fprintf(out, "    style=filled;\n");
    fprintf(out, "    color=lightgrey;\n");
    fprintf(out, "    node [style=filled, color=white];\n");

    for (size_t i = 0; i < chunk->block_count; i++)
    {
        struct block* block = chunk->blocks[i];
        block_build_graph(chunk->symbol, block, out);
    }

    recursive_link(chunk->symbol, chunk->blocks[0], out);

    fprintf(out, "    }\n");
}

void unit_module_debug(struct unit_module* module)
{
    for (int i = 0; i < module->unit_count; i++)
    {
        printf("CHUNK [%s] ---\n", module->units[i]->symbol);
        unit_debug(module->units[i]);
        printf("\n");
    }
}

void unit_module_debug_graph(struct unit_module* module, FILE* out)
{
    fprintf(out, "digraph \"SSA+CFG\" {\n");
    fprintf(out, "  node [shape=box, fontname=\"Maple Mono\"];\n");
    fprintf(out, "  compound=true;\n");

    for (int i = 0; i < module->unit_count; i++)
    {
        unit_build_graph(module->units[i], out);
    }

    fprintf(out, "  }\n");
}