#include "parser.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct parser* parser_new(enum parser_stage stage, struct ast_module* module, struct lexer* lexer) {
    struct parser* self = malloc(sizeof(struct parser));
    assert(self);
    self->stage = stage;
    self->module = module;
    self->lexer = lexer;
    self->tp = 0;
    self->scope_stack = malloc(sizeof(struct ast_node*));
    assert(self->scope_stack);
    self->scope_stack_count = 0;
    self->scope_stack_capacity = 1;
    parser_advance(self);
    if (module)
        parser_push_scope(self, module->symbols);
    self->error = false;
    return self;
}

void parser_free(struct parser* parser) {
    free(parser->scope_stack);
    free(parser);
}

struct ast_node* parser_scope(struct parser* parser) {
    assert(parser);
    assert(parser->scope_stack);
    return parser->scope_stack[parser->scope_stack_count - 1];
}

void parser_push_scope(struct parser* parser, struct ast_node* node) {
    if (parser->scope_stack_count >= parser->scope_stack_capacity) {
        parser->scope_stack_capacity *= 2;
        parser->scope_stack = realloc(parser->scope_stack, sizeof(struct ast_node*) * parser->scope_stack_capacity);
        assert(parser->scope_stack);
    }
    parser->scope_stack[parser->scope_stack_count++] = node;
}

void parser_pop_scope(struct parser* parser) {
    assert(parser->scope_stack_count > 0);
    parser->scope_stack_count--;
}

void parser_error(struct parser* parser, struct token at, const char* message) {
    fprintf(stderr, "[line %d] Error ", at.line);
    if (at.type == TOKEN_TYPE_EOF) {
        fprintf(stderr, "at end");
    } else if (at.type == TOKEN_TYPE_ERROR) {
        
    } else {
        fprintf(stderr, "at '%.*s': ", (uint32_t)at.length, at.start);
    }
    fprintf(stderr, "%s\n", message);
    parser->error = true;
}

void parser_advance(struct parser* parser) {
    parser->previous = parser->current;

    while (true) {
        parser->current = lexer_read(parser->lexer, parser->tp++);
        if (parser->current.type != TOKEN_TYPE_ERROR)
            break;

        parser_error(parser, parser->current, "Unexpected end of input");
    }
}

struct token parser_peek(struct parser* parser, uint32_t offset) {
    return lexer_read(parser->lexer, parser->tp + offset - 1);
}

bool parser_type_exists(struct parser* parser, struct token name) {
    if (name.type != TOKEN_TYPE_IDENTIFIER) {
        return false;
    }
    bool has_symbol = ast_module_get_symbol(parser_scope(parser), name);
    if (has_symbol) {
        parser_advance(parser);
    }
    return has_symbol;
}

void parser_declare_type(struct parser* parser, struct ast_node* symbol) {
    ast_module_add_symbol(parser->module, symbol);
}

void parser_consume(struct parser* parser, enum token_type type, const char* error_message) {
    if (parser->current.type == type) {
        parser_advance(parser);
        return;
    }

    parser_error(parser, parser->previous, error_message);
}

bool parser_check(struct parser* parser, enum token_type type) {
    return parser->current.type == type;
}

bool parser_match(struct parser* parser, enum token_type type) {
    if (!parser_check(parser, type))
        return false;
    parser_advance(parser);
    return true;
}

bool parser_match_type(struct parser* parser) {
    bool t = parser_match(parser, TOKEN_TYPE_I8) ||
            parser_match(parser, TOKEN_TYPE_I16) ||
            parser_match(parser, TOKEN_TYPE_I32) ||
            parser_match(parser, TOKEN_TYPE_I64) ||
            parser_match(parser, TOKEN_TYPE_U8) ||
            parser_match(parser, TOKEN_TYPE_U16) ||
            parser_match(parser, TOKEN_TYPE_U32) ||
            parser_match(parser, TOKEN_TYPE_U64) ||
            parser_match(parser, TOKEN_TYPE_ISIZE) ||
            parser_match(parser, TOKEN_TYPE_USIZE) ||
            parser_match(parser, TOKEN_TYPE_STRING) ||
            parser_match(parser, TOKEN_TYPE_F32) ||
            parser_match(parser, TOKEN_TYPE_F64) ||
            parser_match(parser, TOKEN_TYPE_VOID);
    

    if (t) {
        return true;
    }
    struct ast_node* symbol = ast_module_get_symbol(parser_scope(parser), parser->current);
    if (symbol &&
        (symbol->type == AST_NODE_TYPE_STRUCT ||
            symbol->type == AST_NODE_TYPE_INTERFACE)
        ) {
        parser_advance(parser);
        return true;
    }

    return false;
}

void skip_block(struct parser* parser) {
    while (!parser_check(parser, TOKEN_TYPE_RIGHT_BRACE))
    {
        if (parser_match(parser, TOKEN_TYPE_LEFT_BRACE))
        {
            skip_block(parser);
        }
        else
        {
            parser_advance(parser);
        }
    }

    parser_consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after block");
}

static struct ast_node* get_type_node(struct parser* parser, struct token token) {
    switch (token.type)
    {
        case TOKEN_TYPE_I8:
            return ast_node_new(AST_NODE_TYPE_I8, token);
        case TOKEN_TYPE_I16:
            return ast_node_new(AST_NODE_TYPE_I16, token);
        case TOKEN_TYPE_I32:
            return ast_node_new(AST_NODE_TYPE_I32, token);
        case TOKEN_TYPE_I64:
            return ast_node_new(AST_NODE_TYPE_I64, token);
        case TOKEN_TYPE_U8:
            return ast_node_new(AST_NODE_TYPE_U8, token);
        case TOKEN_TYPE_U16:
            return ast_node_new(AST_NODE_TYPE_U16, token);
        case TOKEN_TYPE_U32:
            return ast_node_new(AST_NODE_TYPE_U32, token);
        case TOKEN_TYPE_U64:
            return ast_node_new(AST_NODE_TYPE_U64, token);
        case TOKEN_TYPE_F32:
            return ast_node_new(AST_NODE_TYPE_F32, token);
        case TOKEN_TYPE_F64:
            return ast_node_new(AST_NODE_TYPE_F64, token);
        case TOKEN_TYPE_VOID:
            return ast_node_new(AST_NODE_TYPE_VOID, token);
        case TOKEN_TYPE_IDENTIFIER:
            return ast_module_get_symbol(parser_scope(parser), token);
        default:
            return NULL;
    }
}

static struct ast_node* append_type_attribute(struct parser* parser, struct ast_node* current)
{
    if (parser_match(parser, TOKEN_TYPE_STAR))
    {
        enum ast_node_type type = AST_NODE_TYPE_REFERENCE;
        if (parser_match(parser, TOKEN_TYPE_QUESTION))
        {
            type = AST_NODE_TYPE_POINTER;
        }
        struct ast_node* pointer = ast_node_new(type, parser->previous);
        ast_node_append_child(pointer, current);
        return append_type_attribute(parser, pointer);
    }
    if (parser_match(parser, TOKEN_TYPE_STAR_STAR))
    {
        struct ast_node* base_pointer = ast_node_new(AST_NODE_TYPE_REFERENCE, parser->previous);
        ast_node_append_child(base_pointer, current);

        enum ast_node_type type = AST_NODE_TYPE_REFERENCE;
        if (parser_match(parser, TOKEN_TYPE_QUESTION))
        {
            type = AST_NODE_TYPE_POINTER;
        }
        struct ast_node* pointer = ast_node_new(type, parser->previous);
        ast_node_append_child(pointer, base_pointer);
        return append_type_attribute(parser, pointer);
    }
    if (parser_match(parser, TOKEN_TYPE_LEFT_BRACKET))
    {
        struct ast_node* array = ast_node_new(AST_NODE_TYPE_ARRAY, parser->previous);

        ast_node_append_child(array, current);
        struct ast_node* node = append_type_attribute(parser, array);
        // add initial size option

        parser_consume(parser, TOKEN_TYPE_RIGHT_BRACKET, "forgotten closing bracket ']'");
        return node;
    }
    if (parser_match(parser, TOKEN_TYPE_LESS))
    {
        struct ast_node* simd = ast_node_new(AST_NODE_TYPE_SIMD, parser->previous);
        parser_consume(parser, TOKEN_TYPE_INTEGER, "SIMD types must have fixed size");
        struct ast_node* size = ast_node_new(AST_NODE_TYPE_INTEGER, parser->previous);
        parser_consume(parser, TOKEN_TYPE_GREATER, "forgotten closing '>' for SIMD type");
        ast_node_append_child(simd, current);
        ast_node_append_child(simd, size);
        return append_type_attribute(parser, simd);
    }
    return current;
}

struct ast_node* parser_build_type(struct parser* parser) {
    struct token type = parser->previous;
    struct ast_node* type_node = get_type_node(parser, type);
    while (parser_match(parser, TOKEN_TYPE_DOT))
    {
        parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected sub type after '.'"); //move the dot out of the way
        type_node = ast_node_symbol_sub(type_node, parser->previous);
    }
    return append_type_attribute(parser, type_node);
}