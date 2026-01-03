#ifndef COMPILER_MODULE_GEN_H
#define COMPILER_MODULE_GEN_H
#include "ast_module.h"
#include "lexer.h"

struct ast_module_list* modules_pass(struct lexer** lexers, uint32_t count);

#endif //COMPILER_MODULE_GEN_H
