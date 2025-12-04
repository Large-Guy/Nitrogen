#ifndef COMPILER_IR_GEN_H
#define COMPILER_IR_GEN_H

#include "chunk.h"
#include "module.h"

struct chunk_list* compile_module(struct module* module);

#endif //COMPILER_IR_GEN_H