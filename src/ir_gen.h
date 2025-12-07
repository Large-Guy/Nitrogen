#ifndef COMPILER_IR_GEN_H
#define COMPILER_IR_GEN_H

#include "ir.h"
#include "module.h"

struct ir_module* ir_gen_module(struct module* module);

#endif //COMPILER_IR_GEN_H