#include "module_gen.h"

#include "parser.h"

struct ast_module_list* modules_pass(struct lexer** lexers, uint32_t count) {
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
