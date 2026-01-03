#include "type_declaration_gen.h"

static struct ast_node* type_declaration_struct(struct parser* parser) {
    struct ast_node* symbol = ast_node_new(AST_NODE_TYPE_STRUCT, parser->previous);
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected struct name");
    if (parser->error)
        return false;
    struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, parser->previous);
    ast_node_append_child(symbol, name);
    struct ast_node* inherits = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    struct ast_node* statics = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    struct ast_node* members = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    ast_node_append_child(symbol, inherits);
    ast_node_append_child(symbol, members);
    ast_node_append_child(symbol, statics);
    
    if (parser_match(parser, TOKEN_TYPE_COLON)) {
        do {
            parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected identifier");
        } while (parser_match(parser, TOKEN_TYPE_COMMA));
    }
    
    parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected brace after struct declaration");
    if (parser->error)
        goto fail;
    
    while (!parser_match(parser, TOKEN_TYPE_RIGHT_BRACE)) {
        if (parser_match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else if (parser_match(parser, TOKEN_TYPE_STRUCT)) {
            struct ast_node* sub_struct = type_declaration_struct(parser);
            if (!sub_struct) {
                goto fail;
            }
            ast_node_append_child(symbol, sub_struct);
        }
        else {
            parser_advance(parser);
        }
    }
    
    return symbol;
    
fail:
    ast_node_free(symbol);
    return NULL;
}

static struct ast_node* type_declaration_interface(struct parser* parser) {
    struct ast_node* symbol = ast_node_new(AST_NODE_TYPE_INTERFACE, parser->previous);
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected interface name");
    if (parser->error)
        goto fail;
    
    struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, parser->previous);
    ast_node_append_child(symbol, name);
    struct ast_node* abstracts = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    struct ast_node* associations = ast_node_new(AST_NODE_TYPE_SEQUENCE, token_null);
    ast_node_append_child(symbol, abstracts);
    ast_node_append_child(symbol, associations);
    
    
    return symbol;
    
fail:
    ast_node_free(symbol);
    return NULL;
}

bool type_declaration_gen(struct ast_module* module) {
    for (int i = 0; i < module->lexer_count; i++) {
        struct lexer* lexer = module->lexers[i];
        
        struct parser* parser = parser_new(PARSER_STAGE_TYPE_DECLARATION, module, lexer);
        while (!parser_match(parser, TOKEN_TYPE_EOF)) {
            if (parser_match(parser, TOKEN_TYPE_STRUCT)) {
                struct ast_node* node = type_declaration_struct(parser);
                if (!node) {
                    goto fail;
                }
                ast_module_add_symbol(module, node);
            }
            else if (parser_match(parser, TOKEN_TYPE_INTERFACE)) {
                struct ast_node* node = type_declaration_interface(parser);
                if (!node) {
                    goto fail;
                }
                ast_module_add_symbol(module, node);
            }
            else {
                parser_advance(parser);
            }
        }
        
        parser_free(parser);
        continue;
        
        fail:
        parser_free(parser);
        return false;
    }
    return true;
}
