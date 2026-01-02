#ifndef COMPILER_TYPE_DECLARATION_H
#define COMPILER_TYPE_DECLARATION_H
#include <stdbool.h>
#include "ast_module.h"
#include "parser.h"

bool type_declaration_gen(struct ast_module* module);

#endif //COMPILER_TYPE_DECLARATION_H