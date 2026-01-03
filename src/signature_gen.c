#include "signature_gen.h"
#include "parser.h"
#include "ast_layout.h"

static struct ast_node* function_arg(struct parser* parser) {
    struct ast_node* type = parser_build_type(parser);
    if (!type) {
        return NULL;
    }
    
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token identifier = parser->previous;
    if (parser->error)
        return NULL;
    
    struct ast_node* arg = ast_node_new(AST_NODE_TYPE_VARIABLE, parser->previous);
    struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, identifier);
    ast_node_append_child(arg, name);
    ast_node_append_child(arg, type);
 
    return arg;
}

static bool struct_symbol_signature(struct parser* parser, bool is_static) {
    struct ast_node* type = parser_build_type(parser);
    if (!type) {
        return false;
    }
    
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token identifier = parser->previous;
    if (parser->error)
        return false;
    
    if (parser_match(parser, TOKEN_TYPE_LEFT_PAREN)) {
        // it's a method
        struct ast_node* function = ast_node_new(is_static ? AST_NODE_TYPE_FUNCTION : AST_NODE_TYPE_METHOD, token_null);
        struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, identifier);
        ast_node_append_child(function, name);
        ast_node_append_child(function, type); // return
        struct ast_node* args = ast_node_new(AST_NODE_TYPE_SEQUENCE, parser->previous);
        ast_node_append_child(function, args);
        
        if (!parser_check(parser, TOKEN_TYPE_RIGHT_PAREN)) {
            do {
                if (parser_match_type(parser)) {
                    struct ast_node* arg = function_arg(parser);
                    ast_node_append_child(args, arg);
                }
                else {
                    parser_error(parser, parser->current, "expected type");
                }
            } while (parser_match(parser, TOKEN_TYPE_COMMA));
        }
        
        parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after argument list");
        if (parser->error)
            return false;
        
        parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after method declaration");
        if (parser->error)
            return false;
        
        skip_block(parser);
        
        ast_node_append_child(parser_scope(parser)->children[is_static ? STRUCT_LAYOUT_STATICS : STRUCT_LAYOUT_MEMBERS], function);
        
        return true;
    }
    
    parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected semicolon after field definition");
    if (parser->error)
        return false;
    
    struct ast_node* field = ast_node_new(is_static ? AST_NODE_TYPE_VARIABLE : AST_NODE_TYPE_FIELD, token_null);
    struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, identifier);
    ast_node_append_child(field, name);
    ast_node_append_child(field, type);
    
    ast_node_append_child(parser_scope(parser)->children[is_static ? STRUCT_LAYOUT_STATICS : STRUCT_LAYOUT_MEMBERS], field);
    
    return true;
}

static bool signature_struct(struct parser* parser) {
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected struct name");
    if (parser->error)
        return false;
    struct ast_node* symbol = ast_module_get_symbol(parser_scope(parser), parser->previous);
    parser_push_scope(parser, symbol);
    
    if (parser_match(parser, TOKEN_TYPE_COLON)) {
        do {
            parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected interface name");
            struct ast_node* interface_symbol = ast_module_get_symbol(parser_scope(parser), parser->previous);
            ast_node_append_child(symbol->children[STRUCT_LAYOUT_IMPLEMENTS], interface_symbol);
        } while (parser_match(parser, TOKEN_TYPE_COMMA));
    }
    
    parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected brace after struct declaration");
    if (parser->error)
        return false;
    
    while (!parser_match(parser, TOKEN_TYPE_RIGHT_BRACE)) {
        if (parser_match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else if (parser_match(parser, TOKEN_TYPE_STRUCT)) {
            if (!signature_struct(parser)) {
                return false;
            }
        }
        else if (parser_match_type(parser)) {
            if (!struct_symbol_signature(parser, false)) {
                return false;
            }
        }
        else if (parser_match(parser, TOKEN_TYPE_STATIC)) {
            if (parser_match_type(parser)) {
                if (!struct_symbol_signature(parser, true)) {
                    return false;
                }
            }
            else {
                parser_error(parser, parser->current, "expected type after static");
                return false;
            }
        }
        else {
            parser_error(parser, parser->current, "unexpected token in struct definition");
            return false;
        }
    }
    parser_pop_scope(parser);
    
    return true;
}

static bool interface_symbol_signature(struct parser* parser, bool is_static) {
    struct ast_node* type = parser_build_type(parser);
    if (!type) {
        return false;
    }
    
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token identifier = parser->previous;
    if (parser->error)
        return false;
    
    if (parser_match(parser, TOKEN_TYPE_LEFT_PAREN)) {
        // it's a method
        struct ast_node* function = ast_node_new(is_static ? AST_NODE_TYPE_ASSOCIATED : AST_NODE_TYPE_ABSTRACT, token_null);
        struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, identifier);
        ast_node_append_child(function, name);
        ast_node_append_child(function, type); // return
        struct ast_node* args = ast_node_new(AST_NODE_TYPE_SEQUENCE, parser->previous);
        ast_node_append_child(function, args);
        
        if (!parser_check(parser, TOKEN_TYPE_RIGHT_PAREN)) {
            do {
                if (parser_match_type(parser)) {
                    struct ast_node* arg = function_arg(parser);
                    ast_node_append_child(args, arg);
                }
                else {
                    parser_error(parser, parser->current, "expected type");
                }
            } while (parser_match(parser, TOKEN_TYPE_COMMA));
        }
        
        parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after argument list");
        if (parser->error)
            return false;
        
        parser_consume(parser, TOKEN_TYPE_SEMICOLON, "implementations on interface methods are not allowed");
        if (parser->error)
            return false;
       
        ast_node_append_child(parser_scope(parser)->children[is_static ? INTERFACE_LAYOUT_ASSOCIATIONS : INTERFACE_LAYOUT_ABSTRACTS], function);
        
        return true;
    }
    
    parser_error(parser, parser->previous, "expected a abstract method, not a field");
    return false;
}

static bool signature_interface(struct parser* parser) {
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected struct name");
    if (parser->error)
        return false;
    struct ast_node* symbol = ast_module_get_symbol(parser_scope(parser), parser->previous);
    parser_push_scope(parser, symbol);
    
    parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected brace after struct declaration");
    if (parser->error)
        return false;
    
    while (!parser_match(parser, TOKEN_TYPE_RIGHT_BRACE)) {
        if (parser_match(parser, TOKEN_TYPE_LEFT_BRACE)) {
            skip_block(parser);
        }
        else if (parser_match(parser, TOKEN_TYPE_STRUCT)) {
            if (!signature_struct(parser)) {
                return false;
            }
        }
        else if (parser_match_type(parser)) {
            if (!interface_symbol_signature(parser, false)) {
                return false;
            }
        }
        else if (parser_match(parser, TOKEN_TYPE_STATIC)) {
            if (parser_match_type(parser)) {
                if (!interface_symbol_signature(parser, true)) {
                    return false;
                }
            }
            else {
                parser_error(parser, parser->current, "expected type after static");
                return false;
            }
        }
        else {
            parser_error(parser, parser->current, "unexpected token in struct definition");
            return false;
        }
    }
    parser_pop_scope(parser);
    
    return true;
}


static bool module_symbol_signature(struct parser* parser, bool is_static) {
    struct ast_node* type = parser_build_type(parser);
    if (!type) {
        return false;
    }
    
    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected variable name");
    struct token identifier = parser->previous;
    if (parser->error)
        return false;
    
    if (parser_match(parser, TOKEN_TYPE_LEFT_PAREN)) {
        //NOTE: is_static actually means the INVERSE here. interesting.
        struct ast_node* function = ast_node_new(is_static ? AST_NODE_TYPE_METHOD : AST_NODE_TYPE_FUNCTION, token_null);
        function->symbol = true;
        struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, identifier);
        ast_node_append_child(function, name);
        ast_node_append_child(function, type); // return
        struct ast_node* args = ast_node_new(AST_NODE_TYPE_SEQUENCE, parser->previous);
        ast_node_append_child(function, args);
        
        if (!parser_check(parser, TOKEN_TYPE_RIGHT_PAREN)) {
            do {
                if (parser_match_type(parser)) {
                    struct ast_node* arg = function_arg(parser);
                    ast_node_append_child(args, arg);
                }
                else {
                    parser_error(parser, parser->current, "expected type");
                }
            } while (parser_match(parser, TOKEN_TYPE_COMMA));
        }
        
        parser_consume(parser, TOKEN_TYPE_RIGHT_PAREN, "expected ')' after argument list");
        if (parser->error)
            return false;
        
        parser_consume(parser, TOKEN_TYPE_LEFT_BRACE, "expected '{' after method declaration");
        if (parser->error)
            return false;
        
        skip_block(parser);
        
        ast_node_append_child(parser_scope(parser), function);
        
        return true;
    }
    
    parser_consume(parser, TOKEN_TYPE_SEMICOLON, "expected semicolon after field definition");
    if (parser->error)
        return false;
    
    //NOTE: is_static actually means the INVERSE here. interesting.
    struct ast_node* field = ast_node_new(is_static ? AST_NODE_TYPE_FIELD : AST_NODE_TYPE_VARIABLE, token_null);
    field->symbol = true;
    struct ast_node* name = ast_node_new(AST_NODE_TYPE_NAME, identifier);
    ast_node_append_child(field, name);
    ast_node_append_child(field, type);
    
    ast_node_append_child(parser_scope(parser), field);
    
    return true;
}

bool signature_gen(struct ast_module* module) {
    for (int i = 0; i < module->lexer_count; i++) {
        struct lexer* lexer = module->lexers[i];
        
        struct parser* parser = parser_new(PARSER_STAGE_TYPE_DEFINITION, module, lexer);
        while (!parser_match(parser, TOKEN_TYPE_EOF)) {
            if (parser_match(parser, TOKEN_TYPE_STRUCT)) {
                if (!signature_struct(parser)) {
                    goto fail;
                }
            }
            else if (parser_match(parser, TOKEN_TYPE_INTERFACE)) {
                if (!signature_interface(parser)) {
                    goto fail;
                }
            }
            else if (parser_match_type(parser)) {
                if (!module_symbol_signature(parser, false)) {
                    return false;
                }
            }
            else if (parser_match(parser, TOKEN_TYPE_STATIC)) {
                if (parser_match_type(parser)) {
                    if (!module_symbol_signature(parser, true)) {
                        return false;
                    }
                }
                else {
                    parser_error(parser, parser->current, "expected type after static");
                    return false;
                }
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
