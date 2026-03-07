cmake_minimum_required(VERSION 3.20)
project(clover LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_executable(clover
    src/main.cpp
    src/lex.cpp
    src/token.cpp
    src/util.cpp
    src/sem.cpp

    src/parser/parse.cpp
    src/parser/parse_block.cpp
    src/parser/parse_expression.cpp
    src/parser/parse_extern.cpp
    src/parser/parse_function.cpp
    src/parser/parse_program.cpp
    src/parser/parse_statement.cpp
    src/parser/parse_struct.cpp
    src/parser/parse_type.cpp
    src/parser/parse_var_decl.cpp
)

target_include_directories(clover PRIVATE include)
