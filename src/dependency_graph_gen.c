#include "dependency_graph_gen.h"

bool dependency_graph_gen(struct ast_module_list* modules)  {
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
        for (int j = 0; j < module->lexer_count; j++) {
            struct lexer* lexer = module->lexers[j];
            struct parser* parser = parser_new(PARSER_STAGE_DEPENDENCY_GRAPH, module, lexer);
            
            while (!parser_match(parser, TOKEN_TYPE_EOF)) {
                if (parser_match(parser, TOKEN_TYPE_IMPORT)) {
                    parser_consume(parser, TOKEN_TYPE_IDENTIFIER, "expected import name");
                    
                    if (parser->error)
                        return false;
                    
                    struct token name = parser->previous;
                    struct ast_module* import = ast_module_list_find(modules, name);
                    
                    if (!import) {
                        parser_error(parser, parser->previous, "unknown module symbol");
                        return false;
                    }
                    
                    if (!ast_module_add_dependency(module, import)) {
                        parser_error(parser, parser->previous, "circular dependency in modules");
                        return false;
                    }
                }
                parser_advance(parser);
            }
        }
    }
    return true;
}
