// Gem Compiler to C //
#include "compiler.hpp"
#include "../gemSettings.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>

tokenKind resolve_type(astToken &node, const std::string &side) {
    if (side == "left") {
        if (!node.left) {
            return node.kind;
        } else {
            return resolve_type(*node.left, side);
        }
    } else {
        if (!node.right) {
            return node.kind;
        } else {
            return resolve_type(*node.right, side);
        }
    }
}

std::unordered_map<std::string, std::string> templates{
    {"object", "gem_object*"},
    {"bool", "(gem_object_bool*)"},
    {"string", "(gem_object_string*)"},
    {"number", "(gem_object_number)*"},
    {"new_number", "make_number({})"},
    {"new_string", "make_string({})"},
    {"new_bool", "make_bool({})"}};

void gem_compiler::code_gen_body(std::vector<std::shared_ptr<astToken>> body) {
    std::vector<std::string> stored_pointers;

    for (std::shared_ptr<astToken> &token : body) {
        std::optional<std::string> variable = gem_compiler::code_gen(*token);

        if (variable.has_value()) {
            stored_pointers.push_back(variable.value());
        }
    }

    gem_compiler::out << "\n";

    for (std::string &ptr : stored_pointers) {
        gem_compiler::out << ptr << "->references--;\n";
        gem_compiler::out << string_format("gem_check_free({})", ptr) << ";\n";
    }
}

void gem_compiler::code_gen_program(astToken &node) {
    gem_compiler::code_gen_body(node.body);
}

void gem_compiler::code_gen_number(astToken &node) {
    gem_compiler::out << string_format(templates["new_number"], node.value);
}

void gem_compiler::code_gen_bool(astToken &node) {
    gem_compiler::out << string_format(templates["new_bool"], node.value);
}

void gem_compiler::code_gen_string(astToken &node) {
    gem_compiler::out << string_format(templates["new_string"], node.value);
}

std::string gem_compiler::code_gen_var_decl(astToken &node) {
    gem_compiler::out << templates["object"] << " " << node.name << " = ";
    gem_compiler::code_gen(*node.right);
    gem_compiler::out << ";\n" << node.name << "->references++;\n";

    return node.name;
}

void gem_compiler::code_gen_binaryoperation(astToken &node) {
    if (node.op == "+") {
        tokenKind token_type = resolve_type(node, "left");
        if (token_type == tokenKind::NumberLiteral) {
            gem_compiler::out << "number_add";
        } else {
            gem_compiler::out << "string_add";
        }
    } else if (node.op == "-") {
        gem_compiler::out << "number_sub";
    } else if (node.op == "*") {
        gem_compiler::out << "number_mul";
    } else if (node.op == "/") {
        gem_compiler::out << "number_div";
    } else if (node.op == "^") {
        gem_compiler::out << "number_pow";
    } else if (node.op == "%") {
        gem_compiler::out << "number_mod";
    }
    gem_compiler::out << "(";
    gem_compiler::code_gen(*node.left);
    gem_compiler::out << ", ";
    gem_compiler::code_gen(*node.right);
    gem_compiler::out << ")";
}

void gem_compiler::code_gen_assignmentexpr(astToken &node) {
    gem_compiler::code_gen(*node.left);
    gem_compiler::out << " = ";
    gem_compiler::out << "gem_assign(";
    gem_compiler::code_gen(*node.left);
    gem_compiler::out << ", ";
    gem_compiler::code_gen(*node.right);
    gem_compiler::out << ");\n";
}

void gem_compiler::code_gen_conditionals(astToken &node) {
    if (node.op == "==") {
        gem_compiler::out << "gem_equal(";
        gem_compiler::code_gen(*node.left);
        gem_compiler::out << ", ";
        gem_compiler::code_gen(*node.right);
        gem_compiler::out << ")";
    } else if (node.op == "!=") {
        gem_compiler::out << "gem_notEqual(";
        gem_compiler::code_gen(*node.left);
        gem_compiler::out << ", ";
        gem_compiler::code_gen(*node.right);
        gem_compiler::out << ")";
    }
}

void gem_compiler::code_gen_ifstmt(astToken &node) {
    gem_compiler::out << "if (";
    gem_compiler::code_gen(*node.left);
    gem_compiler::out << ") {\n";

    gem_compiler::code_gen_body(node.body);

    gem_compiler::out << "}\n";

    if (node.elifChain.size() > 0) {
        for (auto &elif : node.elifChain) {
            gem_compiler::out << "else if (";
            gem_compiler::code_gen(*elif->left);
            gem_compiler::out << ") {\n";
            gem_compiler::code_gen_body(elif->body);
            gem_compiler::out << "}\n";
        }
    }

    if (node.elseBody.size() > 0) {
        gem_compiler::out << "else {\n";

        gem_compiler::code_gen_body(node.elseBody);

        gem_compiler::out << "}\n";
    }
}

std::optional<std::string> gem_compiler::code_gen(astToken &node) {
    switch (node.kind) {
    case tokenKind::Program:
        gem_compiler::code_gen_program(node);
        break;
    case tokenKind::Identifier:
        gem_compiler::out << node.value;
        break;
    case tokenKind::NumberLiteral:
        gem_compiler::code_gen_number(node);
        break;
    case tokenKind::StringLiteral:
        gem_compiler::code_gen_string(node);
        break;
    case tokenKind::BooleanLiteral:
        gem_compiler::code_gen_bool(node);
        break;
    case tokenKind::VariableDeclaration:
        return gem_compiler::code_gen_var_decl(node);
    case tokenKind::BinaryExpr:
        gem_compiler::code_gen_binaryoperation(node);
        break;
    case tokenKind::AssignmentExpr:
        gem_compiler::code_gen_assignmentexpr(node);
        break;
    case tokenKind::ComparisonExpr:
        gem_compiler::code_gen_conditionals(node);
        break;
    case tokenKind::IfStmt:
        gem_compiler::code_gen_ifstmt(node);
        break;
    default:
        break;
    };

    return std::nullopt;
}

std::string read_file(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error opening the file: " << path << std::endl;
        return "";
    }

    std::string line;
    std::string src;
    while (std::getline(file, line)) {
        src += line + '\n';
    }

    return src;
}

void gem_compiler::link(const std::string &c_file) {
    std::string c_file_source = read_file(c_file);

    out << c_file_source << "\n";
}

std::string gem_compiler::compile(astToken &ast) {
    if (settings.verbose)
        std::cout << "Begining code compilation to C!" << std::endl;
    if (settings.debug)
        gem_compiler::out << "#define O_DEBUG\n";

    gem_compiler::link("./backend/templates/runtime.c");
    gem_compiler::out << "int main() {\n";
    gem_compiler::code_gen(ast);
    gem_compiler::out << "\n}";

    if (settings.verbose)
        std::cout << "Code compilation to C has finished!" << std::endl;

    return gem_compiler::out.str();
}