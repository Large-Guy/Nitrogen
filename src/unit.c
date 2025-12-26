#include "unit.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "block.h"

struct unit* unit_new(char* symbol, bool global, enum unit_type type)
{
    struct unit* chunk = malloc(sizeof(struct unit));
    assert(chunk);

    chunk->arguments = malloc(sizeof(struct operand));
    assert(chunk->arguments);
    chunk->argument_count = 0;
    chunk->argument_capacity = 1;

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

struct unit_module* unit_module_new(char* name)
{
    struct unit_module* list = malloc(sizeof(struct unit_module));
    assert(list);
    list->name = malloc(strlen(name) + 1);
    memcpy(list->name, name, strlen(name) + 1);
    list->units = malloc(sizeof(struct unit*));
    list->unit_count = 0;
    list->unit_capacity = 1;
    return list;
}

void unit_module_free(struct unit_module* list)
{
    assert(list);
    free(list->name);
    for (int i = 0; i < list->unit_count; i++)
    {
        unit_free(list->units[i]);
    }
    free(list->units);
    free(list);
}

void unit_module_append(struct unit_module* list, struct unit* chunk)
{
    if (list->unit_count >= list->unit_capacity)
    {
        list->unit_capacity *= 2;
        list->units = realloc(list->units, list->unit_capacity * sizeof(struct unit*));
        assert(list->units);
    }
    list->units[list->unit_count++] = chunk;
}

void unit_free(struct unit* chunk)
{
    assert(chunk != NULL);
    free(chunk->symbol);
    for (int i = 0; i < chunk->block_count; i++)
    {
        block_free(chunk->blocks[i]);
    }
    free(chunk->blocks);
    free(chunk);
}

struct unit* unit_module_find(struct unit_module* module, struct token symbol)
{
    for (int i = 0; i < module->unit_count; i++)
    {
        struct unit* chunk = module->units[i];
        if (symbol.length == strlen(chunk->symbol) && memcmp(chunk->symbol, symbol.start, symbol.length) == 0)
        {
            return module->units[i];
        }
    }
    return NULL;
}

void unit_add(struct unit* chunk, struct block* block)
{
    assert(chunk != NULL);
    if (chunk->block_count >= chunk->block_capacity)
    {
        chunk->block_capacity *= 2;
        chunk->blocks = realloc(chunk->blocks, chunk->block_capacity * sizeof(struct ssa_instruction));
        assert(chunk->blocks);
    }
    chunk->blocks[chunk->block_count++] = block;
    block->id = chunk->block_count;
}

void unit_arg(struct unit* chunk, struct operand arg)
{
    assert(chunk != NULL);
    if (chunk->argument_count >= chunk->argument_capacity)
    {
        chunk->argument_capacity *= 2;
        chunk->arguments = realloc(chunk->arguments, chunk->argument_capacity * sizeof(struct operand));
        assert(chunk->arguments);
    }
    chunk->arguments[chunk->argument_count++] = arg;
}

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
            fprintf(out, "ref<");
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, ">");
            break;
        case AST_NODE_TYPE_POINTER:
            fprintf(out, "ptr<");
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, ">");
            break;
        case AST_NODE_TYPE_ARRAY:
            fprintf(out, "array<");
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, ">");
            break;
        case AST_NODE_TYPE_SIMD:
            fprintf(out, "simd<");
            ast_node_type_debug(out, node->children[0]);
            fprintf(out, ", %.*s", (int)node->children[1]->token.length, node->children[1]->token.start);
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

static void operand_debug(struct operand operand, FILE* out)
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
            fprintf(out, "[block &%d: ", operand.value.block->id);
            break;
        case OPERAND_TYPE_NONE:
            return;
        case OPERAND_TYPE_IR:
            fprintf(out, "[func @%s: ", operand.value.ir->symbol);
            break;
    }
    type_code_name(out, operand.typename);
    fprintf(out, "] ");
}

static void instruction_debug(struct ssa_instruction instruction, FILE* out)
{
    operand_debug(instruction.result, out);

    if (instruction.result.type > OPERAND_TYPE_END)
        fprintf(out, "= ");

    fprintf(out, "%s ", operator_name(instruction.operator));

    for (int i = 0; i < MAX_OPERANDS; i++)
        operand_debug(instruction.operands[i], out);

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
        instruction_debug(block->instructions[i], stdout);
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
        instruction_debug(block->instructions[i], out);
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
        operand_debug(chunk->arguments[i], out);
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

char* unit_compile(struct unit* chunk, FILE* out)
{
    //TODO: compile to ARM64
}
