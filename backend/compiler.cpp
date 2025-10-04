// Gem Compiler to C //
#include "compiler.hpp"
#include "../gemSettings.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>
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

    *gem_compiler::out << "\n";

    for (std::string &ptr : stored_pointers) {
        *gem_compiler::out << ptr << "->references--;\n";
        *gem_compiler::out << string_format("gem_check_free({})", ptr) << ";\n";
    }
}

void gem_compiler::code_gen_program(astToken &node) {
    gem_compiler::code_gen_body(node.body);
}

void gem_compiler::code_gen_number(astToken &node) {
    *gem_compiler::out << string_format(templates["new_number"], node.value);
}

void gem_compiler::code_gen_bool(astToken &node) {
    *gem_compiler::out << string_format(templates["new_bool"], node.value);
}

void gem_compiler::code_gen_string(astToken &node) {
    *gem_compiler::out << string_format(templates["new_string"], node.value);
}

std::string gem_compiler::code_gen_var_decl(astToken &node) {
    *gem_compiler::out << templates["object"] << " " << node.name << " = ";
    gem_compiler::code_gen(*node.right);
    *gem_compiler::out << ";\n" << node.name << "->references++;\n";

    return node.name;
}

void gem_compiler::code_gen_binaryoperation(astToken &node) {
    if (node.op == "+") {
        *gem_compiler::out << "gem_add";
    } else if (node.op == "-") {
        *gem_compiler::out << "number_sub";
    } else if (node.op == "*") {
        *gem_compiler::out << "number_mul";
    } else if (node.op == "/") {
        *gem_compiler::out << "number_div";
    } else if (node.op == "^") {
        *gem_compiler::out << "number_pow";
    } else if (node.op == "%") {
        *gem_compiler::out << "number_mod";
    }
    *gem_compiler::out << "(st, ";
    gem_compiler::code_gen(*node.left);
    *gem_compiler::out << ", ";
    gem_compiler::code_gen(*node.right);
    *gem_compiler::out << ")";
}

void gem_compiler::code_gen_assignmentexpr(astToken &node) {
    gem_compiler::code_gen(*node.left);
    *gem_compiler::out << " = ";
    *gem_compiler::out << "gem_assign(";
    gem_compiler::code_gen(*node.left);
    *gem_compiler::out << ", ";
    gem_compiler::code_gen(*node.right);
    *gem_compiler::out << ");\n";
}

void gem_compiler::code_gen_conditionals(astToken &node) {
    if (node.op == "==") {
        *gem_compiler::out << "gem_equal(st, ";
        gem_compiler::code_gen(*node.left);
        *gem_compiler::out << ", ";
        gem_compiler::code_gen(*node.right);
        *gem_compiler::out << ")";
    } else if (node.op == "!=") {
        *gem_compiler::out << "gem_notEqual(st, ";
        gem_compiler::code_gen(*node.left);
        *gem_compiler::out << ", ";
        gem_compiler::code_gen(*node.right);
        *gem_compiler::out << ")";
    }
}

void gem_compiler::code_gen_ifstmt(astToken &node) {
    *gem_compiler::out << "if (";
    gem_compiler::code_gen(*node.left);
    *gem_compiler::out << ") {\n";

    gem_compiler::code_gen_body(node.body);

    *gem_compiler::out << "}\n";

    if (node.elifChain.size() > 0) {
        for (auto &elif : node.elifChain) {
            *gem_compiler::out << "else if (";
            gem_compiler::code_gen(*elif->left);
            *gem_compiler::out << ") {\n";
            gem_compiler::code_gen_body(elif->body);
            *gem_compiler::out << "}\n";
        }
    }

    if (node.elseBody.size() > 0) {
        *gem_compiler::out << "else {\n";

        gem_compiler::code_gen_body(node.elseBody);

        *gem_compiler::out << "}\n";
    }
}

void gem_compiler::code_gen_forloop(astToken &node) {
    if (std::holds_alternative<std::vector<std::shared_ptr<astToken>>>(
            node.iterator)) {
        auto iterator =
            std::get<std::vector<std::shared_ptr<astToken>>>(node.iterator);
        *gem_compiler::out << "for (" << templates["object"] << node.params[0]
                           << "=";
        gem_compiler::code_gen(*iterator[0]);
        *gem_compiler::out << "; ";
        *gem_compiler::out << "number_lessThan(";
        *gem_compiler::out << node.params[0] << ", ";
        gem_compiler::code_gen(*iterator[1]);
        *gem_compiler::out << "); " << node.params[0] << "+=";
    }

    *gem_compiler::out << ") {\n";

    gem_compiler::code_gen_body(node.body);

    *gem_compiler::out << "}\n";
}

void search_for_identifiers_in_node(
    astToken &node, std::vector<std::string> &container);

void search_for_identifiers_in_body(
    std::vector<std::shared_ptr<astToken>> &body,
    std::vector<std::string> &container) {
    for (auto &node : body) {
        search_for_identifiers_in_node(*node, container);
    }
}

void search_for_identifiers_in_node(
    astToken &node, std::vector<std::string> &container) {

    switch (node.kind) {
    case tokenKind::Identifier:
        container.push_back(node.value);
        break;
    case tokenKind::ForLoopStmt: {
        if (std::holds_alternative<std::shared_ptr<astToken>>(node.iterator)) {
            auto ptr = std::get<std::shared_ptr<astToken>>(node.iterator);
            search_for_identifiers_in_node(*ptr, container);
        } else {
            auto ptr =
                std::get<std::vector<std::shared_ptr<astToken>>>(node.iterator);
            search_for_identifiers_in_body(ptr, container);
        }
        search_for_identifiers_in_body(node.body, container);

        break;
    }
    case tokenKind::ObjectLiteral: {
        for (auto &prop : node.properties) {
            search_for_identifiers_in_node(*prop.value, container);
        }
        break;
    }
    default:
        if (node.left) {
            search_for_identifiers_in_node(*node.left, container);
        }
        if (node.right) {
            search_for_identifiers_in_node(*node.right, container);
        }
        if (node.caller) {
            search_for_identifiers_in_node(*node.caller, container);
        }
        if (node.object) {
            search_for_identifiers_in_node(*node.object, container);
        }
        if (node.property) {
            search_for_identifiers_in_node(*node.property, container);
        }
        search_for_identifiers_in_body(node.body, container);
        search_for_identifiers_in_body(node.elseBody, container);
        search_for_identifiers_in_body(node.elifChain, container);
        search_for_identifiers_in_body(node.args, container);

        break;
    }
}

std::optional<std::string> gem_compiler::code_gen_function(astToken &node) {
    // enviroment copy
    std::vector<std::string> declared_variables;
    std::vector<std::string> internals;
    std::vector<std::string> container;

    for (auto &nested_variable : node.body) {
        if (nested_variable->kind == tokenKind::VariableDeclaration) {
            declared_variables.push_back(nested_variable->name);
        }
    }

    for (auto &nested_variable : node.params) {
        declared_variables.push_back(nested_variable);
    }

    search_for_identifiers_in_node(node, container);

    for (auto &identifier : container) {
        if (std::find(declared_variables.begin(),
                declared_variables.end(),
                identifier) == declared_variables.end()) {
            internals.push_back(identifier);
        }
    }

    std::string fnName = node.name + "_inner_internals";
    *gem_compiler::out << "gem_object** " << fnName
                       << " = "
                          "malloc(sizeof(gem_object*) * "
                       << internals.size() << ");\n";
    int index = 0;

    for (std::string &internal : internals) {
        *gem_compiler::out << fnName << "[" << index << "] = copy_object("
                           << internal << ");\n"
                           << fnName << "[" << index << "]->references++;\n";
        index++;
    }
    // declare it
    gem_compiler::make_stream();

    *gem_compiler::out
        << templates["object"] << node.name
        << "(stack_trace* st, gem_object** internals, gem_object** "
           "arguments) {\n";
    /*st->FileName = "main.gem";

    gem_object* index = make_number(20);
    index->references++;

    //only 1 internal!
    gem_object** func_inner_internals = malloc(sizeof(gem_object*) * 1);
    func_inner_internals[0] = copy_object(index);
    func_inner_internals[0]->references++;

    gem_object* func_inner = make_function(func_inner_internals, 1,
    iterator_nest_1, 0);

    index->references--;
    gem_check_free(index);
    */

    *gem_compiler::out << "st->FileName = \"" << gem_compiler::file_name
                       << "\";\n";
    *gem_compiler::out << "st->Line = " << node.line << ";"
                       << "trace_add_trace(st, " << node.line
                       << ", \"in function <" << node.name << "> \", false, \""
                       << gem_compiler::file_name << "\");\n";
    /*	gem_object* index = internals[0];
    st->FileName = "main.gem";

    st->Line = 4;
    trace_add_trace(st, 4, "in function <iterator:3>", false, "main.gem");
    index = gem_assign(index, gem_add(st, index, make_number(1)) );
    back(st);

    //set internals, NEVER check_free internals
    internals[0] = index;
    return index;
    return make_nil();
}*/
    int internal_index = 0;
    for (auto &outside_variable : internals) {
        *gem_compiler::out << templates["object"] << outside_variable
                           << " = internals[" << internal_index << "];\n";
        internal_index++;
    }

    gem_compiler::code_gen_body(node.body);
    *gem_compiler::out << "back(st);\n";
    *gem_compiler::out << "\nreturn make_nil();\n};";
    std::string function = (*gem_compiler::out).str();

    gem_compiler::free_stream();
    gem_compiler::add_header(function);
    //	gem_object* func_inner = make_function(func_inner_internals, 1,
    // iterator_nest_1, 0);
    *gem_compiler::out << templates["object"] << " " << node.name
                       << " = make_function("
                       << (internals.size() > 0 ? fnName : "NULL") << ", "
                       << internals.size() << ", " << node.name << ", "
                       << node.params.size() << ");\n";
    return node.name;
}

std::optional<std::string> gem_compiler::code_gen(astToken &node) {
    switch (node.kind) {
    case tokenKind::Program:
        gem_compiler::code_gen_program(node);
        break;
    case tokenKind::Identifier:
        *gem_compiler::out << node.value;
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
    case tokenKind::ForLoopStmt:
        gem_compiler::code_gen_forloop(node);
        break;
    case tokenKind::ReturnStmt:
        *gem_compiler::out << "back(st);\n";
        *gem_compiler::out << "return ";
        gem_compiler::code_gen(*node.right);
        *gem_compiler::out << ";\n";
        break;
    case tokenKind::FunctionDeclaration:
        return gem_compiler::code_gen_function(node);
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
    gem_compiler::add_header(c_file_source);
}

void gem_compiler::add_header(const std::string &header) {
    std::string content = (*gem_compiler::out).str();
    (*gem_compiler::out).str("");
    *gem_compiler::out << header << "\n" << content;
}

std::string gem_compiler::compile(astToken &ast) {
    if (settings.verbose)
        std::cout << "Begining code compilation to C!" << std::endl;
    gem_compiler::make_stream();
    *gem_compiler::out << "int main() {\n";
    *gem_compiler::out << "stack_trace* st = create_stack_trace();\n"
                       << "st->FileName = \"" << gem_compiler::file_name
                       << "\";\n"
                       << "st->Line = " << ast.line << ";\n"
                       << "trace_add_trace(st, " << ast.line
                       << ", \"in main chunk\", false, \""
                       << gem_compiler::file_name << "\");\n";
    gem_compiler::code_gen(ast);
    *gem_compiler::out << "\nback(st);\ndestroy_stack(st);\n}";

    gem_compiler::link("./backend/templates/runtime.c");
    if (settings.debug)
        gem_compiler::add_header("#define O_DEBUG");

    if (settings.verbose)
        std::cout << "Code compilation to C has finished!" << std::endl;

    return (*gem_compiler::out).str();
}