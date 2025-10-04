#pragma once
#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <string>

enum class TokenType
{
    Number,
    Identifier,
    String,
    Boolean,

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
    WhileLoop,

    Reflect,
    Shine,
    Extern,
    
    In,
    Return,
    Keyword,
    Comment,

    Arrow,
    Delete,
    Self,
    
    EndOfFile,
    Any
};

struct lexer_token
{
    std::string value;
    TokenType type;
    int line;
    int column;
};

std::vector<lexer_token> tokenize(const std::string &sourceCode, const std::string &file_name);

#endif