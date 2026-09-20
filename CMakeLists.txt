cmake_minimum_required(VERSION 3.20)
project(Nyan C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(nyanc
        src/ast.c
        src/diagnostic.c
        src/elf.c
        src/evaluator.c
        src/lexer.c
        src/main.c
        src/parser.c
        src/token.c)

target_include_directories(nyanc PRIVATE src)
target_compile_options(nyanc PRIVATE -Wall -Wextra -Wpedantic)
