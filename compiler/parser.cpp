#include "parser.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <map>
#include <any>

std::vector<lexer_token> tokens;
astToken parser::produceAST(const std::string &source)
{
    std::vector<astToken> body;
    tokens = tokenize(source);

    while (tokens[0].type != TokenType::EndOfFile)
    {
        body.push_back(parser::parseStmt());
    }

    return astToken{
        .kind = tokenKind::Program,
        .body = body};
};

lexer_token parser::at()
{
    return tokens[0];
}

lexer_token parser::eat()
{
    lexer_token value = tokens[0];
    tokens.erase(tokens.begin());
    return value;
}

lexer_token parser::expect(TokenType type)
{
    if (parser::at().type != type)
    {
        exit(0);
    };

    return parser::eat();
}

void parser::skip_semi_colon()
{
    if (parser::at().type == TokenType::SemiColon)
    {
        parser::eat();
    }
}

astToken parser::parseStmt()
{
    switch (parser::at().type)
    {
    case TokenType::Var:
        return parser::parse_var_declaration();
    default:
        return parser::parse_expr();
    };
}

astToken parser::parse_expr()
{
    return parser::parse_additive_expr();
}

astToken parser::parse_var_declaration()
{
    parser::eat();
    std::string identifier = parser::expect(TokenType::Identifier).value;

    if (parser::at().type == TokenType::Equals)
    {
        parser::eat();
        std::shared_ptr<astToken> value = std::make_shared<astToken>(parser::parseStmt());

        parser::skip_semi_colon();

        return astToken{
            .kind = tokenKind::VariableDeclaration,
            .right = value,
            .name = identifier,
        };
    };

    parser::skip_semi_colon();

    return astToken{
        .kind = tokenKind::VariableDeclaration,
        .right = std::make_shared<astToken> (astToken{
            .kind = tokenKind::NumberLiteral,
            .value = "0"
        }),
        .name = identifier};
}

astToken parser::parse_multiplicative_expr() {
    astToken left = parser::parse_primary_expr();

    while (parser::at().value == "*") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_primary_expr();

        return astToken {
           .kind = tokenKind::BinaryExpr,
           .right = std::make_shared<astToken>(right),
           .left = std::make_shared<astToken>(left),
           .op = op
        };
    }

    return left;
}

astToken parser::parse_subtraction_expr() {
    astToken left = parser::parse_multiplicative_expr();

    while (parser::at().value == "-") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_multiplicative_expr();

        return astToken {
           .kind = tokenKind::BinaryExpr,
           .right = std::make_shared<astToken>(right),
           .left = std::make_shared<astToken>(left),
           .op = op
        };
    }

    return left;
}

astToken parser::parse_additive_expr() {
    astToken left = parser::parse_subtraction_expr();

    while (parser::at().value == "+") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_subtraction_expr();

        return astToken {
           .kind = tokenKind::BinaryExpr,
           .right = std::make_shared<astToken>(right),
           .left = std::make_shared<astToken>(left),
           .op = op
        };
    }

    return left;
}

astToken parser::parse_primary_expr()
{
    TokenType type = parser::at().type;
    switch (type)
    {
    case TokenType::Identifier:
        return astToken {
            .kind = tokenKind::Identifier,
            .value = parser::eat().value
        };
    case TokenType::Number:
        return astToken {
            .kind = tokenKind::NumberLiteral,
            .value = parser::eat().value
        };
    case TokenType::String:
        return astToken {
            .kind = tokenKind::StringLiteral,
            .value = parser::eat().value
         };
    case TokenType::OpenParen: {
        parser::eat();
        astToken value = parser::parseStmt();
        parser::expect(TokenType::CloseParen);
        return value;
    };
    default:
        std::cout << "exit" << std::endl;
        exit(0);
    }
}