#ifndef NCC_BACKEND_H
#define NCC_BACKEND_H

#include "unit.h"

struct backend;

typedef int(*compile_unit_fn)(struct backend* backend, struct unit* unit);

struct backend {
    compile_unit_fn compile_function;
    compile_unit_fn compile_variable;

    struct ir* out;
};

#endif //NCC_BACKEND_H