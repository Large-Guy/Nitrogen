#include "parser.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>

struct ast_node* ast_node_new(enum ast_node_type type, struct token token) {
    struct ast_node* node = malloc(sizeof(struct ast_node));
    assert(node);
    node->type = type;
    node->token = token;
    node->children_count = 0;
    node->children_capacity = 1;
    node->children = malloc(sizeof(struct ast_node*) * node->children_capacity);
    return node;
}

void ast_node_free(struct ast_node* node) {
    for (size_t i = 0; i < node->children_count; i++) {
        ast_node_free(node->children[i]);
    }
    free(node->children);
    free(node);
}

void ast_node_append_child(struct ast_node* node, struct ast_node* child) {
    if (node->children_count >= node->children_capacity) {
        node->children_capacity *= 2;
        node->children = realloc(node->children, node->children_capacity * sizeof(struct ast_node*));
        assert(node->children);
    }
    node->children[node->children_count++] = child;
}

struct parser {
    struct lexer* lexer;
    struct token current;
    struct token previous;
};

static void advance(struct parser* parser) {
    parser->previous = parser->current;

    while (true) {
        parser->current = lexer_scan(parser->lexer);
        if (parser->current.type != TOKEN_TYPE_ERROR)
            break;

        //TODO: error out
    }
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

static struct token token_null = {0, NULL, 0, 0};

enum precedence {
    PRECEDENCE_NONE,
    PRECEDENCE_ASSIGNMENT, // =
    PRECEDENCE_OR, // or
    PRECEDENCE_AND, // and
    PRECEDENCE_EQUALITY, // == !=
    PRECEDENCE_COMPARISON, // < > <= >=
    PRECEDENCE_TERM, // + -
    PRECEDENCE_FACTOR, // * /
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


static struct ast_node* expression(struct parser* parser);

static struct ast_node* number(struct parser* parser, bool canAssign);

static struct ast_node* grouping(struct parser* parser, bool canAssign);

static struct ast_node* unary(struct parser* parser, bool canAssign);

static struct ast_node* binary(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* variable(struct parser* parser, bool canAssign);

static struct ast_node* and(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* or(struct parser* parser, struct ast_node* left, bool canAssign);

static struct ast_node* literal(struct parser* parser, bool canAssign);

struct parse_rule rules[] = {
    [TOKEN_TYPE_ERROR] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_EOF] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_LEFT_PAREN] = {grouping, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RIGHT_PAREN] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_LEFT_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RIGHT_BRACE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_LEFT_BRACKET] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_RIGHT_BRACKET] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_SEMICOLON] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_DOT] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_COMMA] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_PLUS] = {NULL, binary, PRECEDENCE_TERM},
    [TOKEN_TYPE_PLUS_PLUS] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_PLUS_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_MINUS] = {unary, binary, PRECEDENCE_TERM},
    [TOKEN_TYPE_MINUS_MINUS] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_MINUS_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_STAR] = {unary, binary, PRECEDENCE_FACTOR},
    [TOKEN_TYPE_STAR_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_SLASH] = {NULL, binary, PRECEDENCE_FACTOR},
    [TOKEN_TYPE_SLASH_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_BANG] = {unary, NULL, PRECEDENCE_TERM},
    [TOKEN_TYPE_BANG_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [TOKEN_TYPE_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_EQUAL_EQUAL] = {NULL, binary, PRECEDENCE_EQUALITY},
    [TOKEN_TYPE_GREATER] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_GREATER_EQUAL] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_LESS] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_LESS_EQUAL] = {NULL, binary, PRECEDENCE_COMPARISON},
    [TOKEN_TYPE_COLON] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_COLON_COLON] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_AND] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_AND_AND] = {NULL, and, PRECEDENCE_AND},
    [TOKEN_TYPE_PIPE] = {NULL, NULL, PRECEDENCE_NONE},
    [TOKEN_TYPE_PIPE_PIPE] = {NULL, or, PRECEDENCE_OR},
    [TOKEN_TYPE_TILDE] = {NULL, NULL, PRECEDENCE_NONE},
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

static struct ast_node* parse_precedence(struct parser* parser, enum precedence precedence);

static struct ast_node* expression(struct parser* parser) {
    return parse_precedence(parser, PRECEDENCE_ASSIGNMENT);
}

// prefix

static struct ast_node* variable(struct parser* parser, bool canAssign) {
    struct token token = parser->previous;
    //TODO: proper type handling
    struct ast_node* variable = ast_node_new(AST_NODE_TYPE_I32, token);
    
    if (canAssign && match(parser, TOKEN_TYPE_EQUAL)) {
        struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
        ast_node_append_child(assignment, variable);
        ast_node_append_child(assignment, expression(parser));
        return assignment;
    }
    return variable;
}

static struct ast_node* number(struct parser* parser, bool canAssign) { //TODO: floating point numbers
    struct token token = parser->previous;
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
        case TOKEN_TYPE_BANG: {
            struct ast_node* node = ast_node_new(AST_NODE_TYPE_INVERSE, token);
            ast_node_append_child(node, operand);
            return node;
        }
        default: {
            fprintf(stderr, "unary unexpected token type, this should never happen: %i\n", token.type);
            return NULL;
        }
    }
}

static struct ast_node* literal(struct parser* parser, bool canAssign) {
    struct token token = parser->previous;

    switch (token.type) {
        case TOKEN_TYPE_NULL:
        case TOKEN_TYPE_TRUE:
        case TOKEN_TYPE_FALSE:
            return ast_node_new(AST_NODE_TYPE_INTEGER, token);
        default:
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

static struct ast_node* parse_precedence(struct parser* parser, enum precedence precedence) {
    advance(parser);
    prefix_fn prefix_rule = get_rule(parser->previous.type)->prefix; //NOTE: this has been changed from the tutorial, keep in mind!
    if (prefix_rule == NULL) {
        fprintf(stderr, "Expect expression."); //TODO: proper error handling
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

static struct ast_node* return_statement(struct parser* parser) {
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_RETURN_STATEMENT, parser->previous);
    if (match(parser, TOKEN_TYPE_SEMICOLON)) {
        return node;
    }
    ast_node_append_child(node, expression(parser));
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after return");
    return node;
}

static struct ast_node* declare_variable(struct parser* parser, enum ast_node_type type) {
    ensure(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token token = parser->current;
    return ast_node_new(type, token);
}

static struct ast_node* expression_statement(struct parser* parser) {
    struct ast_node* node = expression(parser);
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after expression");
    return node;
}

static struct ast_node* statement(struct parser* parser);
static struct ast_node* declaration(struct parser* parser);

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
    if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
        return block(parser);
    }
    return expression_statement(parser);
}

static struct ast_node* declaration(struct parser* parser) {
    //TODO: Implement all of this
    if (match(parser, TOKEN_TYPE_STRUCT)) {
        
    }
    else if (match(parser, TOKEN_TYPE_UNION)) {
        
    }
    else if (match(parser, TOKEN_TYPE_INTERFACE)) {
        
    } //Types
    else if (match(parser, TOKEN_TYPE_I8)) {
        
    }
    else if (match(parser, TOKEN_TYPE_I16)) {
        
    }
    else if (match(parser, TOKEN_TYPE_I32)) {
        return declare_variable(parser, AST_NODE_TYPE_I32_DECLARE);
    }
    else if (match(parser, TOKEN_TYPE_I64)) {
        
    }
    else if (match(parser, TOKEN_TYPE_U8)) {
        
    }
    else if (match(parser, TOKEN_TYPE_U16)) {
        
    }
    else if (match(parser, TOKEN_TYPE_U32)) {
        
    }
    else if (match(parser, TOKEN_TYPE_U64)) {
        
    }
    else if (match(parser, TOKEN_TYPE_ISIZE)) {
        
    }
    else if (match(parser, TOKEN_TYPE_USIZE)) {
        
    }
    else if (match(parser, TOKEN_TYPE_STRING)) {
        
    }
    else {
        return statement(parser);
    }
}

struct ast_node* ast_node_build(struct lexer* lexer) {
    struct ast_node* sequence = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    struct parser parser;
    parser.lexer = lexer;
    advance(&parser);

    while (!match(&parser, TOKEN_TYPE_EOF)) {
        ast_node_append_child(sequence, declaration(&parser));
    }

    return sequence;
}

struct chunk* ast_node_compile(struct ast_node* node) {
    
}

static void debug(struct ast_node* node, int32_t depth) {
    // Generate a color based on the type number
    const char* colors[] = {
        "\033[31m", // red
        "\033[32m", // green
        "\033[33m", // yellow
        "\033[34m", // blue
        "\033[35m", // magenta
        "\033[36m", // cyan
        "\033[37m", // white
    };
    
    for (int i = 0; i < depth; i++) {
        printf("%s| ", colors[i % (sizeof(colors) / sizeof(colors[0]))]);
    }
    
    int color_index = node->type % (sizeof(colors)/sizeof(colors[0]));

    // Print node with type-colored bracket
    printf("%snode\033[0m: [%d] %s %.*s\n",
           "\033[1;37m",           // "node" in bright white
           node->type,
           colors[color_index],    // color based on type
           (int32_t)node->token.length,
           node->token.start);

    for (int i = 0; i < node->children_count; i++) {
        debug(node->children[i], depth + 1);
    }
}

void ast_node_debug(struct ast_node* node) {
    debug(node, 0);
}