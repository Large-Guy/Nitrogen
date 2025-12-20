#ifndef COMPILER_IR_GEN_H
#define COMPILER_IR_GEN_H

#include "unit.h"
#include "ast_module.h"

struct unit_module* ir_gen_module(struct ast_module* module);

#endif //COMPILER_IR_GEN_H