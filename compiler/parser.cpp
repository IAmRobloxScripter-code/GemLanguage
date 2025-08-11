#include "parser.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <map>
#include <any>
#include <random>
#include <bits/chrono.h>

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
    case TokenType::Function:
        return parser::parse_function_declaration();
    default:
        return parser::parse_expr();
    };
}

astToken parser::parse_expr()
{
    return parser::parse_assignment_expr();
}

std::string generateRandomString(size_t length) {
    const std::string chars =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<> dist(0, chars.size() - 1);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result += chars[dist(rng)];
    }

    return result;
}

astToken parser::parse_function_declaration() {
    parser::eat();
    std::string identifier = (parser::at().type == TokenType::Identifier) ? parser::eat().value : generateRandomString(10);
    
    std::vector<astToken> args = parser::parse_arguments();
    std::vector<std::string> params;

    for (astToken value : args) {
        params.push_back(value.value);
    }

    parser::expect(TokenType::OpenBrace);
    std::vector<astToken> body;

    while (parser::at().type != TokenType::EndOfFile && parser::at().type != TokenType::CloseBrace)
    {
        body.push_back(parser::parseStmt());
    }
    parser::expect(TokenType::CloseBrace);

    return astToken {
        .kind = tokenKind::FunctionDeclaration,
        .body = body,
        .name = identifier,
        .params = params,
    };
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

astToken parser::parse_assignment_expr() {
    astToken left = parser::parse_additive_expr();

    if (parser::at().type == TokenType::Equals) {
        std::string op = parser::eat().value;
        astToken right = parser::parse_additive_expr();

        return astToken {
            .kind = tokenKind::AssignmentExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left)
        };
    }
    
    return left;
}

astToken parser::parse_power_expr() {
    astToken left = parser::parse_member_call_expr();

    while (parser::at().value == "^" || parser::at().value == "%") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_member_call_expr();

        return astToken {
           .kind = tokenKind::BinaryExpr,
           .right = std::make_shared<astToken>(right),
           .left = std::make_shared<astToken>(left),
           .op = op
        };
    }

    return left;
}

astToken parser::parse_division_expr() {
    astToken left = parser::parse_power_expr();

    while (parser::at().value == "/") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_power_expr();

        return astToken {
           .kind = tokenKind::BinaryExpr,
           .right = std::make_shared<astToken>(right),
           .left = std::make_shared<astToken>(left),
           .op = op
        };
    }

    return left;
}


astToken parser::parse_multiplicative_expr() {
    astToken left = parser::parse_division_expr();

    while (parser::at().value == "*") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_division_expr();

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

/*
function this.parse_arguments_list()
		local args = { this.parseStmt() }

		while this.at().type == "Comma" and this.eat() do
			table.insert(args, this.parseStmt())
		end

		return args
	end

	function this.parse_args()
		this.expect("OpenPar", "( after arguments")
		local args = this.at().type == "ClosedPar" and {} or this.parse_arguments_list()

		this.expect("ClosedPar", ") after arguments")

		return args
	end
*/

std::vector<astToken> parser::parse_arguments_list() {
    std::vector<astToken> args = {parser::parseStmt()};
    
    while (parser::at().type == TokenType::Comma) {
        parser::eat();
        args.push_back(parser::parseStmt());
    }

    return args;
}

std::vector<astToken> parser::parse_arguments() {
    parser::expect(TokenType::OpenParen);
    std::vector<astToken> args = parser::at().type == TokenType::CloseParen ? std::vector<astToken>{} : parser::parse_arguments_list();

    parser::expect(TokenType::CloseParen);

    return args;
}

/*
	function this.parse_call_expr(caller)
		local call_expr = {
			type = "CallExpr",
			caller = caller,
			args = this.parse_args(),
		}

		if this.at().type == "OpenPar" then
			call_expr = this.parse_call_expr(call_expr)
		end

		if this.at() ~= nil and this.at().type == "DotComma" then
			this.eat()
		end

		return call_expr
	end

    function this.parse_member_call_expr()
		local member = this.parse_member_expr()

		if this.at().type == "OpenPar" then
			return this.parse_call_expr(member)
		end

		return member
	end
*/

astToken parser::parse_call_expr(std::shared_ptr<astToken> caller) {
    astToken call_expr {
        .kind = tokenKind::CallExpr,
        .args = parser::parse_arguments(),
        .caller = caller,
    };

    if (parser::at().type == TokenType::OpenParen) {
        call_expr = parser::parse_call_expr(caller);
    }

    parser::skip_semi_colon();
    return call_expr;
}

astToken parser::parse_member_call_expr() {
    astToken member = parser::parse_primary_expr();

    if (parser::at().type == TokenType::OpenParen) {
        return parser::parse_call_expr(std::make_shared<astToken>(member));
    }

    return member;
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