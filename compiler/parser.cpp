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
    std::vector<std::shared_ptr<astToken>> body;
    tokens = tokenize(source);

    while (tokens[0].type != TokenType::EndOfFile)
    {   
        if (tokens[0].type == TokenType::Comment) {
            parser::eat();
            continue;
        }
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
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
    case TokenType::IfStmt:
        return parser::parse_if_stmt();
    default:
        return parser::parse_expr();
    };
}

astToken parser::parse_expr()
{
    return parser::parse_unary_expr();
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
astToken parser::parse_or_keyword() {
    astToken left = parser::parse_and_keyword();

    while (parser::at().value == "||") {
        parser::eat();
        astToken right = parser::parse_and_keyword();

        left = astToken {
            .kind = tokenKind::LogicGateExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = "or"
        };
    }

    return left;
}

astToken parser::parse_and_keyword() {
    astToken left = parser::parse_comparasion_expr();

    while (parser::at().value == "&&") {
        parser::eat();
        astToken right = parser::parse_comparasion_expr();

        left = astToken {
            .kind = tokenKind::LogicGateExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = "and"
        };
    }

    return left;
}

astToken parser::parse_if_stmt(bool isELIFChain) {
    parser::eat();
    astToken condition = parser::parse_expr();

    parser::expect(TokenType::OpenBrace);
    std::vector<std::shared_ptr<astToken>> body;

    while (parser::at().type != TokenType::EndOfFile && parser::at().type != TokenType::CloseBrace)
    {
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }
    parser::expect(TokenType::CloseBrace);

    std::vector<std::shared_ptr<astToken>> elifChain;
    std::vector<std::shared_ptr<astToken>> elsebody;

    if (isELIFChain == false) {
        if (parser::at().value == "elif") {
            while (parser::at().value == "elif") {
                elifChain.push_back(std::make_shared<astToken>(parser::parse_if_stmt(true)));
            }
        } 
        if (parser::at().value == "else") {
            parser::eat();
            parser::expect(TokenType::OpenBrace);

            while (parser::at().type != TokenType::EndOfFile && parser::at().type != TokenType::CloseBrace)
            {
                elsebody.push_back(std::make_shared<astToken>(parser::parseStmt()));
            }

            parser::expect(TokenType::CloseBrace);
        }
    }

    return astToken {
        .kind = tokenKind::IfStmt,
        .left = std::make_shared<astToken>(condition),
        .elifChain = elifChain,
        .elseBody = elsebody,
        .body = body
    };
}

astToken parser::parse_function_declaration() {
    parser::eat();
    std::string identifier = (parser::at().type == TokenType::Identifier) ? parser::eat().value : generateRandomString(10);
    
    std::vector<std::shared_ptr<astToken>> args = parser::parse_arguments();
    std::vector<std::string> params;

    for (std::shared_ptr<astToken> value : args) {
        params.push_back(value.get()->value);
    }

    parser::expect(TokenType::OpenBrace);
    std::vector<std::shared_ptr<astToken>> body;

    while (parser::at().type != TokenType::EndOfFile && parser::at().type != TokenType::CloseBrace)
    {
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
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
    astToken left = parser::parse_or_keyword();

    if (parser::at().type == TokenType::Equals) {
        std::string op = parser::eat().value;
        astToken right = parser::parse_or_keyword();

        return astToken {
            .kind = tokenKind::AssignmentExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left)
        };
    }
    
    return left;
}
/*
	function this.parse_unary_expr()
		if this.at().value == "-" or this.at().value == "!" then
			local operator = this.eat().value
			this.expect("any", "value after unary expression")
			local value = this.parse_primary_expr()

			return {
				type = "UnaryExpr",
				operator = operator,
				value = value,
			}
		end

		return this.parse_primary_expr()
	end
*/
astToken parser::parse_unary_expr() {
    if (parser::at().value == "-" || parser::at().value == "!") {
        std::string op = parser::eat().value;
        astToken value = parser::parse_assignment_expr();

        return astToken {
            .kind = tokenKind::UnaryExpr,
            .right = std::make_shared<astToken>(value),
            .op = op
        };
    }

    return parser::parse_assignment_expr();
}

astToken parser::parse_comparasion_expr() {
    astToken left = parser::parse_object_expr();

    while (parser::at().value == ">" || parser::at().value == "<" || parser::at().value == ">=" || parser::at().value == "<" || parser::at().value == "==" || parser::at().value == "!=") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_object_expr();

        return astToken {
           .kind = tokenKind::ComparisonExpr,
           .right = std::make_shared<astToken>(right),
           .left = std::make_shared<astToken>(left),
           .op = op
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

std::vector<std::shared_ptr<astToken>> parser::parse_arguments_list() {
    std::vector<std::shared_ptr<astToken>> args;
    args.push_back(std::make_shared<astToken>(parser::parseStmt()));
    
    while (parser::at().type == TokenType::Comma) {
        parser::eat();
        args.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }

    return args;
}

std::vector<std::shared_ptr<astToken>> parser::parse_arguments() {
    parser::expect(TokenType::OpenParen);
    std::vector<std::shared_ptr<astToken>> args = parser::at().type == TokenType::CloseParen ? std::vector<std::shared_ptr<astToken>>{} : parser::parse_arguments_list();

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
    astToken member = parser::parse_member_expr();

    if (parser::at().type == TokenType::OpenParen) {
        return parser::parse_call_expr(std::make_shared<astToken>(member));
    }

    return member;
}

using property = std::map<std::string, std::variant<std::shared_ptr<astToken>, std::string, float, int, std::vector<std::string>>>;

astToken parser::parse_object_expr() {
    if (parser::at().type != TokenType::OpenBrace) {
        return parser::parse_additive_expr();
    }

    parser::eat();
    
    std::vector<property> properties;

    while (parser::at().type != TokenType::EndOfFile && parser::at().type != TokenType::CloseBrace) {
        std::shared_ptr<astToken> key = std::make_shared<astToken>(parser::parseStmt());

        if (parser::at().type == TokenType::Comma) {
            parser::eat();
            property token {
                {"key", std::to_string(static_cast<int>(properties.size()))},
                {"value", key}
            };
            properties.push_back(token);
            continue;
        } else if (parser::at().type == TokenType::CloseBrace) {
            property token {
                {"key", std::to_string(static_cast<int>(properties.size()))},
                {"value", key}
            };
            properties.push_back(token);
            continue;
        };

        parser::expect(TokenType::Colon);

        std::shared_ptr<astToken> value = std::make_shared<astToken>(parser::parseStmt());
        property token {
            {"key", key.get()->value},
            {"value", value}
        };

        properties.push_back(token);

        if (parser::at().type != TokenType::CloseBrace) {
            parser::expect(TokenType::Comma);
        }
    }

    parser::expect(TokenType::CloseBrace);
    parser::skip_semi_colon();

    return astToken {
      .kind = tokenKind::ObjectLiteral,
      .properties = properties,
    };
}

/*	function this.parse_member_expr()
		local object = this.parse_unary_expr()

		while this.at().type == "Dot" or this.at().type == "OpenSqrB" do
			local operator = this.eat().type

			local computed: boolean
			local property

			if operator == "Dot" then
				computed = false
				property = this.parse_unary_expr()

				if property.type ~= "Identifier" then
					this.expect("any", "identifier after member expression")
				end
			else
				computed = true
				property = this.parse_expr()

				this.expect("ClosedSqrB", "] closing member expression")
			end

			object = {
				type = "MemberExpr",
				object = object,
				property = property,
				computed = computed,
			}
		end

		return object
	end*/

astToken parser::parse_member_expr() {
    astToken object = parser::parse_primary_expr();

    while (parser::at().type == TokenType::Dot || parser::at().type == TokenType::OpenBracket) {
        TokenType op = parser::eat().type;

        bool computed;
        astToken property;
        
        if (op == TokenType::Dot) {
            computed = false;
            property = parser::parse_primary_expr();
        } else {
            computed = true;
            property = parser::parse_expr();

            parser::expect(TokenType::CloseBracket);
        }

        astToken newObject {
            .kind = tokenKind::MemberExpr,
            .object = std::make_shared<astToken>(object),
            .property = std::make_shared<astToken>(property),
            .computed = computed
        };

        object = newObject;
    }

    return object;
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
        std::cout << parser::at().value << std::endl;
        std::cout << "exit" << std::endl;
        exit(0);
    }
}