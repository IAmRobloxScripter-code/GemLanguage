#ifndef COMPILER_HPP
#define COMPILER_HPP

#pragma once
#include "parser.hpp"

class compiler {
    public:
        static std::string bytecode;
        static int identation;
        std::string compile(astToken &program);
        std::string spaces();

        void concat(std::string &bytecode, std::string value);
        void generate(astToken &node, bool isDeclaration = false);
        void generate(std::shared_ptr<astToken> &node, bool isDeclaration = false);
        void compile_var_declaration(astToken &node);
        void compile_program(astToken &node);
        void compile_binary_expr(astToken &node);
        void compile_function_declaration(astToken &node, bool isDeclaration = false);
        void compile_call_expr(astToken &node);
        void compile_identifier(astToken &node);
        void compile_assignment_expr(astToken &node);
        void compile_object_literal(astToken &node);
        void compile_member_expr(astToken &node);
};

#endif