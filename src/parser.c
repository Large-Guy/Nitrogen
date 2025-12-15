#include "parser.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#pragma region parser

enum parser_stage {
    PARSER_STAGE_MODULE_GENERATION,
    PARSER_STAGE_SYMBOL_RESOLUTION_PASS,
    PARSER_STAGE_TREE_GENERATION,
};

struct parser {
    enum parser_stage stage;
    struct module* module;
    struct lexer* lexer;
    struct token current;
    struct token previous;
    uint32_t tp;

    struct ast_node** scope_stack;
    size_t scope_stack_count;
    size_t scope_stack_capacity;
};

static struct ast_node* scope(struct parser* parser) {
    return parser->scope_stack[parser->scope_stack_count - 1];
}

static void push_scope(struct parser* parser, struct ast_node* node) {
    if (parser->scope_stack_count >= parser->scope_stack_capacity) {
        parser->scope_stack_capacity *= 2;
        parser->scope_stack = realloc(parser->scope_stack, sizeof(struct ast_node*) * parser->scope_stack_capacity);
        assert(parser->scope_stack);
    }
    parser->scope_stack[parser->scope_stack_count++] = node;
}

static void pop_scope(struct parser* parser) {
    assert(parser->scope_stack_count > 0);
    parser->scope_stack_count--;
}

static void advance(struct parser* parser) {
    parser->previous = parser->current;

    while (true) {
        parser->current = lexer_read(parser->lexer, parser->tp++);
        if (parser->current.type != TOKEN_TYPE_ERROR)
            break;

        //TODO: error out
    }
}

static struct token peek(struct parser* parser, uint32_t offset) {
    return lexer_read(parser->lexer, parser->tp + offset - 1);
}

static bool type_exists(struct parser* parser, struct token name) {
    if (name.type != TOKEN_TYPE_IDENTIFIER) {
        return false;
    }
    bool has_symbol = module_get_symbol(scope(parser), name);
    if (has_symbol) {
        advance(parser);
    }
    return has_symbol;
}

static void declare_type(struct parser* parser, struct ast_node* symbol) {
    module_add_symbol(parser->module, symbol);
}

static void consume(struct parser* parser, enum token_type type, const char* error_message) {
    if (parser->current.type == type) {
        advance(parser);
        return;
    }

    //TODO: properly error out
    fprintf(stderr, "%s\n", error_message);
    advance(parser); //TODO: eliminate this once erroring is handled properly
}

static bool check(struct parser* parser, enum token_type type) {
    return parser->current.type == type;
}

static void ensure(struct parser* parser, enum token_type type, const char* error_message) {
    if (check(parser, type)) {
        return;
    }

    //TODO: properly error out
    fprintf(stderr, "%s\n", error_message);
}

static bool match(struct parser* parser, enum token_type type) {
    if (!check(parser, type))
        return false;
    advance(parser);
    return true;
}

static bool match_type(struct parser* parser) {
    bool t = match(parser, TOKEN_TYPE_I8) ||
            match(parser, TOKEN_TYPE_I16) ||
            match(parser, TOKEN_TYPE_I32) ||
            match(parser, TOKEN_TYPE_I64) ||
            match(parser, TOKEN_TYPE_U8) ||
            match(parser, TOKEN_TYPE_U16) ||
            match(parser, TOKEN_TYPE_U32) ||
            match(parser, TOKEN_TYPE_U64) ||
            match(parser, TOKEN_TYPE_ISIZE) ||
            match(parser, TOKEN_TYPE_USIZE) ||
            match(parser, TOKEN_TYPE_STRING) ||
            match(parser, TOKEN_TYPE_F32) ||
            match(parser, TOKEN_TYPE_F64) ||
            match(parser, TOKEN_TYPE_VOID);
    

    if (t) {
        return true;
    }
    
    if (module_get_symbol(scope(parser), parser->current)) {
        advance(parser);
        return true;
    }

    return false;
}

struct parser* parser_new(enum parser_stage stage, struct module* module, struct lexer* lexer) {
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
    advance(self);
    if (module)
        push_scope(self, module->definitions);
    return self;
}

void parser_free(struct parser* parser) {
    free(parser->scope_stack);
    free(parser);
}

#pragma endregion

#pragma region expression

static struct token token_null = {0, NULL, 0, 0};
static struct token token_zero = {TOKEN_TYPE_INTEGER, "0", 1, 0};
static struct token token_one = {TOKEN_TYPE_INTEGER, "1", 1, 0};

enum precedence {
    PRECEDENCE_NONE,
    PRECEDENCE_ASSIGNMENT, // =
    PRECEDENCE_OR, // or
    PRECEDENCE_AND, // and
    PRECEDENCE_BITWISE_OR, // |
    PRECEDENCE_BITWISE_XOR, // ^
    PRECEDENCE_BITWISE_AND, // &
    PRECEDENCE_EQUALITY, // == !=
    PRECEDENCE_COMPARISON, // < > <= >=
    PRECEDENCE_SHIFT, // << >>
    PRECEDENCE_TERM, // + -
    PRECEDENCE_FACTOR, // * /
    PRECEDENCE_EXPONENT, // **
    PRECEDENCE_MODULO, // %
    PRECEDENCE_UNARY, // ! -
    PRECEDENCE_CALL, // . ()
    PRECEDENCE_PRIMARY
};

typedef struct ast_node* (*prefix_fn)(struct parser* parser, bool canAssign);
typedef struct ast_node* (*infix_fn)(struct parser* parser, struct ast_node* left, bool canAssign);

struct parse_rule {
    prefix_fn prefix;
    infix_fn infix;
    enum precedence precedence;
};

static struct ast_node* parse_precedence(struct parser* parser, enum precedence precedence);

static struct ast_node* expression(struct parser* parser);

static struct ast_node* number(struct parser* parser, bool canAssign);

static struct ast_node* grouping(struct parser* parser, bool canAssign);

static struct ast_node* unary(struct parser* parser, bool canAssign);

static struct ast_node* binary(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* variable(struct parser* parser, bool canAssign);

static struct ast_node* and(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* or(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* literal(struct parser* parser, bool canAssign);

static struct ast_node* call(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* field(struct parser* parser, struct ast_node* left, bool canAssign);

struct parse_rule rules[] = {
    [TOKEN_TYPE_ERROR] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_EOF] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_LEFT_PAREN] = {grouping, call, PRECEDENCE_CALL},
    [TOKEN_TYPE_RIGHT_PAREN] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_LEFT_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RIGHT_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_LEFT_BRACKET] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RIGHT_BRACKET] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_SEMICOLON] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_DOT] = {NULL, field, PRECEDENCE_CALL},
    [TOKEN_TYPE_COMMA] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_PLUS] = {NULL, binary, PRECEDENCE_TERM},
    [TOKEN_TYPE_PLUS_PLUS] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_PLUS_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_MINUS] = {unary, binary, PRECEDENCE_TERM},
    [TOKEN_TYPE_MINUS_MINUS] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_MINUS_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STAR] = {unary, binary, PRECEDENCE_FACTOR},
    [TOKEN_TYPE_STAR_STAR] = {NULL, binary, PRECEDENCE_EXPONENT},
    [TOKEN_TYPE_STAR_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_SLASH] = {NULL, binary, PRECEDENCE_FACTOR},
    [TOKEN_TYPE_SLASH_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_BANG] = {unary, NULL, PRECEDENCE_TERM},
    [TOKEN_TYPE_BANG_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [TOKEN_TYPE_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_EQUAL_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [TOKEN_TYPE_GREATER] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_GREATER_GREATER] = {NULL, binary, PRECEDENCE_SHIFT},
    [TOKEN_TYPE_GREATER_EQUAL] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_LESS] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_LESS_LESS] = {NULL, binary, PRECEDENCE_SHIFT},
    [TOKEN_TYPE_LESS_EQUAL] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_COLON] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_COLON_COLON] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_AND] = {NULL, binary, PRECEDENCE_BITWISE_AND},
    [TOKEN_TYPE_AND_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_AND_AND] = {NULL, and, PRECEDENCE_AND},
    [TOKEN_TYPE_PIPE] = {NULL, binary, PRECEDENCE_BITWISE_OR},
    [TOKEN_TYPE_PIPE_PIPE] = {NULL, or, PRECEDENCE_OR},
    [TOKEN_TYPE_PERCENT] = {NULL, binary, PRECEDENCE_MODULO},
    [TOKEN_TYPE_PERCENT_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_CARET] = {NULL, binary, PRECEDENCE_BITWISE_XOR},
    [TOKEN_TYPE_CARET_EQUAL] = {NULL, NULL, PRECEDENCE_CALL},
    [TOKEN_TYPE_TILDE] = {unary, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_TILDE_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_MOVE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_COPY] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STRING_LITERAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_INTEGER] = {number, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_FLOATING] = {number, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_IDENTIFIER] = {variable, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_MODULE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_IMPORT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RETURN] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STRUCT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_UNION] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_INTERFACE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STATIC] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_REF] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_CONST] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_OPERATOR] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_UNIQUE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_SHARED] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_VOID] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_I8] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_I16] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_I32] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_I64] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_U8] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_U16] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_U32] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_U64] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_ISIZE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_USIZE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STRING] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_NULL] = {literal, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_TRUE] = {literal, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_FALSE] = {literal, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_IF] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_ELSE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_WHILE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_DO] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_FOR] = {NULL, NULL, PRECEDENCE_NONE},
};

static struct parse_rule* get_rule(enum token_type type) {
    return &rules[type];
}

// prefix
static struct ast_node* make_compound_assignment(struct parser* parser, enum ast_node_type type, struct ast_node* variable, struct ast_node* left) {
    struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
    ast_node_append_child(assignment, variable);
            
    struct ast_node* addition = ast_node_new(type, parser->previous);
    ast_node_append_child(addition, left);
    ast_node_append_child(addition, expression(parser));
            
    ast_node_append_child(assignment, addition);

    return assignment;
}

static struct ast_node* variable(struct parser* parser, bool canAssign) {
    
    struct token token = parser->previous;
    //TODO: proper type handling
    struct ast_node* variable = ast_node_new(AST_NODE_TYPE_NAME, token);
    
    if (canAssign) {
        if (match(parser, TOKEN_TYPE_EQUAL)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            ast_node_append_child(assignment, expression(parser));
            return assignment;
        }

        struct ast_node* left = ast_node_new(AST_NODE_TYPE_NAME, parser->previous);
        
        if (match(parser, TOKEN_TYPE_PLUS_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_ADD, variable, left);
        }
        if (match(parser, TOKEN_TYPE_MINUS_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_SUBTRACT, variable, left);
        }
        if (match(parser, TOKEN_TYPE_STAR_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_MULTIPLY, variable, left);
        }
        if (match(parser, TOKEN_TYPE_SLASH_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_DIVIDE, variable, left);
        }
        if (match(parser, TOKEN_TYPE_PERCENT_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_MODULO, variable, left);
        }
        if (match(parser, TOKEN_TYPE_AND_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_BITWISE_AND, variable, left);
        }
        if (match(parser, TOKEN_TYPE_PIPE_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_BITWISE_OR, variable, left);
        }
        if (match(parser, TOKEN_TYPE_CARET_EQUAL)) {
            return make_compound_assignment(parser, AST_NODE_TYPE_BITWISE_XOR, variable, left);
        }
        if (match(parser, TOKEN_TYPE_PLUS_PLUS)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_ADD, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, ast_node_new(AST_NODE_TYPE_INTEGER, token_one));
            
            ast_node_append_child(assignment, addition);
            return assignment;
        }
        if (match(parser, TOKEN_TYPE_MINUS_MINUS)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_SUBTRACT, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, ast_node_new(AST_NODE_TYPE_INTEGER, token_one));
            
            ast_node_append_child(assignment, addition);
            return assignment;
        }

        ast_node_free(left); //in case it doesn't match a case
    }
    return variable;
}

static struct ast_node* number(struct parser* parser, bool canAssign) { //TODO: floating point numbers
    struct token token = parser->previous;
    if (parser->previous.type == TOKEN_TYPE_FLOATING)
        return ast_node_new(AST_NODE_TYPE_FLOAT, token);
    return ast_node_new(AST_NODE_TYPE_INTEGER, token);
}

static struct ast_node* grouping(struct parser* parser, bool canAssign) {
    struct ast_node* node = expression(parser);
    consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after grouping");
    return node;
}

static struct ast_node* unary(struct parser* parser, bool canAssign) {
    struct token token = parser->previous;

    struct ast_node* operand = parse_precedence(parser, PRECEDENCE_UNARY);
    
    switch (token.type) {
        case TOKEN_TYPE_MINUS: {
            struct ast_node* node = ast_node_new(AST_NODE_TYPE_NEGATE, token);
            ast_node_append_child(node, operand);
            return node;
        }
        case TOKEN_TYPE_TILDE: {
            struct ast_node* node = ast_node_new(AST_NODE_TYPE_BITWISE_NOT, token);
            ast_node_append_child(node, operand);
            return node;
        }
        case TOKEN_TYPE_BANG: {
            struct ast_node* node = ast_node_new(AST_NODE_TYPE_NOT, token);
            ast_node_append_child(node, operand);
            return node;
        }
        default: {
            //TODO: error out
            fprintf(stderr, "unary unexpected token type, this should never happen: %i\n", token.type);
            return NULL;
        }
    }
}

static struct ast_node* literal(struct parser* parser, bool canAssign) {
    struct token token = parser->previous;

    switch (token.type) {
        case TOKEN_TYPE_NULL:
        case TOKEN_TYPE_FALSE:
            return ast_node_new(AST_NODE_TYPE_INTEGER, token_zero);
        case TOKEN_TYPE_TRUE:
            return ast_node_new(AST_NODE_TYPE_INTEGER, token_one);
        default:
            //TODO: error out
            fprintf(stderr, "unexpected literal token type, this should never happen: %i\n", token.type);
    }
    return NULL;
}

//infix

static struct ast_node* binary(struct parser* parser, struct ast_node* left, bool canAssign) {
    struct token op_token = parser->previous;

    struct parse_rule* rule = get_rule(op_token.type);
    
    struct ast_node* right = parse_precedence(parser, (enum precedence)(rule->precedence + 1));

    struct ast_node* operator = NULL;
    
    switch (op_token.type) {
        case TOKEN_TYPE_PLUS: {
            operator = ast_node_new(AST_NODE_TYPE_ADD, op_token);
            break;
        }
        case TOKEN_TYPE_MINUS: {
            operator = ast_node_new(AST_NODE_TYPE_SUBTRACT, op_token);
            break;
        }
        case TOKEN_TYPE_STAR: {
            operator = ast_node_new(AST_NODE_TYPE_MULTIPLY, op_token);
            break;
        }
        case TOKEN_TYPE_SLASH: {
            operator = ast_node_new(AST_NODE_TYPE_DIVIDE, op_token);
            break;
        }
        case TOKEN_TYPE_STAR_STAR: {
            operator = ast_node_new(AST_NODE_TYPE_POWER, op_token);
            break;
        }
        case TOKEN_TYPE_CARET: {
            operator = ast_node_new(AST_NODE_TYPE_BITWISE_XOR, op_token);
            break;
        }
        case TOKEN_TYPE_PIPE: {
            operator = ast_node_new(AST_NODE_TYPE_BITWISE_OR, op_token);
            break;
        }
        case TOKEN_TYPE_AND: {
            operator = ast_node_new(AST_NODE_TYPE_BITWISE_AND, op_token);
            break;
        }
        case TOKEN_TYPE_LESS_LESS: {
            operator = ast_node_new(AST_NODE_TYPE_BITWISE_LEFT, op_token);
            break;
        }
        case TOKEN_TYPE_GREATER_GREATER: {
            operator = ast_node_new(AST_NODE_TYPE_BITWISE_RIGHT, op_token);
            break;
        }
        case TOKEN_TYPE_PERCENT: {
            operator = ast_node_new(AST_NODE_TYPE_MODULO, op_token);
            break;
        }
        case TOKEN_TYPE_EQUAL_EQUAL: {
            operator = ast_node_new(AST_NODE_TYPE_EQUAL, op_token);
            break;
        }
        case TOKEN_TYPE_BANG_EQUAL: {
            operator = ast_node_new(AST_NODE_TYPE_NOT_EQUAL, op_token);
            break;
        }
        case TOKEN_TYPE_GREATER: {
            operator = ast_node_new(AST_NODE_TYPE_GREATER_THAN, op_token);
            break;
        }
        case TOKEN_TYPE_GREATER_EQUAL: {
            operator = ast_node_new(AST_NODE_TYPE_GREATER_THAN_EQUAL, op_token);
            break;
        }
        case TOKEN_TYPE_LESS: {
            operator = ast_node_new(AST_NODE_TYPE_LESS_THAN, op_token);
            break;
        }
        case TOKEN_TYPE_LESS_EQUAL: {
            operator = ast_node_new(AST_NODE_TYPE_LESS_THAN_EQUAL, op_token);
            break;
        }
        default: {
            return NULL;
        }
    }

    ast_node_append_child(operator, left);
    ast_node_append_child(operator, right);
    return operator;
}

static struct ast_node* and(struct parser* parser, struct ast_node* left, bool canAssign) {
    struct token op_token = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_AND, op_token);
    ast_node_append_child(node, left);
    ast_node_append_child(node, parse_precedence(parser, PRECEDENCE_AND));
    return node;
}

static struct ast_node* or(struct parser* parser, struct ast_node* left, bool canAssign) {
    struct token op_token = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_OR, op_token);
    ast_node_append_child(node, left);
    ast_node_append_child(node, parse_precedence(parser, PRECEDENCE_OR));
    return node;
}

static struct ast_node* call(struct parser* parser, struct ast_node* left, bool canAssign) {
    struct token op_token = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_CALL, op_token);
    ast_node_append_child(node, left);
    if (!check(parser, TOKEN_TYPE_RIGHT_PAREN)) {
        do {
            ast_node_append_child(node, expression(parser));
        } while (match(parser, TOKEN_TYPE_COMMA));
    }
    consume(parser, TOKEN_TYPE_RIGHT_PAREN, "Expect ')' after arguments");

    return node;
}

static struct ast_node* field(struct parser* parser, struct ast_node* left, bool canAssign) {
    struct token op_token = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
    struct token field_name = parser->previous;
    if (canAssign) {
        if (match(parser, TOKEN_TYPE_EQUAL)) {
            struct ast_node* node = ast_node_new(AST_NODE_TYPE_SET_FIELD, op_token);
            ast_node_append_child(node, left);
            ast_node_append_child(node, expression(parser));
            return node;
        }    
    }
    
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_GET_FIELD, op_token);
    ast_node_append_child(node, left);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, field_name));
    return node;
}

static struct ast_node* parse_precedence(struct parser* parser, enum precedence precedence) {
    advance(parser);
    prefix_fn prefix_rule = get_rule(parser->previous.type)->prefix; //NOTE: this has been changed from the tutorial, keep in mind!
    if (prefix_rule == NULL) {
        fprintf(stderr, "Expect expression.\n"); //TODO: proper error handling
        return NULL;
    }

    bool canAssign = precedence <= PRECEDENCE_ASSIGNMENT;
    struct ast_node* prefix_result = prefix_rule(parser, canAssign);

    struct ast_node* result = prefix_result;
    
    while (precedence <= get_rule(parser->current.type)->precedence) {
        advance(parser);
        infix_fn infix_rule = get_rule(parser->previous.type)->infix;
        result = infix_rule(parser, result, canAssign);
    }

    return result;
}

static struct ast_node* expression(struct parser* parser) {
    return parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

#pragma endregion

#pragma region ast_gen

static struct ast_node* statement(struct parser* parser);
static struct ast_node* declaration(struct parser* parser);

static struct ast_node* return_statement(struct parser* parser) {
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_RETURN_STATEMENT, parser->previous);
    if (match(parser, TOKEN_TYPE_SEMICOLON)) {
        return node;
    }
    ast_node_append_child(node, expression(parser));
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after return");
    return node;
}

static struct ast_node* get_type_node(struct parser* parser, struct token token) {
    switch (token.type) { //TODO: most of these nodes should be made static
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
            return module_get_symbol(scope(parser), token);
        default:
            return NULL;
    }
}

static struct ast_node* get_sub_symbol(struct ast_node* parent_symbol, struct token name) {
    for (int i = 0; i < parent_symbol->children_count; i++) {
        struct ast_node* child = parent_symbol->children[i];
        if (child->type != AST_NODE_TYPE_STRUCT)
            continue;
        struct token symbol = (*child->children)->token;
        if (symbol.length == name.length &&
            memcmp(symbol.start, name.start, name.length) == 0) {
            return child;
        }
    }
    return NULL;
}

static struct ast_node* definition(struct parser* parser, bool statement, bool canAssign, bool inlineDeclaration) {
    struct token type = parser->previous;
    struct ast_node* type_node = get_type_node(parser, type);
    while (match(parser, TOKEN_TYPE_DOT)) {
        consume(parser, TOKEN_TYPE_IDENTIFIER, "expected sub type after '.'"); //move the dot out of the way
        type_node = get_sub_symbol(type_node, parser->previous);
    }
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token name = parser->previous;
    if (match(parser, TOKEN_TYPE_LEFT_PAREN)) {
        //function
        struct ast_node* node = ast_node_new(AST_NODE_TYPE_FUNCTION, token_null);
        ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
        ast_node_append_child(node, type_node);
        //parameters
        if (!check(parser, TOKEN_TYPE_RIGHT_PAREN)) {
            do {
                advance(parser);
                ast_node_append_child(node, definition(parser, false, true, true));
            } while (match(parser, TOKEN_TYPE_COMMA));
        }
        consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after declaration");
        if (inlineDeclaration)
            ast_node_append_child(node, declaration(parser));
        else if (statement)
            consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after variable definition");
        return node;
    }

    //variable
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_VARIABLE, token_null);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
    ast_node_append_child(node, type_node);
    if (canAssign && match(parser, TOKEN_TYPE_EQUAL)) {
        ast_node_append_child(node, expression(parser));
    }
    if (statement)
        consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after variable definition");
    return node;
}

static struct ast_node* definition_statement(struct parser* parser) {
    struct ast_node* node = definition(parser, true, true, true);
    return node;
}

static struct ast_node* struct_statement(struct parser* parser) {
    struct token type = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected struct name");
    struct token name = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_STRUCT, type);
    struct ast_node* symbol = get_type_node(parser, name);
    ast_node_append_child(node, symbol);
    
    if (match(parser, TOKEN_TYPE_COLON)) {
        do {
            consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
            struct token interface = parser->previous;
            ast_node_append_child(node, module_get_symbol(parser->module->definitions, interface));
        } while (match(parser, TOKEN_TYPE_COMMA));
    }

    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct definition");

    push_scope(parser, symbol);
    
    while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
        if (match(parser, TOKEN_TYPE_STRUCT)) {
            ast_node_append_child(node, struct_statement(parser));
        }
        else if (match_type(parser)) {
            printf("matched type");
            ast_node_append_child(node, definition(parser, true, false, true));
        }
        else {
            advance(parser);
            //TODO: error out
            fprintf(stderr, "expected type\n");
        }
    }
    
    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");

    pop_scope(parser);
    
    return node;
}

static struct ast_node* interface_statement(struct parser* parser) {
    struct token type = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected interface name");
    struct token name = parser->previous;
    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after interface definition");
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_INTERFACE, type);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));

    while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
        advance(parser);
        ast_node_append_child(node, definition(parser, true, false, false));
    }
    
    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");
    return node;
}

static struct ast_node* expression_statement(struct parser* parser) {
    struct ast_node* node = expression(parser);
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after expression");
    return node;
}

static struct ast_node* block(struct parser* parser) {
    struct token token = parser->previous;
    struct ast_node* sequence = ast_node_new(AST_NODE_TYPE_SEQUENCE, token);
    while (!check(parser, TOKEN_TYPE_RIGHT_BRACE) && !check(parser, TOKEN_TYPE_EOF)) {
        ast_node_append_child(sequence, declaration(parser));
    }

    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after block");
    return sequence;
}

static struct ast_node* if_statement(struct parser* parser) {
    struct token op_token = parser->previous;
    struct ast_node* branch = ast_node_new(AST_NODE_TYPE_IF, op_token);
    consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after if");
    ast_node_append_child(branch, expression(parser));
    consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after if");
    ast_node_append_child(branch, declaration(parser));
    if (match(parser, TOKEN_TYPE_ELSE)) {
        ast_node_append_child(branch, declaration(parser));
    }
    
    return branch;
}

static struct ast_node* while_statement(struct parser* parser) {
    struct token op_token = parser->previous;
    struct ast_node* loop = ast_node_new(AST_NODE_TYPE_WHILE, op_token);
    consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after while");
    ast_node_append_child(loop, expression(parser));
    consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after while");
    ast_node_append_child(loop, declaration(parser));
    
    return loop;
}

static struct ast_node* do_while_statement(struct parser* parser) {
    struct token op_token = parser->previous;
    struct ast_node* loop = ast_node_new(AST_NODE_TYPE_DO_WHILE, op_token);
    ast_node_append_child(loop, declaration(parser));
    consume(parser, TOKEN_TYPE_WHILE, "expected 'while' statement after do block");
    consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after while");
    ast_node_append_child(loop, expression(parser));
    consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after while");
    return loop;
}

static struct ast_node* for_statement(struct parser* parser) {
    struct token op_token = parser->previous;
    struct ast_node* loop = ast_node_new(AST_NODE_TYPE_WHILE, op_token);
    consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after for");
    advance(parser); //move the type to the previous
    struct ast_node* init = definition_statement(parser);
    struct ast_node* condition = expression_statement(parser);
    struct ast_node* incr = expression(parser);
    consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after for");
    struct ast_node* body = declaration(parser);
    struct ast_node* root = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    ast_node_append_child(root, init);
    ast_node_append_child(root, condition);
    ast_node_append_child(root, incr);
    ast_node_append_child(root, body);
    return root;
}

static struct ast_node* module_statement(struct parser* parser) {
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected module name");
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after module");
    return NULL;
}

static struct ast_node* statement(struct parser* parser) {
    if (match(parser, TOKEN_TYPE_RETURN)) {
        return return_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_IF)) {
        return if_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_WHILE)) {
        return while_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_DO)) {
        return do_while_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_FOR)) {
        return for_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_MODULE)) {
        return module_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_IMPORT)) {
        return module_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
        return block(parser);
    }
    return expression_statement(parser);
}

static struct ast_node* declaration(struct parser* parser) {
    if (match(parser, TOKEN_TYPE_STRUCT)) {
        return struct_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_UNION)) {
        //TODO: unions
    }
    if (match(parser, TOKEN_TYPE_INTERFACE)) {
        return interface_statement(parser);
    } //Types
    if (match_type(parser)) {
        return definition_statement(parser);
    }
    return statement(parser);
}

#pragma endregion 

#pragma region passes

static void skip_block(struct parser* parser) {
    while (!check(parser, TOKEN_TYPE_RIGHT_BRACE)) {
        if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else {
            advance(parser);
        }
    }

    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after block");
}

static struct ast_node* interface_type(struct parser* parser) {
    struct token op_token = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier after struct");
    struct token name = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_INTERFACE, op_token);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_TYPE, name));

    if (match(parser, TOKEN_TYPE_COLON)) {
        do {
            consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
        } while (match(parser, TOKEN_TYPE_COMMA));
    }
    
    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct definition");
    
    while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
        if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else {
            advance(parser);
        }
    }
    
    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");
    return node;
}

static struct ast_node* struct_type(struct parser* parser) {
    struct token op_token = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier after struct");
    struct token name = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_STRUCT, op_token);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_TYPE, name));
    
    if (match(parser, TOKEN_TYPE_COLON)) {
        do {
            consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
        } while (match(parser, TOKEN_TYPE_COMMA));
    }

    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct definition");
    
    while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
        if (match(parser, TOKEN_TYPE_STRUCT)) {
            ast_node_append_child(node, struct_type(parser));
        }
        else if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else {
            advance(parser);
        }
    }
    
    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");
    return node;
}

static struct ast_node* variable_symbol(struct parser* parser) {
    struct token type = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier after field symbol");
    struct token name = parser->previous;
    if (match(parser, TOKEN_TYPE_LEFT_PAREN)) {
        struct ast_node* node = ast_node_new(AST_NODE_TYPE_FUNCTION, type);
        ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));

        if (!check(parser, TOKEN_TYPE_RIGHT_PAREN)) {
            do {
                advance(parser);
                ast_node_append_child(node, variable_symbol(parser));
            } while (match(parser, TOKEN_TYPE_COMMA));
        }
        consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after declaration");
        if (match(parser, TOKEN_TYPE_LEFT_BRACE))
            skip_block(parser);
        return node;
    }
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_VARIABLE, type);
    ast_node_append_child(node, get_type_node(parser, type));
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
    return node;
}

static void struct_symbol_resolve(struct parser* parser) {
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier after struct");
    struct token name = parser->previous;
    struct ast_node* symbol = module_get_symbol(scope(parser), name);
    if (symbol == NULL) {
        //TODO: error out
        fprintf(stderr, "new symbol discovered during resolution pass");
    }
    if (match(parser, TOKEN_TYPE_COLON)) {
        do {
            consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
            struct token interface = parser->previous;
            ast_node_append_child(symbol, module_get_symbol(scope(parser), interface));
        } while (match(parser, TOKEN_TYPE_COMMA));
    }
    push_scope(parser, symbol);
    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct");
    while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
        if (match(parser, TOKEN_TYPE_STRUCT)) {
            struct_symbol_resolve(parser);
        }
        else if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else if (match_type(parser)) {
            ast_node_append_child(symbol, variable_symbol(parser));
        }
        else {
            advance(parser);
        }
    }
    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct");
    pop_scope(parser);
}

static void interface_symbol_resolve(struct parser* parser) {
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier after struct");
    struct token name = parser->previous;
    struct ast_node* symbol = module_get_symbol(scope(parser), name);
    if (symbol == NULL) {
        //TODO: error out
        fprintf(stderr, "new symbol discovered during resolution pass");
    }
    if (match(parser, TOKEN_TYPE_COLON)) {
        do {
            consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
            struct token interface = parser->previous;
            ast_node_append_child(symbol, module_get_symbol(scope(parser), interface));
        } while (match(parser, TOKEN_TYPE_COMMA));
    }
    push_scope(parser, symbol);
    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct");
    while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
        if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else if (match_type(parser)) {
            ast_node_append_child(symbol, variable_symbol(parser));
        }
        else {
            advance(parser);
        }
    }
    consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after interface");
    pop_scope(parser);
}

static void symbol_pass(struct module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        struct module* module = list->modules[i];
        for (int y = 0; y < module->lexer_count; y++) {
            struct lexer* lexer = module->lexers[y];
            
            struct parser* parser = parser_new(PARSER_STAGE_SYMBOL_RESOLUTION_PASS, module, lexer);

            while (!match(parser, TOKEN_TYPE_EOF)) {
                if (match(parser, TOKEN_TYPE_STRUCT)) {
                    struct_symbol_resolve(parser);
                }
                else if (match(parser, TOKEN_TYPE_INTERFACE)) {
                    interface_symbol_resolve(parser);
                }
                else if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
                    skip_block(parser);
                }
                else if (match_type(parser)) {
                    ast_node_append_child(module->definitions, variable_symbol(parser));
                }
                else {
                    advance(parser);
                }
            }

            parser_free(parser);
        }
    }
}

static void type_pass(struct module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        struct module* module = list->modules[i];
        for (int y = 0; y < module->lexer_count; y++) {
            struct lexer* lexer = module->lexers[y];
            
            struct parser* parser = parser_new(PARSER_STAGE_SYMBOL_RESOLUTION_PASS, module, lexer);

            while (!match(parser, TOKEN_TYPE_EOF)) {
                if (match(parser, TOKEN_TYPE_STRUCT)) {
                    module_add_symbol(module, struct_type(parser));
                }
                else if (match(parser, TOKEN_TYPE_INTERFACE)) {
                    module_add_symbol(module, interface_type(parser));
                }
                else if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
                    skip_block(parser);
                }
                else {
                    advance(parser);
                }
            }

            parser_free(parser);
        }
    }
}

static void import_pass(struct module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        struct module* module = list->modules[i];
        for (int y = 0; y < module->lexer_count; y++) {
            struct lexer* lexer = module->lexers[y];
            
            struct parser* parser = parser_new(PARSER_STAGE_SYMBOL_RESOLUTION_PASS, module, lexer);
   
            while (!match(parser, TOKEN_TYPE_EOF)) {
                if (match(parser, TOKEN_TYPE_IMPORT)) {
                    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected module name");
                    struct token name = parser->previous;
                    struct module* import = NULL;
                    for (int j = 0; j < list->module_count; j++) {
                        if (strlen(list->modules[j]->name) == name.length &&
                            memcmp(list->modules[j]->name, name.start, name.length) == 0) {
                            import = list->modules[j];
                        }
                    }
                    if (import == NULL) {
                        //TODO error out
                        fprintf(stderr, "unable to find module to import\n");
                        goto exit;
                    }
                    //TODO: import symbols
                    printf("imported module...\n");
                    consume(parser, TOKEN_TYPE_SEMICOLON, "expected semi colon after import name");
                }
                advance(parser);
            }

            exit:
            parser_free(parser);
        }
    }
}

static struct module_list module_pass(struct lexer** lexers, uint32_t count) {
    struct module** modules = malloc(sizeof(struct module*));
    uint32_t modules_count = 0;
    uint32_t modules_capacity = 1;

    for (uint32_t i = 0; i < count; i++) {
        struct lexer* lexer = lexers[i];
        
        struct parser* parser = parser_new(PARSER_STAGE_MODULE_GENERATION, NULL, lexer);

        bool found = false;
        
        while (!match(parser, TOKEN_TYPE_EOF)) {
            if (match(parser, TOKEN_TYPE_MODULE)) {
                consume(parser, TOKEN_TYPE_IDENTIFIER, "expected module name");
                struct token name = parser->previous;
                struct module* module = NULL;
                for (uint32_t j = 0; j < modules_count; j++) {
                    if (strlen(modules[j]->name) == name.length &&
                        memcmp(modules[j]->name, name.start, name.length) == 0) {
                        module = modules[j];
                    }
                }
                if (module == NULL) {
                    if (modules_count >= modules_capacity) {
                        modules_capacity *= 2;
                        modules = realloc(modules, modules_capacity * sizeof(struct module*));
                        assert(modules != NULL);
                    }
                    module = module_new(parser->previous);
                    modules[modules_count++] = module;
                }
                module_add_source(module, lexer);
                found = true;
                break;
            }
            advance(parser);
        }

        if (!found) //todo: error out
            fprintf(stderr, "failed to find module name in source file");

        parser_free(parser);
    }

    struct module_list list = {modules, modules_count};
    return list;
}

static void tree_gen_pass(struct module_list* list) {
    for (int i = 0; i < list->module_count; i++) {
        struct module* module = list->modules[i];

        for (int y = 0; y < module->lexer_count; y++) {
            struct lexer* lexer = module->lexers[y];
            
            struct parser* parser = parser_new(PARSER_STAGE_TREE_GENERATION, module, lexer);

            while (!match(parser, TOKEN_TYPE_EOF)) {
                struct ast_node* node = declaration(parser);
                if (node != NULL)
                    ast_node_append_child(module->root, node);
            }

            parser_free(parser);
        }
    }
}

#pragma endregion

struct module_list parse(struct lexer** lexer, uint32_t count) {
    
    //resolve modules and sort lexers
    struct module_list modules = module_pass(lexer, count);

    import_pass(&modules); //merge symbol tables

    type_pass(&modules);

    symbol_pass(&modules);

    tree_gen_pass(&modules);

    return modules;
}