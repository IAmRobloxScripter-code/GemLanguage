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
    HistoryDeclaration,
    CallExpr,
    MemberExpr,
    ObjectLiteral,
    NumberLiteral,
    StringLiteral,
    BinaryExpr
};

struct astToken
{
    tokenKind kind;
    std::string value;
    std::shared_ptr<astToken> right;
    std::shared_ptr<astToken> left;
    std::vector<astToken> body;
    std::string symbol;
    std::string name;
    std::string op;
};

class parser
{
public:
    astToken produceAST(const std::string &source);
    astToken parse_primary_expr();
    astToken parse_var_declaration();
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
};

#endif