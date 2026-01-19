#include "x86_64.h"

static int compile_function(struct backend* backend, struct unit* unit) {
    if (unit->global) {
        fprintf(backend->out, ".globl %s\n", unit->symbol);
    }
    fprintf(backend->out, "%s:\n", unit->symbol);

    fprintf(backend->out, "  ret\n");
    return 0;
}

static int compile_variable(struct backend* backend, struct unit* unit) {
    fprintf(backend->out, ".data\n");
    if (unit->global) {
        fprintf(backend->out, ".globl %s\n", unit->symbol);
    }
    fprintf(backend->out, "%s:\n", unit->symbol);
}

const struct backend x86_64_backend = {compile_function, compile_variable};