#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H

#include "lexer.h"
#include "module.h"

struct module_list parse(struct lexer** lexer, uint32_t count);

#endif //COMPILER_PARSER_H