#include "compiler.hpp"
#include "../gemSettings.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

std::filesystem::path fileRoot;
void compiler::concat(std::string &str, std::string value) {
    str += value;
}

std::string compiler::spaces() {
    return std::string(compiler::identation * 2, ' ');
}

void compiler::compile_program(astToken &node, bool includeMain) {
    if (includeMain) {
        compiler::concat(bytecode, "function main :\n");
        compiler::identation++;
    }

    if (std::vector<std::shared_ptr<astToken>> &tokens = node.body;
        !tokens.empty()) {
        for (std::shared_ptr<astToken> &token : tokens) {
            compiler::generate(token);
        }
    }

    if (includeMain) {
        compiler::identation--;
        compiler::concat(bytecode, "RET\n");
        compiler::concat(bytecode, "DECLARE_LOCAL main");
    }
}

void compiler::compile_var_declaration(astToken &node) {
    std::string identifier = node.name;

    compiler::generate(node.right, true);
    compiler::concat(bytecode, "\n");
    compiler::concat(
        bytecode, compiler::spaces() + "DECLARE_LOCAL " + identifier + "\n");
}

void compiler::compile_binary_expr(astToken &node) {
    std::string op = node.op;
    compiler::generate(node.left);
    compiler::generate(node.right);

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

void compiler::compile_function_declaration(
    astToken &node, bool isDeclaration) {
    compiler::concat(
        bytecode, compiler::spaces() + "function " + node.name + " ");
    compiler::identation++;

    for (std::string &param : node.params) {
        compiler::concat(bytecode, param + " ");
    }

    compiler::concat(bytecode, ":\n");

    for (std::shared_ptr<astToken> &token : node.body) {
        compiler::generate(token);
    }

    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "RET\n");
    if (!isDeclaration) {
        compiler::concat(
            bytecode, compiler::spaces() + "DECLARE_LOCAL " + node.name + "\n");
    }
}

void compiler::compile_call_expr(astToken &node) {
    std::reverse(node.args.begin(), node.args.end());
    for (std::shared_ptr<astToken> &param : node.args) {
        compiler::generate(param);
    }
    compiler::generate(node.caller);
    compiler::concat(bytecode, compiler::spaces() + "CALL\n");
}

void compiler::compile_identifier(astToken &node) {
    compiler::concat(
        bytecode, compiler::spaces() + "LOAD_LOCAL " + node.value + "\n");
}
using chainType = std::vector<std::variant<std::shared_ptr<astToken>, bool>>;
void compiler::compile_assignment_expr(astToken &node) {
    if (node.left.get()->kind == tokenKind::MemberExpr) {
        std::vector<chainType> chain;
        astToken *current = &*node.left;

        while (current->kind == tokenKind::MemberExpr) {
            chain.push_back({current->property, current->computed});
            current = current->object.get();
        }

        compiler::generate(node.right);

        for (size_t i = 0; i < chain.size(); i++) {
            if (std::get<bool>(chain[i][1]) == true) {
                compiler::generate(
                    std::get<std::shared_ptr<astToken>>(chain[i][0]));
            } else {
                compiler::concat(bytecode,
                    compiler::spaces() + "PUSH \"" +
                        std::get<std::shared_ptr<astToken>>(chain[i][0])
                            ->value +
                        "\"\n");
            }
        }

        compiler::concat(bytecode,
            compiler::spaces() + "LOAD_LOCAL " + current->value + "\n");
        compiler::concat(bytecode,
            compiler::spaces() + "STORE_NESTED_ASSIGNMENT " +
                std::to_string(chain.size()) + "\n");

        compiler::concat(bytecode,
            compiler::spaces() + "STORE_LOCAL " + current->value + "\n");
    } else {
        astToken *current = &node;
        compiler::generate(current->right);
        compiler::concat(bytecode,
            compiler::spaces() + "STORE_LOCAL " + node.left.get()->value +
                "\n");
    }
}

void compiler::compile_object_literal(astToken &node) {
    compiler::concat(bytecode, compiler::spaces() + "NEW_TABLE\n");

    if (!node.properties.empty()) {
        for (auto &property : node.properties) {
            compiler::generate(
                *std::get<std::shared_ptr<astToken>>(property.at("value")),
                true);
            std::string key = std::get<std::string>(property.at("key"));
            if (key[0] != '\"' && key[key.size() - 1] != '\"') {
                key = "\"" + key + "\"";
            }
            compiler::concat(
                bytecode, compiler::spaces() + "PUSH \"" + key + "\"\n");
            compiler::concat(bytecode, compiler::spaces() + "STORE_KEY\n");
        }
    }
}

void compiler::compile_member_expr(astToken &node) {
    std::vector<chainType> chain;
    astToken *current = &node;

    while (current->kind == tokenKind::MemberExpr) {
        chain.push_back({current->property, current->computed});
        current = current->object.get();
    }

    std::reverse(chain.begin(), chain.end());

    compiler::concat(
        bytecode, compiler::spaces() + "LOAD_LOCAL " + current->value + "\n");

    for (size_t i = 0; i < chain.size(); i++) {
        if (std::get<bool>(chain[i][1]) == true) {
            compiler::generate(
                std::get<std::shared_ptr<astToken>>(chain[i][0]));
        } else {
            compiler::concat(bytecode,
                compiler::spaces() + "PUSH \"" +
                    std::get<std::shared_ptr<astToken>>(chain[i][0])->value +
                    "\"\n");
        }
        compiler::concat(bytecode, compiler::spaces() + "LOAD_KEY\n");
    }
}

void compiler::compile_unary_expr(astToken &node) {
    if (node.op == "-") {
        compiler::concat(bytecode, compiler::spaces() + "PUSH 0\n");
        compiler::generate(node.right);
        compiler::concat(bytecode, compiler::spaces() + "SUB\n");
    } else if (node.op == "!") {
        compiler::generate(node.right);
        compiler::concat(bytecode, compiler::spaces() + "NOT\n");
    }
}

void compiler::compile_comparison_expr(astToken &node) {
    compiler::generate(node.left);
    compiler::generate(node.right);
    if (node.op == "==") {
        compiler::concat(bytecode, compiler::spaces() + "EQ\n");
    } else if (node.op == ">=") {
        compiler::concat(bytecode, compiler::spaces() + "GTE\n");
    } else if (node.op == "<=") {
        compiler::concat(bytecode, compiler::spaces() + "LTE\n");
    } else if (node.op == ">") {
        compiler::concat(bytecode, compiler::spaces() + "GT\n");
    } else if (node.op == "<") {
        compiler::concat(bytecode, compiler::spaces() + "LT\n");
    } else if (node.op == "!=") {
        compiler::concat(bytecode, compiler::spaces() + "NOE\n");
    }
}

void compiler::compile_logicgate_expr(astToken &node) {
    compiler::generate(node.left);
    compiler::generate(node.right);

    if (node.op == "and") {
        compiler::concat(bytecode, compiler::spaces() + "AND\n");
    } else {
        compiler::concat(bytecode, compiler::spaces() + "OR\n");
    }
}

void compiler::compile_if_stmt(astToken &node) {
    compiler::concat(bytecode, compiler::spaces() + "IF\n");
    compiler::identation++;
    compiler::generate(node.left);
    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "THEN\n");

    compiler::identation++;

    for (std::shared_ptr<astToken> &epxr : node.body) {
        compiler::generate(epxr);
    }
    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");
    compiler::identation++;

    if (node.elifChain.size() > 0) {
        for (std::shared_ptr<astToken> &elifNode : node.elifChain) {
            compiler::identation--;
            compiler::concat(bytecode, compiler::spaces() + "ELIF\n");
            compiler::identation++;
            compiler::generate(elifNode.get()->left);
            compiler::identation--;
            compiler::concat(bytecode, compiler::spaces() + "THEN\n");
            compiler::identation++;

            for (std::shared_ptr<astToken> &epxr : elifNode.get()->body) {
                compiler::generate(epxr);
            }
            compiler::identation--;
            compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");
            compiler::identation++;
        }
    }

    if (node.elseBody.size() > 0) {
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ELSE\n");
        compiler::identation++;
        for (std::shared_ptr<astToken> &epxr : node.elseBody) {
            compiler::generate(epxr);
        }
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");
    }
}

std::string getMemoryName(size_t bits) {
    static int ID = 0;
    ID++;
    if (std::to_string(ID).size() < bits) {
        return "0x" +
               std::string(
                   static_cast<int>(bits) - std::to_string(ID).size(), '0') +
               std::to_string(ID);
    } else {
        return "0x" + std::to_string(ID);
    }
}

void compiler::compile_forloop_stmt(astToken &node) {
    if (std::holds_alternative<std::vector<std::shared_ptr<astToken>>>(
            node.iterator)) {
        // for range
        auto iterator =
            std::get<std::vector<std::shared_ptr<astToken>>>(node.iterator);
        compiler::generate(iterator[0]);
        compiler::concat(bytecode, compiler::spaces() + "PUSH 1\n");
        compiler::concat(bytecode, compiler::spaces() + "SUB\n");

        compiler::concat(bytecode,
            compiler::spaces() + "DECLARE_LOCAL " + node.params[0] + "\n");

        compiler::concat(bytecode, compiler::spaces() + "LOOP\n");
        compiler::identation++;
        // loop
        compiler::concat(bytecode,
            compiler::spaces() + "LOAD_LOCAL " + node.params[0] + "\n");
        if (iterator.size() >= 3) {
            compiler::generate(iterator[2]);
        } else {
            compiler::concat(bytecode, compiler::spaces() + "PUSH 1" + "\n");
        }
        compiler::concat(bytecode, compiler::spaces() + "ADD\n");
        compiler::concat(bytecode,
            compiler::spaces() + "STORE_LOCAL " + node.params[0] + "\n");
        // end

        compiler::concat(bytecode, compiler::spaces() + "IF\n");
        compiler::identation++;
        compiler::concat(bytecode,
            compiler::spaces() + "LOAD_LOCAL " + node.params[0] + "\n");
        compiler::generate(iterator[1]);
        compiler::concat(bytecode, compiler::spaces() + "GTE\n");

        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "THEN\n");
        compiler::identation++;
        compiler::concat(bytecode, compiler::spaces() + "BREAK\n");
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");

        for (std::shared_ptr<astToken> &epxr : node.body) {
            compiler::generate(epxr);
        }

        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDLOOP\n");
        compiler::concat(
            bytecode, compiler::spaces() + "DELETE " + node.params[0] + "\n");
    } else {
        // for table
        std::string fakeMemoryPtr = getMemoryName(8) + "b";

        compiler::generate(std::get<std::shared_ptr<astToken>>(node.iterator));
        compiler::concat(bytecode,
            compiler::spaces() + "DECLARE_LOCAL " + fakeMemoryPtr + "\n");
        compiler::concat(bytecode, compiler::spaces() + "LOOP\n");
        compiler::identation++;

        compiler::concat(bytecode,
            compiler::spaces() + "LOAD_LOCAL " + fakeMemoryPtr + "\n");
        compiler::concat(bytecode, compiler::spaces() + "CALL\n");

        compiler::concat(bytecode,
            compiler::spaces() + "DECLARE_LOCAL " + node.params[0] + "\n");
        compiler::concat(bytecode,
            compiler::spaces() + "DECLARE_LOCAL " + node.params[1] + "\n");

        compiler::concat(bytecode, compiler::spaces() + "IF\n");
        compiler::identation++;
        compiler::concat(bytecode,
            compiler::spaces() + "LOAD_LOCAL " + node.params[0] + "\n");
        compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL null\n");
        compiler::concat(bytecode, compiler::spaces() + "EQ\n");

        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "THEN\n");
        compiler::identation++;
        compiler::concat(bytecode, compiler::spaces() + "BREAK\n");
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");

        for (std::shared_ptr<astToken> &epxr : node.body) {
            compiler::generate(epxr);
        }

        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDLOOP\n");
        compiler::concat(
            bytecode, compiler::spaces() + "DELETE " + fakeMemoryPtr + "\n");
    }
}

void compiler::compile_return_stmt(astToken &node) {
    if (node.right->kind == tokenKind::FunctionDeclaration) {
        compiler::generate(node.right);
        compiler::concat(bytecode,
            compiler::spaces() + "LOAD_LOCAL " + node.right->name + "\n");
    } else {
        compiler::generate(node.right);
    }
    compiler::concat(bytecode, compiler::spaces() + "RETURN\n");
}

std::filesystem::path resolveImport(
    const std::filesystem::path &currentFile, const std::string &importPath) {
    std::filesystem::path currentDir = currentFile.parent_path();

    std::filesystem::path resolved = currentDir / importPath;

    return std::filesystem::weakly_canonical(resolved);
}

void compiler::compile_reflect(astToken &node) {
    // linker

    // reflect "./Numgem"
    // reflect "./Numgem" :: {abs}
    //  shine stmt

    std::string path = node.left->value;
    auto includes = node.params;

    path.erase(0, 1);
    path.erase(path.size() - 1, 1);

    auto actualPath = resolveImport(fileRoot, path);
    std::ifstream file(actualPath);

    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + actualPath.string());
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    std::string source = buffer.str();

    parser parserInstance;
    astToken ast = parserInstance.produceAST(source);
    astToken exported{.kind = tokenKind::Program, .body = {}};

    for (auto &ast : ast.body) {
        if (ast->kind != tokenKind::Export) {
            continue;
        }
        exported.body.push_back(ast->left);
    }

    if (includes.size() == 0) {
        compiler compilerInstance;
        compilerInstance.identation = 1;
        compilerInstance.compile_program(exported, false);
        bytecode += compilerInstance.bytecode;
    } else {
        for (auto it = exported.body.begin(); it != exported.body.end();) {
            auto include = *it;
            if (std::find(includes.begin(), includes.end(), include->name) ==
                includes.end()) {
                it = exported.body.erase(it);
            } else {
                ++it;
            }
        }

        compiler compilerInstance;
        compilerInstance.identation = 1;
        compilerInstance.compile_program(exported, false);
        bytecode += compilerInstance.bytecode;
    }

    compiler::concat(bytecode, "\n");
}

void compiler::compile_whileloop_stmt(astToken &node) {
    compiler::concat(bytecode, compiler::spaces() + "LOOP\n");
    compiler::identation++;

    astToken tempIf = astToken{.kind = tokenKind::IfStmt,
        .left = node.left,
        .elifChain = {},
        .elseBody = {std::make_shared<astToken>(
            astToken{.kind = tokenKind::Keyword, .value = "break"})},
        .body = node.body};

    compiler::generate(tempIf);

    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "ENDLOOP\n");
}

void compiler::compile_extern(astToken &node) {
    auto refPath = node.right->value;
    refPath.erase(0, 1);
    refPath.erase(refPath.size() - 1, 1);
    auto path = resolveImport(fileRoot, refPath);
    auto params = node.params;
    std::reverse(params.begin(), params.end());

    for (auto &param : params) {
        compiler::concat(bytecode, compiler::spaces() + "PUSH " + param + "\n");
    }

    compiler::concat(
        bytecode, compiler::spaces() + "PUSH " + node.value + "\n");
    compiler::concat(bytecode,
        compiler::spaces() + "PUSH " +
            std::to_string(static_cast<int>(params.size())) + "\n");
    compiler::concat(bytecode,
        compiler::spaces() + "DEFINE " + node.left->value + " " +
            path.string() + "\n");
}

void compiler::generate(astToken &node, bool isDeclaration) {
    switch (node.kind) {
    case tokenKind::NumberLiteral: {
        compiler::concat(
            bytecode, compiler::spaces() + "PUSH " + node.value + "\n");
        break;
    }
    case tokenKind::StringLiteral: {
        compiler::concat(
            bytecode, compiler::spaces() + "PUSH " + node.value + "\n");
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
        compiler::compile_function_declaration(node, isDeclaration);
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
    case tokenKind::ObjectLiteral: {
        compiler::compile_object_literal(node);
        break;
    }
    case tokenKind::MemberExpr: {
        compiler::compile_member_expr(node);
        break;
    }
    case tokenKind::UnaryExpr: {
        compiler::compile_unary_expr(node);
        break;
    }
    case tokenKind::ComparisonExpr: {
        compiler::compile_comparison_expr(node);
        break;
    }
    case tokenKind::LogicGateExpr: {
        compiler::compile_logicgate_expr(node);
        break;
    }
    case tokenKind::IfStmt: {
        compiler::compile_if_stmt(node);
        break;
    }
    case tokenKind::ForLoopStmt: {
        compiler::compile_forloop_stmt(node);
        break;
    }
    case tokenKind::ReturnStmt: {
        compiler::compile_return_stmt(node);
        break;
    }
    case tokenKind::Keyword: {
        std::string str = node.value;
        std::transform(
            str.begin(), str.end(), str.begin(), [](unsigned char c) {
                return std::toupper(c);
            });
        compiler::concat(bytecode, compiler::spaces() + str + "\n");
        break;
    }
    case tokenKind::Import: {
        compiler::compile_reflect(node);
        break;
    }
    case tokenKind::WhileLoopStmt: {
        compiler::compile_whileloop_stmt(node);
        break;
    }
    case tokenKind::Extern: {
        compiler::compile_extern(node);
        break;
    }
    case tokenKind::Delete: {
        compiler::concat(
            bytecode, compiler::spaces() + "DELETE " + node.value + "\n");
        break;
    }
    default:
        throw "Invalid ast found during compilation";
    }
}

void compiler::generate(std::shared_ptr<astToken> &node, bool isDeclaration) {
    if (node) {
        generate(*node, isDeclaration);
    } else {
        throw "Tried to generate from nullptr";
    }
}

std::string compiler::compile(
    astToken &program, const std::filesystem::path &file) {
    fileRoot = file;
    if (settings.verbose)
        std::cout << "Begining to compile ast to bytecode" << std::endl;
    if (settings.verbose && settings.optimize)
        std::cout << "Optimizations have been enabled" << std::endl;
    compiler::generate(program);
    if (settings.verbose)
        std::cout << "Bytecode compilation finished" << std::endl;
    return compiler::bytecode;
}