#pragma once
#ifndef PARSER_HPP
#define PARSER_HPP

#include "lexer.hpp"
#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <map>

enum class tokenKind
{   
    Unknown,
    Program,
    Identifier,
    VariableDeclaration,
    FunctionDeclaration,
    IfStmt,
    ForLoopStmt,
    WhileLoopStmt,
    ReturnStmt,
    CallExpr,
    AssignmentExpr,
    MemberExpr,
    ObjectLiteral,
    NumberLiteral,
    StringLiteral,
    BooleanLiteral,
    BinaryExpr,
    ComparisonExpr,
    UnaryExpr,
    LogicGateExpr,
    Keyword,
    Import,
    Export,
    Extern,
    Delete,
    Self,
};

std::string generateRandomString(size_t length);
// ik this struct is big and uses a lot of memory but too late to change 80% of the code now
struct astToken
{
    tokenKind kind = tokenKind::Unknown;
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
    std::variant<std::shared_ptr<astToken>, std::vector<std::shared_ptr<astToken>>> iterator;
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
    astToken parse_while_loop_stmt();
    astToken parse_if_stmt(bool isELIFChain = false);
    astToken parse_return_stmt();
    astToken parse_and_keyword();
    astToken parse_or_keyword();
    astToken parse_comparasion_expr();
    astToken parse_unary_expr();
    astToken parse_reflect();
    astToken parse_shine();
    astToken parse_extern();
};

#endif