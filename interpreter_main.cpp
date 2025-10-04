#include "./backend/interpreter.hpp"
#include "./backend/std/values.hpp"
#include <algorithm>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

scope *root = nullptr;
u_int64_t MAX_ALLOCATIONS = 100;
std::string read_file(std::ifstream &path) {
    std::string content((std::istreambuf_iterator<char>(path)), {});
    return content;
}

// gem ./main.gem
int main(int argc, char *argv[]) {
    std::filesystem::path file_path = argc > 1 ? argv[1] : "";
    std::ifstream file = std::ifstream(file_path);

    if (!file) {
        std::cerr << "File not found: " << file_path;
        exit(1);
    }

    const char *prefix = "alloc=";
    size_t prefix_len = std::strlen(prefix);

    if (argc > 2 && std::strncmp(argv[2], prefix, prefix_len) == 0) {
        std::string input;
        while (true) {
            std::cout << "Are you sure you want to manually set the max "
                         "allocations? (y/n): ";
            std::getline(std::cin, input);

            if (input == "y") {
                break;
            } else {
                exit(1);
            }
        }

        MAX_ALLOCATIONS = std::atoi(argv[2] + prefix_len);
        std::cout << "MAX ALLOCATIONS SET TO " << MAX_ALLOCATIONS << std::endl;
    }

    std::string content = read_file(file);
    parser *parser_class = new parser;
    scope *scope_class = new scope;
    scope_class->file_name = file_path.stem().string();
    root = scope_class;

    define_globals(scope_class);
    garbage_collect();

    astToken ast = parser_class->produceAST(content, file_path.stem().string());
    interpret(ast, scope_class);
    delete parser_class;
    delete scope_class;
}