cmake_minimum_required(VERSION 4.0)
project(compiler C)

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

set(CMAKE_C_STANDARD 11)

add_executable(compiler src/main.c
        src/chunk.c
        src/chunk.h
        src/lexer.c
        src/lexer.h
        src/parser.c
        src/parser.h
        src/ast.c
        src/ast.h
        src/ir_gen.c
        src/ir_gen.h
        src/module.c
        src/module.h)
