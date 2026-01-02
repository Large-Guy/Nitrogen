#ifndef COMPILER_DEPENDENCY_GRAPH_GEN_H
#define COMPILER_DEPENDENCY_GRAPH_GEN_H
#include <stdbool.h>
#include "ast_module.h"
#include "parser.h"

bool dependency_graph_gen(struct ast_module_list* modules);

#endif //COMPILER_DEPENDENCY_GRAPH_GEN_H