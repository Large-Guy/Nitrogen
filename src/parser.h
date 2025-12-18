#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "lexer.h"
#include "ast_module.h"

struct ast_module_list parse(struct lexer** lexer, uint32_t count);

#endif //COMPILER_PARSER_H