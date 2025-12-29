#ifndef COMPILER_UNIT_DEBUG_H
#define COMPILER_UNIT_DEBUG_H
#include "unit.h"

void unit_debug(struct unit* chunk);

void unit_build_graph(struct unit* chunk, FILE* out);

void unit_module_debug(struct unit_module* module);

void unit_module_debug_graph(struct unit_module* module, FILE* out);


#endif //COMPILER_UNIT_DEBUG_H