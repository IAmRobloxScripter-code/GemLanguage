#ifndef PARSER_HPP
#define PARSER_HPP
#pragma once

#include "lexer.hpp"
#include <memory>
#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <map>
#include <any>

enum class tokenKind
{
    Program,
    Identifier,
    VariableDeclaration,
    FunctionDeclaration,
    IfStmt,
    HistoryDeclaration,
    CallExpr,
    AssignmentExpr,
    MemberExpr,
    ObjectLiteral,
    NumberLiteral,
    StringLiteral,
    BinaryExpr,
    ComparisonExpr,
    UnaryExpr,
    LogicGateExpr,
};

std::string generateRandomString(size_t length);

struct astToken
{
    tokenKind kind;
    std::string value;
    std::shared_ptr<astToken> right;
    std::shared_ptr<astToken> left;
    std::vector<std::shared_ptr<astToken>> elifChain;
    std::vector<std::shared_ptr<astToken>> elseBody;
    std::vector<std::shared_ptr<astToken>> body;
    std::string symbol;
    std::string name;
    std::string op;
    std::vector<std::shared_ptr<astToken>> args;
    std::shared_ptr<astToken> caller;
    std::vector<std::string> params;
    std::vector<std::map<std::string, std::variant<std::shared_ptr<astToken>, std::string, float, int, std::vector<std::string>>>> properties;
    std::shared_ptr<astToken> object;
    std::shared_ptr<astToken> property;
    bool computed;
};

class parser
{
public:
    astToken produceAST(const std::string &source);
    astToken parse_primary_expr();
    astToken parse_var_declaration();
    astToken parse_object_expr();
    astToken parseStmt();
    lexer_token at();
    lexer_token eat();
    lexer_token expect(TokenType type);
    astToken parse_expr();
    void skip_semi_colon();
    astToken parse_additive_expr();
    astToken parse_subtraction_expr();
    astToken parse_multiplicative_expr();
    astToken parse_division_expr();
    astToken parse_power_expr();
    astToken parse_function_declaration();
    astToken parse_call_expr(std::shared_ptr<astToken> caller);
    astToken parse_member_call_expr();
    astToken parse_member_expr();
    astToken parse_assignment_expr();
    std::vector<std::shared_ptr<astToken>> parse_arguments_list();
    std::vector<std::shared_ptr<astToken>> parse_arguments();
    astToken parse_for_loop_stmt();
    astToken parse_if_stmt(bool isELIFChain = false);
    astToken parse_and_keyword();
    astToken parse_or_keyword();
    astToken parse_comparasion_expr();
    astToken parse_unary_expr();
};

#endif