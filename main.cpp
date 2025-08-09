#include "compiler/compiler.hpp"
#include <fstream>
#include <string>
#include <iostream>

std::string to_string(tokenKind kind) {
    switch (kind) {
        case tokenKind::Program: return "Program";
        case tokenKind::VariableDeclaration: return "VariableDeclaration";
        case tokenKind::Identifier: return "Identifier";
        case tokenKind::NumberLiteral: return "NumberLiteral";
        case tokenKind::StringLiteral: return "StringLiteral";
        default: return "Unknown";
    }
}

int main()
{
    std::ifstream file;
    file.open("main.gem");

    if (!file.is_open())
    {
        std::cerr << "Error opening the file!";
        return 1;
    }

    std::string s;

    while (getline(file, s))
    {
    };

    file.close();

    parser parserInstance;
    astToken ast = parserInstance.produceAST(s);
    compiler compilerInstance;
    std::string bytecode = compilerInstance.compile(ast);

    std::cout << bytecode << std::endl;
    return 0;
}