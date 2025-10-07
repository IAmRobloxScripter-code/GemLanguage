cmake_minimum_required(VERSION 3.13)
project(Gem-C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-g")

option(BUILD_COMPILER "Build the compiler executable" OFF)
option(BUILD_INTERPRETER "Build the interpreter executable" ON)

if(BUILD_COMPILER)
    message(STATUS "Building Gem Compiler")
    add_executable(gem_compiler
        compiler_main.cpp
        ./backend/parser.cpp
        ./backend/lexer.cpp
        ./backend/compiler.cpp
    )
    target_compile_options(gem_compiler PRIVATE -fexceptions)
endif()

if(BUILD_INTERPRETER)
    message(STATUS "Building Gem Interpreter")
    add_executable(gem_interpreter
        interpreter_main.cpp
        ./backend/parser.cpp
        ./backend/lexer.cpp
        ./backend/interpreter.cpp
    )
    target_compile_options(gem_interpreter PRIVATE -fexceptions)
endif()
