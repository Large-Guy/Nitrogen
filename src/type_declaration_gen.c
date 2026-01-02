#include "type_declaration_gen.h"

static struct ast_node* type_declaration_struct(struct parser* parser) {
    struct ast_node* symbol = ast_node_new(AST_NODE_TYPE_STRUCT, parser->previous);
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected struct name");
    if (parser->error)
        return false;
    struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, parser->previous);
    ast_node_append_child(symbol, name);
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
