#ifndef COMPILER_IR_GEN_H
#define COMPILER_IR_GEN_H

#include "unit.h"

struct unit_module* unit_module_generate(struct ast_module* module);

#endif //COMPILER_IR_GEN_H