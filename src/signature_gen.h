#ifndef COMPILER_SIGNATURE_GEN_H
#define COMPILER_SIGNATURE_GEN_H
#include <stdbool.h>
#include "ast_module.h"

bool signature_gen(struct ast_module* module);

#endif //COMPILER_SIGNATURE_GEN_H