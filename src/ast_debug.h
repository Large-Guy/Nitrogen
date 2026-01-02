#ifndef COMPILER_AST_DEBUG_H
#define COMPILER_AST_DEBUG_H
#include <stdio.h>
#include "ast.h"

void ast_node_debug(FILE* out, struct ast_node* node);

#endif //COMPILER_AST_DEBUG_H