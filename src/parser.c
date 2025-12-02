#include "parser.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

enum parser_mode {
    PARSER_MODE_FORWARD_PASS,
    PARSER_MODE_TREE_GENERATION,
};

struct module {
    struct token name;
    
    struct lexer** lexers;
    size_t lexer_count;
    size_t lexer_capacity;
    
    struct token* symbols;
    size_t symbols_count;
    size_t symbols_capacity;
};

struct module* module_new(struct token name) {
    struct module* module = malloc(sizeof(struct module));
    assert(module);
    module->name = name;
    module->lexers = malloc(sizeof(struct lexer*));
    assert(module->lexers);
    module->lexer_count = 0;
    module->lexer_capacity = 1;
    module->symbols = malloc(sizeof(struct token));
    assert(module->symbols);
    module->symbols_count = 0;
    module->symbols_capacity = 1;
    return module;
}

void module_free(struct module* module) {
    free(module->lexers);
    free(module->symbols);
    free(module);
}

void module_add_source(struct module* module, struct lexer* lexer) {
    if (module->lexer_count >= module->lexer_capacity) {
        module->lexer_capacity *= 2;
        module->lexers = realloc(module->lexers, module->lexer_capacity * sizeof(struct lexer*));
        assert(module->lexers);
    }

    module->lexers[module->lexer_count++] = lexer;
}

struct parser {
    enum parser_mode mode;
    struct module* module;
    struct lexer* lexer;
    struct token current;
    struct token previous;
};

static void declare_type(struct parser* parser, struct token name) {
    if (parser->module->symbols_count >= parser->module->symbols_capacity) {
        parser->module->symbols_capacity *= 2;
        parser->module->symbols = realloc(parser->module->symbols, sizeof(struct token) * parser->module->symbols_capacity);
        assert(parser->module->symbols);
    }
    parser->module->symbols[parser->module->symbols_count++] = name;
}

static bool type_exists(struct parser* parser, struct token name) {
    if (name.type != TOKEN_TYPE_IDENTIFIER) {
        return false;
    }
    for (size_t i = 0; i < parser->module->symbols_count; i++) {
        if (name.length == parser->module->symbols[i].length &&
            memcmp(name.start, parser->module->symbols[i].start, parser->module->symbols[i].length) == 0) {
            return true;
        }
    }
    return false;
}

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
static struct token token_one = {TOKEN_TYPE_INTEGER, "1", 1, 0};

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
    [TOKEN_TYPE_PERCENT] = {NULL, binary, PRECEDENCE_TERM},
    [TOKEN_TYPE_PERCENT_EQUAL] = {NULL, NULL, PRECEDENCE_NONE},
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
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_ADD, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, expression(parser));
            
            ast_node_append_child(assignment, addition);
            return assignment;
        }
        if (match(parser, TOKEN_TYPE_MINUS_EQUAL)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_SUBTRACT, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, expression(parser));
            
            ast_node_append_child(assignment, addition);
            return assignment;
        }
        if (match(parser, TOKEN_TYPE_STAR_EQUAL)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_MULTIPLY, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, expression(parser));
            
            ast_node_append_child(assignment, addition);
            return assignment;
        }
        if (match(parser, TOKEN_TYPE_SLASH_EQUAL)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_DIVIDE, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, expression(parser));
            
            ast_node_append_child(assignment, addition);
            return assignment;
        }
        if (match(parser, TOKEN_TYPE_PERCENT_EQUAL)) {
            struct ast_node* assignment = ast_node_new(AST_NODE_TYPE_ASSIGN, parser->previous);
            ast_node_append_child(assignment, variable);
            
            struct ast_node* addition = ast_node_new(AST_NODE_TYPE_MODULO, parser->previous);
            ast_node_append_child(addition, left);
            ast_node_append_child(addition, expression(parser));
            
            ast_node_append_child(assignment, addition);
            return assignment;
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

static struct ast_node* return_statement(struct parser* parser) {
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_RETURN_STATEMENT, parser->previous);
    if (match(parser, TOKEN_TYPE_SEMICOLON)) {
        return node;
    }
    ast_node_append_child(node, expression(parser));
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after return");
    return node;
}

static enum ast_node_type token_to_type(struct token token) {
    switch (token.type) {
        case TOKEN_TYPE_VOID:
            return AST_NODE_TYPE_VOID;
        case TOKEN_TYPE_I8:
            return AST_NODE_TYPE_I8;
        case TOKEN_TYPE_I16:
            return AST_NODE_TYPE_I16;
        case TOKEN_TYPE_I32:
            return AST_NODE_TYPE_I32;
        case TOKEN_TYPE_I64:
            return AST_NODE_TYPE_I64;
        case TOKEN_TYPE_U8:
            return AST_NODE_TYPE_U8;
        case TOKEN_TYPE_U16:
            return AST_NODE_TYPE_U16;
        case TOKEN_TYPE_U32:
            return AST_NODE_TYPE_U32;
        case TOKEN_TYPE_U64:
            return AST_NODE_TYPE_U64;
        default:
            fprintf(stderr, "unimplemented or unknown node type");    
    }
}

static struct ast_node* statement(struct parser* parser);
static struct ast_node* declaration(struct parser* parser);

static struct ast_node* definition(struct parser* parser, bool statement, bool canAssign, bool inlineDeclaration) {
    struct token type = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token name = parser->previous;
    if (match(parser, TOKEN_TYPE_LEFT_PAREN)) {
        //function
        struct ast_node* node = ast_node_new(AST_NODE_TYPE_FUNCTION_DECLARATION, type);
        ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
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
        else
            consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after variable definition");
        return node;
    }

    //variable
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_VARIABLE_DECLARATION, type);
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
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
    if (parser->mode == PARSER_MODE_TREE_GENERATION) {
        struct ast_node* node = ast_node_new(AST_NODE_TYPE_STRUCT_DECLARATION, type);
        ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));

        if (match(parser, TOKEN_TYPE_COLON)) {
            do {
                consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
                struct token interface = parser->previous;
                ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_INTERFACE, interface));
            } while (match(parser, TOKEN_TYPE_COMMA));
        }

        consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after struct definition");
        
        while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
            advance(parser);
            ast_node_append_child(node, definition(parser, true, false, true));
        }
        
        consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");
        return node;
    }

    //this is a forward declaration pass
    declare_type(parser, name);
    printf("Type declaration: %.*s\n", (int32_t)name.length, name.start);
    return NULL;
}

static struct ast_node* interface_statement(struct parser* parser) {
    struct token type = parser->previous;
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected interface name");
    struct token name = parser->previous;
    consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after interface definition");
    if (parser->mode == PARSER_MODE_TREE_GENERATION) {
        struct ast_node* node = ast_node_new(AST_NODE_TYPE_INTERFACE_DECLARATION, type);
        ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_NAME, name));
    
        while (!check(parser,TOKEN_TYPE_RIGHT_BRACE)) {
            advance(parser);
            ast_node_append_child(node, definition(parser, true, false, false));
        }
        
        consume(parser, TOKEN_TYPE_RIGHT_BRACE, "expected '}' after struct statement");
        return node;
    }

    declare_type(parser, name);
    printf("Interface declaration: %.*s\n", (int32_t)name.length, name.start);
    return NULL;
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
    struct ast_node* node = ast_node_new(AST_NODE_TYPE_MODULE_STATEMENT, parser->previous);
    consume(parser, TOKEN_TYPE_IDENTIFIER, "expected module name");
    ast_node_append_child(node, ast_node_new(AST_NODE_TYPE_MODULE_NAME, parser->previous));
    consume(parser, TOKEN_TYPE_SEMICOLON, "expected ';' after module");
    return node;
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
    if (match(parser, TOKEN_TYPE_LEFT_BRACE)) {
        return block(parser);
    }
    return expression_statement(parser);
}

static struct ast_node* declaration(struct parser* parser) {
    //TODO: Implement all of this
    if (match(parser, TOKEN_TYPE_STRUCT)) {
        return struct_statement(parser);
    }
    if (match(parser, TOKEN_TYPE_UNION)) {
        
    }
    if (match(parser, TOKEN_TYPE_INTERFACE)) {
        return interface_statement(parser);
    } //Types
    if (match(parser, TOKEN_TYPE_I8) ||
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
        match(parser, TOKEN_TYPE_VOID)
        ) {
        return definition_statement(parser);
    }
    if (type_exists(parser, parser->current)) {
        advance(parser);
        return definition_statement(parser);
    }
    return statement(parser);
}

static void type_pass(struct parser* parser) {
    if (match(parser, TOKEN_TYPE_STRUCT)) {
        struct_statement(parser);
        return;
    }
    if (match(parser, TOKEN_TYPE_INTERFACE)) {
        interface_statement(parser);
    }
    advance(parser);
}

static struct module** module_pass(struct lexer** lexers, uint32_t count) {
    struct parser parser;
    parser.module = NULL;
    parser.mode = PARSER_MODE_FORWARD_PASS;
    parser.lexer = NULL;
    
    struct module** modules = malloc(sizeof(struct module));
    uint32_t modules_count = 0;
    uint32_t modules_capacity = 1;

    while (!match(&parser, TOKEN_TYPE_EOF)) {
        advance(&parser);
        if (match(&parser, TOKEN_TYPE_MODULE)) {
            consume(&parser, TOKEN_TYPE_IDENTIFIER, "expected module name");
            if (modules_count >= modules_capacity) {
                modules_capacity *= 2;
                modules = realloc(modules, modules_capacity);
                assert(modules != NULL);
            }
            modules[modules_count++] = module_new(parser.previous);
            break;
        }
    }
    
    return modules;
}

struct ast_node* ast_node_build(struct lexer* lexer) {
    struct ast_node* sequence = ast_node_new(AST_NODE_TYPE_TRANSLATION_UNIT, token_null);

    
    //resolve modules
    //module_pass(&lexer, 1);

    struct parser parser;
    parser.lexer = lexer;
    parser.mode = PARSER_MODE_FORWARD_PASS;
    parser.module = module_new(token_null);
    advance(&parser);
   
    while (!match(&parser, TOKEN_TYPE_EOF)) {
        type_pass(&parser);
    }

    lexer_reset(lexer);
    parser.mode = PARSER_MODE_TREE_GENERATION;
    advance(&parser);

    while (!match(&parser, TOKEN_TYPE_EOF)) {
        ast_node_append_child(sequence, declaration(&parser));
    }

    free(parser.module);
    return sequence;
}

void recursive_compiler(struct chunk* chunk, struct ast_node* node) {
    
}

struct chunk* ast_node_compile(struct ast_node* node) {
    if (node->type != AST_NODE_TYPE_TRANSLATION_UNIT) {
        fprintf(stderr, "expected translation unit type\n");
        return NULL;
    }
    for (int i = 0; i < node->children_count; i++) {
        struct ast_node* child = node->children[i];
        switch (child->type) {
            case AST_NODE_TYPE_FUNCTION_DECLARATION: {
                
            }
        }
    }
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