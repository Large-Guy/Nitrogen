cmake_minimum_required(VERSION 4.0)
project(compiler C)

set(CMAKE_C_STANDARD 11)

add_executable(compiler src/main.c
        src/chunk.c
        src/chunk.h
        src/lexer.c
        src/lexer.h
        src/parser.c
        src/parser.h)
