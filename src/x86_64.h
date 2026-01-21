#ifndef NCC_X86_64_H
#define NCC_X86_64_H
#include "backend.h"

extern const struct backend x86_64_backend;

void debug_x86_64_bytecode(struct chunk* bytecode);

#endif //NCC_X86_64_H