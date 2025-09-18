cmake_minimum_required(VERSION 3.13)
project(Gem-C)

set(CMAKE_CXX_STANDARD 20)

find_package(fmt REQUIRED)

add_executable(gem
    main.cpp
    ./backend/parser.cpp
    ./backend/lexer.cpp
    ./backend/compiler.cpp
)

target_compile_options(gem PRIVATE -fexceptions)
target_link_libraries(gem PRIVATE fmt::fmt)
