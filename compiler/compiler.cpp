#include "compiler.hpp"
#include <sstream>
#include <algorithm>
#include <iostream>
#include "../gemSettings.hpp"

std::string compiler::bytecode = "";
int compiler::identation = 0;

void compiler::concat(std::string &str, std::string value)
{
    str += value;
}

std::string compiler::spaces()
{
    return std::string(compiler::identation * 2, ' ');
}

void compiler::compile_program(astToken &node)
{
    compiler::concat(bytecode, "function main :\n");
    compiler::identation++;

    if (std::vector<std::shared_ptr<astToken>> &tokens = node.body; !tokens.empty())
    {
        for (std::shared_ptr<astToken> &token : tokens)
        {
            compiler::generate(token);
        }
    }

    compiler::identation--;

    compiler::concat(bytecode, "RET\n");
    compiler::concat(bytecode, "STORE_LOCAL main");
}

void compiler::compile_var_declaration(astToken &node)
{
    std::string identifier = node.name;

    compiler::generate(node.right, true);
    compiler::concat(bytecode, "\n");
    compiler::concat(bytecode, compiler::spaces() + "STORE_LOCAL " + identifier + "\n");
}

void compiler::compile_binary_expr(astToken &node)
{
    std::string op = node.op;
    compiler::generate(node.left);
    compiler::generate(node.right);

    if (op == "+")
    {
        compiler::concat(bytecode, compiler::spaces() + "ADD\n");
    }
    else if (op == "-")
    {
        compiler::concat(bytecode, compiler::spaces() + "SUB\n");
    }
    else if (op == "*")
    {
        compiler::concat(bytecode, compiler::spaces() + "MUL\n");
    }
    else if (op == "/")
    {
        compiler::concat(bytecode, compiler::spaces() + "DIV\n");
    }
    else if (op == "^")
    {
        compiler::concat(bytecode, compiler::spaces() + "POW\n");
    }
    else if (op == "%")
    {
        compiler::concat(bytecode, compiler::spaces() + "MOD\n");
    }
}

void compiler::compile_function_declaration(astToken &node, bool isDeclaration)
{
    compiler::concat(bytecode, compiler::spaces() + "function " + node.name + " ");
    compiler::identation++;

    for (std::string &param : node.params)
    {
        compiler::concat(bytecode, param + " ");
    }

    compiler::concat(bytecode, ":\n");

    for (std::shared_ptr<astToken> &token : node.body)
    {
        compiler::generate(token);
    }

    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "RET\n");
    if (!isDeclaration)
    {
        compiler::concat(bytecode, compiler::spaces() + "STORE_LOCAL " + node.name + "\n");
    }
}

void compiler::compile_call_expr(astToken &node)
{
    for (std::shared_ptr<astToken> &param : node.args)
    {
        compiler::generate(param);
    }
    compiler::generate(node.caller);
    compiler::concat(bytecode, compiler::spaces() + "CALL\n");
}

void compiler::compile_identifier(astToken &node)
{
    compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL " + node.value + "\n");
}

void compiler::compile_assignment_expr(astToken &node)
{
    if (node.left.get()->kind == tokenKind::MemberExpr)
    {
        std::vector<std::string> chain;
        astToken *current = &*node.left;

        while (current->kind == tokenKind::MemberExpr)
        {
            chain.push_back(current->property->value);
            current = current->object.get();
        }

        std::reverse(chain.begin(), chain.end());
        compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL " + current->value + "\n");

        for (size_t i = 0; i < chain.size() - 1; i++)
        {
            compiler::concat(bytecode, compiler::spaces() + "LOAD_KEY \"" + chain[i] + "\"\n");
        }

        compiler::generate(node.right);
        compiler::concat(bytecode, compiler::spaces() + "STORE_KEY \"" + chain.at(chain.size() - 1) + "\"\n");
    }
    else
    {
        compiler::concat(bytecode, compiler::spaces() + "STORE_LOCAL " + node.left.get()->value + "\n");
    }
}

void compiler::compile_object_literal(astToken &node)
{
    compiler::concat(bytecode, compiler::spaces() + "NEW_TABLE\n");

    if (!node.properties.empty())
    {
        for (auto &property : node.properties)
        {
            compiler::generate(*std::get<std::shared_ptr<astToken>>(property.at("value")), true);
            compiler::concat(bytecode, compiler::spaces() + "STORE_KEY " + "\"" + std::get<std::string>(property.at("key")) + "\"\n");
        }
    }
}

void compiler::compile_member_expr(astToken &node)
{
    std::vector<std::string> chain;
    astToken *current = &node;

    while (current->kind == tokenKind::MemberExpr)
    {
        chain.push_back(current->property->value);
        current = current->object.get();
    }

    std::reverse(chain.begin(), chain.end());

    compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL " + current->value + "\n");

    for (size_t i = 0; i < chain.size(); i++)
    {
        compiler::concat(bytecode, compiler::spaces() + "LOAD_KEY \"" + chain[i] + "\"\n");
    }

    // compiler::concat(bytecode, compiler::spaces() + "POP\n");
}

void compiler::compile_unary_expr(astToken &node)
{
    if (node.op == "-")
    {
        compiler::concat(bytecode, compiler::spaces() + "PUSH 0\n");
        compiler::generate(node.right);
        compiler::concat(bytecode, compiler::spaces() + "SUB\n");
    }
    else if (node.op == "!")
    {
        compiler::generate(node.right);
        compiler::concat(bytecode, compiler::spaces() + "NOT\n");
    }
}

void compiler::compile_comparison_expr(astToken &node)
{
    compiler::generate(node.left);
    compiler::generate(node.right);
    if (node.op == "==")
    {
        compiler::concat(bytecode, compiler::spaces() + "EQ\n");
    }
    else if (node.op == ">=")
    {
        compiler::concat(bytecode, compiler::spaces() + "GTE\n");
    }
    else if (node.op == "<=")
    {
        compiler::concat(bytecode, compiler::spaces() + "LTE\n");
    }
    else if (node.op == ">")
    {
        compiler::concat(bytecode, compiler::spaces() + "GT\n");
    }
    else if (node.op == "<")
    {
        compiler::concat(bytecode, compiler::spaces() + "LT\n");
    }
    else if (node.op == "!=")
    {
        compiler::concat(bytecode, compiler::spaces() + "NOE\n");
    }
}

void compiler::compile_logicgate_expr(astToken &node)
{
    compiler::generate(node.left);
    compiler::generate(node.right);

    if (node.op == "and")
    {
        compiler::concat(bytecode, compiler::spaces() + "AND\n");
    }
    else
    {
        compiler::concat(bytecode, compiler::spaces() + "OR\n");
    }
}

void compiler::compile_if_stmt(astToken &node)
{
    compiler::concat(bytecode, compiler::spaces() + "IF\n");
    compiler::identation++;
    compiler::generate(node.left);
    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "THEN\n");

    compiler::identation++;

    for (std::shared_ptr<astToken> &epxr : node.body)
    {
        compiler::generate(epxr);
    }
    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");
    compiler::identation++;

    if (node.elifChain.size() > 0)
    {
        for (std::shared_ptr<astToken> &elifNode : node.elifChain)
        {
            compiler::identation--;
            compiler::concat(bytecode, compiler::spaces() + "ELIF\n");
            compiler::identation++;
            compiler::generate(elifNode.get()->left);
            compiler::identation--;
            compiler::concat(bytecode, compiler::spaces() + "THEN\n");
            compiler::identation++;

            for (std::shared_ptr<astToken> &epxr : elifNode.get()->body)
            {
                compiler::generate(epxr);
            }
            compiler::identation--;
            compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");
            compiler::identation++;
        }
    }

    if (node.elseBody.size() > 0)
    {
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ELSE\n");
        compiler::identation++;
        for (std::shared_ptr<astToken> &epxr : node.elseBody)
        {
            compiler::generate(epxr);
        }
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDIF\n"); 
    }
}

void compiler::compile_forloop_stmt(astToken &node)
{
    if (std::holds_alternative<std::vector<std::shared_ptr<astToken>>>(node.iterator))
    {
        // for range
        auto iterator = std::get<std::vector<std::shared_ptr<astToken>>>(node.iterator);
        compiler::generate(iterator[0]);
        compiler::concat(bytecode, compiler::spaces() + "STORE_LOCAL " + node.params[0] + "\n");

        compiler::concat(bytecode, compiler::spaces() + "LOOP\n");
        compiler::identation++;

        compiler::concat(bytecode, compiler::spaces() + "IF\n");
        compiler::identation++;
        compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL " + node.params[0] + "\n");
        compiler::generate(iterator[1]);
        compiler::concat(bytecode, compiler::spaces() + "GTE\n");

        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "THEN\n");
        compiler::identation++;
        compiler::concat(bytecode, compiler::spaces() + "BREAK\n");
        compiler::identation--;
        compiler::concat(bytecode, compiler::spaces() + "ENDIF\n");

        compiler::concat(bytecode, compiler::spaces() + "LOAD_LOCAL " + node.params[0] + "\n");
        if (iterator.size() >= 3)
        {
            compiler::generate(iterator[2]);
        }
        else
        {
            compiler::concat(bytecode, compiler::spaces() + "PUSH 1" + "\n");
        }

        compiler::concat(bytecode, compiler::spaces() + "ADD\n");
    }
    else
    {
        // for table
    }

    for (std::shared_ptr<astToken> &epxr : node.body)
    {
        compiler::generate(epxr);
    }

    compiler::identation--;
    compiler::concat(bytecode, compiler::spaces() + "ENDLOOP\n");
}

void compiler::generate(astToken &node, bool isDeclaration)
{
    switch (node.kind)
    {
    case tokenKind::NumberLiteral:
    {
        compiler::concat(bytecode, compiler::spaces() + "PUSH " + node.value + "\n");
        break;
    }
    case tokenKind::StringLiteral:
    {
        compiler::concat(bytecode, compiler::spaces() + "PUSH " + "\"" + node.value + "\"\n");
        break;
    }
    case tokenKind::Program:
    {
        compiler::compile_program(node);
        break;
    }
    case tokenKind::CallExpr:
    {
        compiler::compile_call_expr(node);
        break;
    }
    case tokenKind::VariableDeclaration:
    {
        compiler::compile_var_declaration(node);
        break;
    }
    case tokenKind::BinaryExpr:
    {
        compiler::compile_binary_expr(node);
        break;
    }
    case tokenKind::FunctionDeclaration:
    {
        compiler::compile_function_declaration(node, isDeclaration);
        break;
    }
    case tokenKind::Identifier:
    {
        compiler::compile_identifier(node);
        break;
    }
    case tokenKind::AssignmentExpr:
    {
        compiler::compile_assignment_expr(node);
        break;
    }
    case tokenKind::ObjectLiteral:
    {
        compiler::compile_object_literal(node);
        break;
    }
    case tokenKind::MemberExpr:
    {
        compiler::compile_member_expr(node);
        break;
    }
    case tokenKind::UnaryExpr:
    {
        compiler::compile_unary_expr(node);
        break;
    }
    case tokenKind::ComparisonExpr:
    {
        compiler::compile_comparison_expr(node);
        break;
    }
    case tokenKind::LogicGateExpr:
    {
        compiler::compile_logicgate_expr(node);
        break;
    }
    case tokenKind::IfStmt:
    {
        compiler::compile_if_stmt(node);
        break;
    }
    case tokenKind::ForLoopStmt:
    {
        compiler::compile_forloop_stmt(node);
        break;
    }
    default:
        throw "Invalid ast found during compilation";
    }
}

void compiler::generate(std::shared_ptr<astToken> &node, bool isDeclaration)
{
    if (node)
    {
        generate(*node, isDeclaration);
    }
    else
    {
        throw "Tried to generate from nullptr";
    }
}

std::string compiler::compile(astToken &program)
{
    if (settings.verbose)
        std::cout << "Begining to compile ast to bytecode" << std::endl;
    if (settings.verbose && settings.optimize)
        std::cout << "Optimizations have been enabled" << std::endl;
    compiler::generate(program);
    if (settings.verbose)
        std::cout << "Bytecode compilation finished" << std::endl;
    return compiler::bytecode;
}