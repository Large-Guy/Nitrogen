#ifndef COMPILER_AST_GEN_H
#define COMPILER_AST_GEN_H

#include "lexer.h"
#include "ast_module.h"

struct ast_module_list* parse(struct lexer** lexers, uint32_t count);

#endif //COMPILER_AST_GEN_H