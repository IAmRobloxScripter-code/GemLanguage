#include "compiler.hpp"
#include <sstream>

std::string compiler::bytecode = "";
int compiler::identation = 0;

void compiler::concat(std::string &str, std::string value) {
    str += value;
}

void compiler::compile_program(astToken &node) {
    compiler::concat(bytecode, "function main :\n");
    compiler::identation++;

    if (std::vector<astToken>& tokens = node.body; !tokens.empty()) {
        for (astToken& token : tokens) {
            compiler::generate(token);
        }
    }

    compiler::identation--;

    compiler::concat(bytecode, "RET");
}

std::string compiler::spaces() {
    return std::string(compiler::identation * 2, ' ');
}

void compiler::compile_var_declaration(astToken &node) {
    std::string identifier = node.name;

    compiler::generate(node.right);
    compiler::concat(bytecode, "\n");
    compiler::concat(bytecode, compiler::spaces() + "STORE_LOCAL " + identifier + "\n");
}

void compiler::compile_binary_expr(astToken &node) {
    std::string op = node.op;
    compiler::generate(node.right);
    compiler::generate(node.left);

    if (op == "+") {
        compiler::concat(bytecode, compiler::spaces() + "ADD\n");
    } else if (op == "-") {
        compiler::concat(bytecode, compiler::spaces() + "SUB\n");
    } else if (op == "*") {
        compiler::concat(bytecode, compiler::spaces() + "MUL\n");
    } else if (op == "/") {
        compiler::concat(bytecode, compiler::spaces() + "DIV\n");
    } else if (op == "^") {
        compiler::concat(bytecode, compiler::spaces() + "POW\n");
    } else if (op == "%") {
        compiler::concat(bytecode, compiler::spaces() + "MOD\n");
    }
}

void compiler::compile_function_declaration(astToken &node) {
    compiler::concat(bytecode, compiler::spaces() + "function " + node.name + " ");
    compiler::identation++;

    for (std::string &param : node.params) {
        compiler::concat(bytecode, param + " ");
    }

    compiler::concat(bytecode, ":\n");

    for (astToken& token : node.body) {
        compiler::generate(token);
    }
    
    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "RET\n");
}

void compiler::compile_call_expr(astToken &node) {
    for (astToken& param : node.args) {
        compiler::generate(param);
    }

    compiler::concat(bytecode, compiler::spaces() + "CALL " + node.caller.get()->value + "\n");
}

void compiler::compile_identifier(astToken &node) {
    compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL " + node.value + "\n");
}

void compiler::compile_assignment_expr(astToken &node) {
    compiler::generate(node.right);
    compiler::concat(bytecode, compiler::spaces() + "STORE_LOCAL " + node.left.get()->value + "\n");
}
 
void compiler::generate(astToken &node) {
    switch (node.kind)
    {
    case tokenKind::NumberLiteral: {
        compiler::concat(bytecode, compiler::spaces() + "PUSH " + node.value + "\n");
        break;
    }
    case tokenKind::StringLiteral: {
        compiler::concat(bytecode, compiler::spaces() + "PUSH " + "\"" + node.value + "\"\n");
        break;
    }
    case tokenKind::Program: {
        compiler::compile_program(node);
        break;
    }
    case tokenKind::CallExpr: {
        compiler::compile_call_expr(node);
        break;
    }
    case tokenKind::VariableDeclaration: {
        compiler::compile_var_declaration(node);
        break;
    }
    case tokenKind::BinaryExpr: {
        compiler::compile_binary_expr(node);
        break;
    }
    case tokenKind::FunctionDeclaration: {
        compiler::compile_function_declaration(node);
        break;
    }
    case tokenKind::Identifier: {
        compiler::compile_identifier(node);
        break;
    }
    case tokenKind::AssignmentExpr: {
        compiler::compile_assignment_expr(node);
        break;
    }
    default:
        std::cout << "EXIT" << std::endl;
        exit(0);
    }
}

void compiler::generate(std::shared_ptr<astToken>& node) {
    if (node) {
        generate(*node);
    } else {
        std::cerr << "Error: Tried to generate from nullptr\n";
    }
}

std::string compiler::compile(astToken &program) {
    compiler::generate(program);
    return compiler::bytecode;
}