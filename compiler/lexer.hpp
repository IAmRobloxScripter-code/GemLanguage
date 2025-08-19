#ifndef LEXER_HPP
#define LEXER_HPP
#pragma once

#include <vector>
#include <string>

enum class TokenType
{
    Number,
    Identifier,
    String,

    Equals,

    OpenParen,
    CloseParen,
    OpenBracket,
    CloseBracket,
    OpenBrace,
    CloseBrace,

    Colon,
    DoubleColon,
    SemiColon,
    Comma,
    Dot,

    BinaryOperator,
    ComparisonOperator,

    Var,
    Function,
    IfStmt,
    ForLoop,

    Keyword,
    
    EndOfFile
};

struct lexer_token
{
    std::string value;
    TokenType type;
};

std::vector<lexer_token> tokenize(const std::string &);

#endif