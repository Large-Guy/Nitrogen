cmake_minimum_required(VERSION 4.0)
project(compiler C)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

set(CMAKE_C_STANDARD 11)

add_executable(compiler src/main.c
        src/unit.c
        src/unit.h
        src/lexer.c
        src/lexer.h
        src/parser.c
        src/parser.h
        src/ast.c
        src/ast.h
        src/unit_gen.c
        src/unit_gen.h
        src/ast_module.c
        src/ast_module.h
        src/block.c
        src/block.h
        src/ssa.c
        src/ssa.h
        src/ast_gen.c
        src/ast_gen.h
        src/register_table.c
        src/register_table.h)
