#include "ast_gen.h"

#include "dependency_graph_gen.h"
#include "parser.h"
#include "signature_gen.h"
#include "type_declaration_gen.h"

#pragma region utilities

static struct ast_node* expression(struct parser* parser);

#pragma endregion 

#pragma region expression

enum precedence
{
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

struct parse_rule
{
    prefix_fn prefix;
    infix_fn infix;
    enum precedence precedence;
};

static struct ast_node* parse_precedence(struct parser* parser, enum precedence precedence);

static struct ast_node* number(struct parser* parser, bool canAssign);

static struct ast_node* grouping(struct parser* parser, bool canAssign);

static struct ast_node* unary(struct parser* parser, bool canAssign);

static struct ast_node* cast(struct parser* parser, bool canAssign);

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
    [TOKEN_TYPE_AND] = {unary, binary, PRECEDENCE_BITWISE_AND},
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
    [TOKEN_TYPE_REGION] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RETURN] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STRUCT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_UNION] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_INTERFACE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STATIC] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_CONST] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_OPERATOR] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_UNIQUE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_SHARED] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_VOID] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_I8] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_I16] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_I32] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_I64] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_U8] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_U16] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_U32] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_U64] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_F32] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_F64] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_ISIZE] = {cast, NULL, PRECEDENCE_UNARY},
    [TOKEN_TYPE_USIZE] = {cast, NULL, PRECEDENCE_UNARY},
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

static struct parse_rule* get_rule(enum token_type type)
{
    return &rules[type];
}

// prefix
static struct ast_node* make_compound_assignment(struct parser* parser, enum ast_node_type type,
                                                 struct ast_node* variable, struct ast_node* left)
{
    struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
    ast_node_append_child(assignment, variable);

    struct ast_node* addition = ast_node_new(type, parser->previous);
    ast_node_append_child(addition, left);
    ast_node_append_child(addition, expression(parser));

    ast_node_append_child(assignment, addition);

    return assignment;
}

static struct ast_node* variable(struct parser* parser, bool canAssign)
{
    struct token token = parser->previous;
    struct ast_node* variable = ast_node_new(AST_NODE_TYPE_NAME, token);

    if (canAssign)
    {
        if (parser_match(parser, TOKEN_TYPE_EQUAL))
        {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            ast_node_append_child(assignment, expression(parser));
            return assignment;
        }

        struct ast_node* left = ast_node_new(AST_NODE_TYPE_NAME, parser->previous);

        if (parser_match(parser, TOKEN_TYPE_PLUS_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_ADD, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_MINUS_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_SUBTRACT, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_STAR_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_MULTIPLY, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_SLASH_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_DIVIDE, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_PERCENT_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_MODULO, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_AND_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_BITWISE_AND, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_PIPE_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_BITWISE_OR, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_CARET_EQUAL))
        {
            return make_compound_assignment(parser, AST_NODE_TYPE_BITWISE_XOR, variable, left);
        }
        if (parser_match(parser, TOKEN_TYPE_PLUS_PLUS))
        {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);

            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_ADD, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, ast_node_new(AST_NODE_TYPE_INTEGER, token_one));

            ast_node_append_child(assignment, addition);
            return assignment;
        }
        if (parser_match(parser, TOKEN_TYPE_MINUS_MINUS))
        {
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

static struct ast_node* number(struct parser* parser, bool canAssign)
{
    struct token token = parser->previous;
    if (parser->previous.type == TOKEN_TYPE_FLOATING)
        return ast_node_new(AST_NODE_TYPE_FLOAT, token);
    return ast_node_new(AST_NODE_TYPE_INTEGER, token);
}

static struct ast_node* grouping(struct parser* parser, bool canAssign)
{
    struct ast_node* node = expression(parser);
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after grouping");
    return node;
}

static struct ast_node* unary(struct parser* parser, bool canAssign)
{
    struct token token = parser->previous;

    struct ast_node* operand = parse_precedence(parser, PRECEDENCE_UNARY);

    switch (token.type)
    {
        case TOKEN_TYPE_MINUS:
            {
                struct ast_node* node = ast_node_new(AST_NODE_TYPE_NEGATE, token);
                ast_node_append_child(node, operand);
                return node;
            }
        case TOKEN_TYPE_TILDE:
            {
                struct ast_node* node = ast_node_new(AST_NODE_TYPE_BITWISE_NOT, token);
                ast_node_append_child(node, operand);
                return node;
            }
        case TOKEN_TYPE_BANG:
            {
                struct ast_node* node = ast_node_new(AST_NODE_TYPE_NOT, token);
                ast_node_append_child(node, operand);
                return node;
            }
        case TOKEN_TYPE_AND:
            {
                struct ast_node* node = ast_node_new(AST_NODE_TYPE_ADDRESS, token);
                ast_node_append_child(node, operand);
                return node;
            }
        case TOKEN_TYPE_STAR: {
            struct ast_node* node = ast_node_new(AST_NODE_TYPE_LOCK, token);
            ast_node_append_child(node, operand);
            return node;
        }
        default:
            {
                parser_error(parser, token, "unary unexpected token type, this should never happen\n");
                return NULL;
            }
    }
}

static struct ast_node* cast(struct parser* parser, bool canAssign) {
    struct ast_node* type_node = parser_build_type(parser);
    struct ast_node* cast_node;
    if (parser_match(parser, TOKEN_TYPE_BANG)) {
        cast_node = ast_node_new(AST_NODE_TYPE_REINTERPRET_CAST, parser->previous);
    }
    else {
        cast_node = ast_node_new(AST_NODE_TYPE_STATIC_CAST, parser->previous);
    }
    parser_consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after cast");
    ast_node_append_child(cast_node, type_node);
    ast_node_append_child(cast_node, expression(parser));
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after cast body");
    return cast_node;
}

static struct ast_node* literal(struct parser* parser, bool canAssign)
{
    struct token token = parser->previous;

    switch (token.type)
    {
        case TOKEN_TYPE_NULL:
            return ast_node_new(AST_NODE_TYPE_POINTER, token_zero);
        case TOKEN_TYPE_FALSE:
            return ast_node_new(AST_NODE_TYPE_BOOL, token_zero);
        case TOKEN_TYPE_TRUE:
            return ast_node_new(AST_NODE_TYPE_BOOL, token_one);
        default:
            parser_error(parser, token, "unexpected literal token type, this should never happen\n");
    }
    return NULL;
}

//infix

static struct ast_node* binary(struct parser* parser, struct ast_node* left, bool canAssign)
{
    struct token op_token = parser->previous;

    struct parse_rule* rule = get_rule(op_token.type);

    struct ast_node* right = parse_precedence(parser, (enum precedence)(rule->precedence + 1));

    struct ast_node* operator = NULL;

    switch (op_token.type)
    {
        case TOKEN_TYPE_PLUS:
            {
                operator = ast_node_new(AST_NODE_TYPE_ADD, op_token);
                break;
            }
        case TOKEN_TYPE_MINUS:
            {
                operator = ast_node_new(AST_NODE_TYPE_SUBTRACT, op_token);
                break;
            }
        case TOKEN_TYPE_STAR:
            {
                operator = ast_node_new(AST_NODE_TYPE_MULTIPLY, op_token);
                break;
            }
        case TOKEN_TYPE_SLASH:
            {
                operator = ast_node_new(AST_NODE_TYPE_DIVIDE, op_token);
                break;
            }
        case TOKEN_TYPE_STAR_STAR:
            {
                operator = ast_node_new(AST_NODE_TYPE_POWER, op_token);
                break;
            }
        case TOKEN_TYPE_CARET:
            {
                operator = ast_node_new(AST_NODE_TYPE_BITWISE_XOR, op_token);
                break;
            }
        case TOKEN_TYPE_PIPE:
            {
                operator = ast_node_new(AST_NODE_TYPE_BITWISE_OR, op_token);
                break;
            }
        case TOKEN_TYPE_AND:
            {
                operator = ast_node_new(AST_NODE_TYPE_BITWISE_AND, op_token);
                break;
            }
        case TOKEN_TYPE_LESS_LESS:
            {
                operator = ast_node_new(AST_NODE_TYPE_BITWISE_LEFT, op_token);
                break;
            }
        case TOKEN_TYPE_GREATER_GREATER:
            {
                operator = ast_node_new(AST_NODE_TYPE_BITWISE_RIGHT, op_token);
                break;
            }
        case TOKEN_TYPE_PERCENT:
            {
                operator = ast_node_new(AST_NODE_TYPE_MODULO, op_token);
                break;
            }
        case TOKEN_TYPE_EQUAL_EQUAL:
            {
                operator = ast_node_new(AST_NODE_TYPE_EQUAL, op_token);
                break;
            }
        case TOKEN_TYPE_BANG_EQUAL:
            {
                operator = ast_node_new(AST_NODE_TYPE_NOT_EQUAL, op_token);
                break;
            }
        case TOKEN_TYPE_GREATER:
            {
                operator = ast_node_new(AST_NODE_TYPE_GREATER_THAN, op_token);
                break;
            }
        case TOKEN_TYPE_GREATER_EQUAL:
            {
                operator = ast_node_new(AST_NODE_TYPE_GREATER_THAN_EQUAL, op_token);
                break;
            }
        case TOKEN_TYPE_LESS:
            {
                operator = ast_node_new(AST_NODE_TYPE_LESS_THAN, op_token);
                break;
            }
        case TOKEN_TYPE_LESS_EQUAL:
            {
                operator = ast_node_new(AST_NODE_TYPE_LESS_THAN_EQUAL, op_token);
                break;
            }
        default:
            {
                parser_error(parser, parser->current, "unexpected operator in binary expression");
                return NULL;
            }
    }

    ast_node_append_child(operator, left);
    ast_node_append_child(operator, right);
    return operator;
}

static struct ast_node* and(struct parser* parser, struct ast_node* left, bool canAssign)
{
    struct token op_token = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_AND, op_token);
    ast_node_append_child(node, left);
    ast_node_append_child(node, parse_precedence(parser, PRECEDENCE_AND));
    return node;
}

static struct ast_node* or(struct parser* parser, struct ast_node* left, bool canAssign)
{
    struct token op_token = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_OR, op_token);
    ast_node_append_child(node, left);
    ast_node_append_child(node, parse_precedence(parser, PRECEDENCE_OR));
    return node;
}

static struct ast_node* call(struct parser* parser, struct ast_node* left, bool canAssign)
{
    struct token op_token = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_CALL, op_token);
    ast_node_append_child(node, left);
    if (!parser_check(parser, TOKEN_TYPE_RIGHT_PAREN))
    {
        do
        {
            ast_node_append_child(node, expression(parser));
        }
        while (parser_match(parser, TOKEN_TYPE_COMMA));
    }
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "Expect ')' after arguments");

    return node;
}

static struct ast_node* field(struct parser* parser, struct ast_node* left, bool canAssign)
{
    struct token op_token = parser->previous;
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
    struct token field_name = parser->previous;
    if (canAssign)
    {
        if (parser_match(parser, TOKEN_TYPE_EQUAL))
        {
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

static struct ast_node* parse_precedence(struct parser* parser, enum precedence precedence)
{
    parser_advance(parser);
    prefix_fn prefix_rule = get_rule(parser->previous.type)->prefix;
    //NOTE: this has been changed from the tutorial, keep in mind!
    if (prefix_rule == NULL)
    {
        parser_error(parser, parser->previous, "expected expression\n");
        return NULL;
    }

    bool canAssign = precedence <= PRECEDENCE_ASSIGNMENT;
    struct ast_node* prefix_result = prefix_rule(parser, canAssign);

    struct ast_node* result = prefix_result;

    while (precedence <= get_rule(parser->current.type)->precedence)
    {
        parser_advance(parser);
        infix_fn infix_rule = get_rule(parser->previous.type)->infix;
        result = infix_rule(parser, result, canAssign);
    }

    return result;
}

static struct ast_node* expression(struct parser* parser)
{
    return parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

#pragma endregion

#pragma region ast_gen

static struct ast_node* statement(struct parser* parser);
static struct ast_node* declaration(struct parser* parser);

static struct ast_node* return_statement(struct parser* parser)
{
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_RETURN_STATEMENT, parser->previous);
    if (parser_match(parser, TOKEN_TYPE_SEMICOLON))
    {
        return node;
    }
    ast_node_append_child(node, expression(parser));
    parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after return");
    return node;
}

static struct ast_node* definition(struct parser* parser, bool statement, bool canAssign, bool inlineDeclaration)
{
    struct ast_node* type_node = parser_build_type(parser);
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token name = parser->previous;
    if (parser_match(parser, TOKEN_TYPE_LEFT_PAREN))
    {
        //function
        struct ast_node* node = ast_node_new(AST_NODE_TYPE_FUNCTION, token_null);
        ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
        ast_node_append_child(node, type_node);
        struct ast_node* arguments = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
        ast_node_append_child(node, arguments);
        //parameters
        if (!parser_check(parser, TOKEN_TYPE_RIGHT_PAREN))
        {
            do
            {
                parser_advance(parser);
                ast_node_append_child(arguments, definition(parser, false, true, true));
            }
            while (parser_match(parser, TOKEN_TYPE_COMMA));
        }
        parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after declaration");
        if (inlineDeclaration)
            ast_node_append_child(node, declaration(parser));
        else if (statement)
            parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after variable definition");
        return node;
    }

    //variable
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_VARIABLE, token_null);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
    ast_node_append_child(node, type_node);
    if (canAssign && parser_match(parser, TOKEN_TYPE_EQUAL))
    {
        ast_node_append_child(node, expression(parser));
    }
    if (statement)
        parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after variable definition");
    return node;
}

static struct ast_node* definition_statement(struct parser* parser)
{
    struct ast_node* node = definition(parser, true, true, true);
    return node;
}

static struct ast_node* struct_statement(struct parser* parser)
{
    struct token type = parser->previous;
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected struct name");
    struct token name = parser->previous;
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_STRUCT, type);
    struct ast_node* symbol = ast_module_get_symbol(parser_scope(parser), name);
    ast_node_append_child(node, symbol);

    if (parser_match(parser, TOKEN_TYPE_COLON))
    {
        do
        {
            parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
            struct token interface = parser->previous;
            ast_node_append_child(node, ast_module_get_symbol(parser->module->symbols, interface));
        }
        while (parser_match(parser, TOKEN_TYPE_COMMA));
    }

    parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct definition");

    parser_push_scope(parser, symbol);

    while (!parser_check(parser, TOKEN_TYPE_RIGHT_BRACE))
    {
        if (parser_match(parser, TOKEN_TYPE_STRUCT))
        {
            ast_node_append_child(node, struct_statement(parser));
        }
        else if (parser_match_type(parser))
        {
            printf("matched type");
            ast_node_free(definition(parser, true, false, true));
        }
        else
        {
            parser_advance(parser);
            parser_error(parser, parser->current, "expected type\n");
        }
    }

    parser_consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");

    parser_pop_scope(parser);

    return node;
}

static struct ast_node* interface_statement(struct parser* parser)
{
    struct token type = parser->previous;
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected interface name");
    struct token name = parser->previous;
    parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after interface definition");
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_INTERFACE, type);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));

    while (!parser_check(parser, TOKEN_TYPE_RIGHT_BRACE))
    {
        parser_advance(parser);
        ast_node_append_child(node, definition(parser, true, false, false));
    }

    parser_consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");
    return node;
}

static struct ast_node* expression_statement(struct parser* parser)
{
    struct ast_node* node = expression(parser);
    parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after expression");
    return node;
}

static struct ast_node* block(struct parser* parser)
{
    struct token token = parser->previous;
    struct ast_node* sequence = ast_node_new(AST_NODE_TYPE_SCOPE, token);
    while (!parser_check(parser, TOKEN_TYPE_RIGHT_BRACE) && !parser_check(parser, TOKEN_TYPE_EOF))
    {
        ast_node_append_child(sequence, declaration(parser));
    }

    parser_consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after block");
    return sequence;
}

static struct ast_node* if_statement(struct parser* parser)
{
    struct token op_token = parser->previous;
    struct ast_node* branch = ast_node_new(AST_NODE_TYPE_IF, op_token);
    parser_consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after if");
    ast_node_append_child(branch, expression(parser));
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after if");
    ast_node_append_child(branch, declaration(parser));
    if (parser_match(parser, TOKEN_TYPE_ELSE))
    {
        ast_node_append_child(branch, declaration(parser));
    }

    return branch;
}

static struct ast_node* while_statement(struct parser* parser)
{
    struct token op_token = parser->previous;
    struct ast_node* loop = ast_node_new(AST_NODE_TYPE_WHILE, op_token);
    parser_consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after while");
    ast_node_append_child(loop, expression(parser));
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after while");
    ast_node_append_child(loop, declaration(parser));

    return loop;
}

static struct ast_node* do_while_statement(struct parser* parser)
{
    struct token op_token = parser->previous;
    struct ast_node* loop = ast_node_new(AST_NODE_TYPE_DO_WHILE, op_token);
    ast_node_append_child(loop, declaration(parser));
    parser_consume(parser, TOKEN_TYPE_WHILE, "expected 'while' statement after do block");
    parser_consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after while");
    ast_node_append_child(loop, expression(parser));
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after while");
    return loop;
}

static struct ast_node* for_statement(struct parser* parser)
{
    struct token op_token = parser->previous;
    struct ast_node* loop = ast_node_new(AST_NODE_TYPE_WHILE, op_token);
    parser_consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after for");
    parser_advance(parser); //move the type to the previous
    struct ast_node* init = definition_statement(parser);
    struct ast_node* condition = expression_statement(parser);
    struct ast_node* incr = expression(parser);
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after for");
    struct ast_node* body = declaration(parser);
    struct ast_node* root = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    ast_node_append_child(root, init);
    ast_node_append_child(root, loop);
    ast_node_append_child(loop, condition);
    struct ast_node* body_group = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    ast_node_append_child(body_group, body);
    ast_node_append_child(body_group, incr);
    ast_node_append_child(loop, body_group);
    return root;
}

static struct ast_node* module_statement(struct parser* parser)
{
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected module name");
    parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after module");
    return NULL;
}

static struct ast_node* region_statement(struct parser* parser) {
    struct token op_token = parser->previous;
    parser_match(parser, TOKEN_TYPE_IDENTIFIER);
    parser_consume(parser, TOKEN_TYPE_LEFT_PAREN, "expected '(' after region");
    struct ast_node* sequence = ast_node_new(AST_NODE_TYPE_SEQUENCE, op_token);
    if (!parser_check(parser, TOKEN_TYPE_RIGHT_PAREN))
    {
        do
        {
            parser_advance(parser);
            ast_node_append_child(sequence, definition(parser, false, false, false));
        }
        while (parser_match(parser, TOKEN_TYPE_COMMA));
    }
    parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after region");
    parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "regions require braces");
    
    // sequences don't being a new scope
    struct token token = parser->previous;
    struct ast_node* body = ast_node_new(AST_NODE_TYPE_SEQUENCE, token);
    while (!parser_check(parser, TOKEN_TYPE_RIGHT_BRACE) && !parser_check(parser, TOKEN_TYPE_EOF))
    {
        ast_node_append_child(body, declaration(parser));
    }

    parser_consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after region");
    ast_node_append_child(sequence, body);
    return sequence;
}

static struct ast_node* statement(struct parser* parser)
{
    if (parser_match(parser, TOKEN_TYPE_RETURN))
    {
        return return_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_IF))
    {
        return if_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_WHILE))
    {
        return while_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_DO))
    {
        return do_while_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_FOR))
    {
        return for_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_MODULE))
    {
        return module_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_IMPORT))
    {
        return module_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_REGION)) 
    {
        return region_statement(parser);    
    }
    if (parser_match(parser, TOKEN_TYPE_LEFT_BRACE))
    {
        return block(parser);
    }
    return expression_statement(parser);
}

static struct ast_node* declaration(struct parser* parser)
{
    if (parser_match(parser, TOKEN_TYPE_STRUCT))
    {
        return struct_statement(parser);
    }
    if (parser_match(parser, TOKEN_TYPE_UNION))
    {
        //TODO: unions
    }
    if (parser_match(parser, TOKEN_TYPE_INTERFACE))
    {
        return interface_statement(parser);
    } //Types
    if (parser_match_type(parser))
    {
        return definition_statement(parser);
    }
    return statement(parser);
}

#pragma endregion

#pragma region passes

static struct ast_module_list* modules_pass(struct lexer** lexers, uint32_t count) {
    struct ast_module_list* modules = ast_module_list_new();
    
    for (int i = 0; i < count; i++) {
        struct lexer* lexer = lexers[i];
        
        struct parser* parser = parser_new(PARSER_STAGE_MODULE_GENERATION, NULL, lexer);
    
        if (parser_match(parser, TOKEN_TYPE_MODULE)) {
            parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected module name after 'module'");
            struct token name = parser->previous;
            
            struct ast_module* module = ast_module_list_find(modules, name);
            if (!module) {
                module = ast_module_new(name);
                ast_module_list_add(modules, module);
            }
            ast_module_add_source(module, lexer);
            continue;
        }
    
        parser_error(parser, parser->current, "file must begin with module definition");
        
        goto fail;
    }
 
    return modules;
    
fail:
    ast_module_list_free(modules);
    return NULL;
}

#pragma endregion

struct ast_module_list* parse(struct lexer** lexers, uint32_t count)
{
    struct ast_module_list* modules = modules_pass(lexers, count);

    if (!modules) {
        goto fail;
    }
    
    if (!dependency_graph_gen(modules)) {
        goto fail;
    }
    
    // create type symbols pass
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
        if (!type_declaration_gen(module)) {
            goto fail;
        }
    }
    
    // populate signatures of type symbols
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
        if (!signature_gen(module)) {
            goto fail;
        }
    }
    
    // gen AST pass
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
    }
    
    return modules;

fail:
    ast_module_list_free(modules);
    return NULL;
}
