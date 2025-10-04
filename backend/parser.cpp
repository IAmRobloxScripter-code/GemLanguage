#include "parser.hpp"
#include "../gemSettings.hpp"
#include "debugger.hpp"
#include "magic_enum/magic_enum.hpp"
#include <any>
#include <bits/chrono.h>
#include <deque>
#include <iostream>
#include <map>
#include <optional>
#include <random>
#include <string>
#include <variant>
#include <vector>

std::deque<lexer_token> tokens;
std::vector<std::string> lines_of_code;
std::string file_name;

std::string parser::get_line() {
    std::string line;
    uint64_t line_count = parser::at().line;
    uint64_t index = 0;
    while (tokens[index].line == line_count) {
        line += tokens[index].value + " ";
        index++;
    }

    return line;
}

astToken parser::produceAST(
    const std::string &source, const std::string &name) {
    tokens.clear();
    lines_of_code.clear();
    file_name = name;
    std::vector<std::shared_ptr<astToken>> body;
    if (settings.verbose)
        std::cout << "Tokenizing file content" << std::endl;

    auto v = tokenize(source, file_name);
    tokens = std::deque(v.begin(), v.end());
    int eof_line = tokens.at(tokens.size() - 1).line;

    for (size_t line = 1; line < eof_line; ++line) {
        lines_of_code.push_back(parser::get_line());
    }

    if (settings.verbose)
        std::cout << "Tokenizing file content has been finished" << std::endl;
    if (settings.verbose)
        std::cout << "Parsing tokens begin" << std::endl;

    while (tokens[0].type != TokenType::EndOfFile) {
        if (tokens[0].type == TokenType::Comment) {
            parser::eat();
            continue;
        }
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }

    if (settings.verbose)
        std::cout << "Parsing tokens finish" << std::endl;

    return astToken{.kind = tokenKind::Program, .body = body};
};

lexer_token parser::at() {
    return tokens[0];
}

lexer_token parser::eat() {
    lexer_token value = tokens.front();
    tokens.pop_front();
    return value;
}

/*lexer_token parser::eat() {
    lexer_token value = tokens[0];
    tokens.erase(tokens.begin());
    return value;
}*/

lexer_token parser::expect(TokenType type, std::string kindof) {
    if (type == TokenType::Any) {
        if (tokens.size() == 0 || tokens[0].type == TokenType::EndOfFile) {
            std::string full_line = lines_of_code[lines_of_code.size() - 1];
            std::string message =
                add_pointers("~", full_line, 0, full_line.size());
            error(error_type::parsing_error,
                message,
                file_name,
                lines_of_code.size() - 1,
                "Expected " + kindof + " after statement/expression!");
            exit(1);
        } else {
            return parser::at();
        }
    };

    if (parser::at().type != type) {
        int line = parser::at().line - 1;
        std::string full_line;

        if (line > lines_of_code.size() - 1) {
            full_line = lines_of_code[lines_of_code.size() - 1];
        } else {
            full_line = lines_of_code[line];
        }

        std::string message = add_pointers(
            "~", full_line, parser::at().column - 1, parser::at().value.size());
        error(error_type::parsing_error,
            message,
            file_name,
            parser::at().line,
            "Expected " + std::string(magic_enum::enum_name(type)) + ", got " +
                std::string(magic_enum::enum_name(parser::at().type)) + "!");
        exit(1);
    };

    return parser::eat();
}

void parser::skip_semi_colon() {
    if (parser::at().type == TokenType::SemiColon) {
        parser::eat();
    }
}

astToken parser::parseStmt() {
    switch (parser::at().type) {
    case TokenType::Var:
        return parser::parse_var_declaration();
    case TokenType::Function:
        return parser::parse_function_declaration();
    case TokenType::IfStmt:
        return parser::parse_if_stmt();
    case TokenType::ForLoop:
        return parser::parse_for_loop_stmt();
    case TokenType::WhileLoop:
        return parser::parse_while_loop_stmt();
    case TokenType::Return:
        return parser::parse_return_stmt();
    case TokenType::Keyword: {
        auto token = parser::eat();
        parser::skip_semi_colon();
        return astToken{.kind = tokenKind::Keyword,
            .value = token.value,
            .line = token.line};
    }
    case TokenType::Reflect:
        return parser::parse_reflect();
    case TokenType::Shine:
        return parser::parse_shine();
    case TokenType::Extern:
        return parser::parse_extern();
    default:
        return parser::parse_expr();
    };
}

astToken parser::parse_expr() {
    return parser::parse_unary_expr();
}

astToken parser::parse_extern() {
    int line = parser::at().line;

    parser::eat();

    astToken identifier = parser::parse_primary_expr();
    parser::expect(TokenType::DoubleColon);
    astToken path = parser::parse_primary_expr();
    parser::expect(TokenType::DoubleColon);

    std::vector<std::shared_ptr<astToken>> args = parser::parse_arguments();
    std::vector<std::string> inputTypes;

    for (std::shared_ptr<astToken> value : args) {
        inputTypes.push_back(value.get()->value);
    }

    parser::expect(TokenType::Arrow);

    astToken returnType = parser::parse_primary_expr();

    parser::skip_semi_colon();

    return astToken{.kind = tokenKind::Extern,
        .value = returnType.value,
        .right = std::make_shared<astToken>(path),
        .left = std::make_shared<astToken>(identifier),
        .name = identifier.value,
        .params = inputTypes,
        .line = line};
}

astToken parser::parse_shine() {
    int line = parser::at().line;

    parser::eat();
    astToken stmt = parser::parseStmt();
    parser::skip_semi_colon();

    return astToken{.kind = tokenKind::Export,
        .left = std::make_shared<astToken>(stmt),
        .line = line};
}

astToken parser::parse_reflect() {
    int line = parser::at().line;

    parser::eat();
    astToken path = parser::parse_primary_expr();

    if (parser::at().type == TokenType::DoubleColon) {
        parser::eat();
        // we parsing those funny things cuz i cant use tables since i cant do
        // {abc} or else it will give a null value cuz identifier stuff
        parser::expect(TokenType::OpenBrace);
        std::vector<std::string> include;

        while (parser::at().type != TokenType::EndOfFile &&
               parser::at().type != TokenType::CloseBrace) {
            include.push_back(parser::eat().value);
            if (parser::at().type != TokenType::CloseBrace)
                parser::expect(TokenType::Comma);
        }

        parser::expect(TokenType::CloseBrace);
        parser::skip_semi_colon();

        return astToken{.kind = tokenKind::Import,
            .left = std::make_shared<astToken>(path),
            .params = include,
            .line = line};
    } else {
        parser::skip_semi_colon();

        return astToken{.kind = tokenKind::Import,
            .left = std::make_shared<astToken>(path),
            .line = line};
    }
}

astToken parser::parse_return_stmt() {
    int line = parser::at().line;

    parser::eat();
    parser::expect(TokenType::Any);
    auto right = std::make_shared<astToken>(parser::parseStmt());
    parser::skip_semi_colon();
    return astToken{
        .kind = tokenKind::ReturnStmt, .right = right, .line = line};
}

astToken parser::parse_or_keyword() {
    astToken left = parser::parse_and_keyword();

    while (parser::at().value == "||") {
        parser::eat();
        astToken right = parser::parse_and_keyword();

        left = astToken{.kind = tokenKind::LogicGateExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = "or",
            .line = left.line};
    }

    return left;
}

astToken parser::parse_and_keyword() {

    astToken left = parser::parse_comparasion_expr();

    while (parser::at().value == "&&") {
        parser::eat();
        astToken right = parser::parse_comparasion_expr();

        left = astToken{.kind = tokenKind::LogicGateExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = "and",
            .line = left.line};
    }

    return left;
}

astToken parser::parse_while_loop_stmt() {
    int line = parser::at().line;

    parser::eat();
    parser::expect(TokenType::Any, "condition");

    astToken condition = parser::parse_expr();

    std::vector<std::shared_ptr<astToken>> body;
    if (parser::at().type == TokenType::OpenBrace) {
        parser::expect(TokenType::OpenBrace);

        while (parser::at().type != TokenType::EndOfFile &&
               parser::at().type != TokenType::CloseBrace) {
            body.push_back(std::make_shared<astToken>(parser::parseStmt()));
        }
        parser::expect(TokenType::CloseBrace);
    } else {
        parser::expect(TokenType::Any);
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }

    parser::skip_semi_colon();

    return astToken{.kind = tokenKind::WhileLoopStmt,
        .left = std::make_shared<astToken>(condition),
        .body = body,
        .line = line};
}

astToken parser::parse_for_loop_stmt() {
    int line = parser::at().line;

    parser::eat();

    /*
        for (index) in (1, 10) {

        }
    */
    parser::expect(TokenType::Any, "arguments");

    std::vector<std::shared_ptr<astToken>> argumentsAST =
        parser::parse_arguments();
    std::vector<std::string> arguments;

    for (std::shared_ptr<astToken> &arg : argumentsAST) {
        arguments.push_back((*arg).value);
    }

    parser::expect(TokenType::In);

    std::variant<std::shared_ptr<astToken>,
        std::vector<std::shared_ptr<astToken>>>
        expr;
    parser::expect(TokenType::Any, "iterator");

    if (parser::at().type == TokenType::OpenParen) {
        expr = parser::parse_arguments();
    } else {
        expr = std::make_shared<astToken>(parser::parse_expr());
    }

    std::vector<std::shared_ptr<astToken>> body;
    if (parser::at().type == TokenType::OpenBrace) {
        parser::expect(TokenType::OpenBrace);

        while (parser::at().type != TokenType::EndOfFile &&
               parser::at().type != TokenType::CloseBrace) {
            body.push_back(std::make_shared<astToken>(parser::parseStmt()));
        }
        parser::expect(TokenType::CloseBrace);
    } else {
        parser::expect(TokenType::Any);
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }

    parser::skip_semi_colon();

    return astToken{.kind = tokenKind::ForLoopStmt,
        .body = body,
        .params = arguments,
        .iterator = expr,
        .line = line};
}

astToken parser::parse_if_stmt(bool isELIFChain) {
    int line = parser::at().line;
    parser::eat();
    parser::expect(TokenType::Any, "condition");
    astToken condition = parser::parse_expr();

    std::vector<std::shared_ptr<astToken>> body;
    if (parser::at().type == TokenType::OpenBrace) {
        parser::expect(TokenType::OpenBrace);

        while (parser::at().type != TokenType::EndOfFile &&
               parser::at().type != TokenType::CloseBrace) {
            body.push_back(std::make_shared<astToken>(parser::parseStmt()));
        }
        parser::expect(TokenType::CloseBrace);
    } else {
        parser::expect(TokenType::Any);
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }

    std::vector<std::shared_ptr<astToken>> elifChain;
    std::vector<std::shared_ptr<astToken>> elsebody;

    if (isELIFChain == false) {
        if (parser::at().value == "elif") {
            while (parser::at().value == "elif") {
                elifChain.push_back(
                    std::make_shared<astToken>(parser::parse_if_stmt(true)));
            }
        }
        if (parser::at().value == "else") {
            parser::eat();
            if (parser::at().type == TokenType::OpenBrace) {
                parser::expect(TokenType::OpenBrace);

                while (parser::at().type != TokenType::EndOfFile &&
                       parser::at().type != TokenType::CloseBrace) {
                    elsebody.push_back(
                        std::make_shared<astToken>(parser::parseStmt()));
                }
                parser::expect(TokenType::CloseBrace);
                parser::skip_semi_colon();
            } else {
                parser::expect(TokenType::Any);
                elsebody.push_back(
                    std::make_shared<astToken>(parser::parseStmt()));
            }
        } else {
            parser::skip_semi_colon();
        }
    }

    return astToken{.kind = tokenKind::IfStmt,
        .left = std::make_shared<astToken>(condition),
        .elifChain = elifChain,
        .elseBody = elsebody,
        .body = body,
        .line = line};
}

astToken parser::parse_function_declaration() {
    int line = parser::at().line;
    parser::eat();
    std::string identifier =
        (parser::at().type == TokenType::Identifier) ? parser::eat().value : "";

    std::vector<std::shared_ptr<astToken>> args = parser::parse_arguments();
    std::vector<std::string> params;

    for (std::shared_ptr<astToken> value : args) {
        params.push_back(value.get()->value);
    }

    parser::expect(TokenType::OpenBrace);
    std::vector<std::shared_ptr<astToken>> body;

    while (parser::at().type != TokenType::EndOfFile &&
           parser::at().type != TokenType::CloseBrace) {
        body.push_back(std::make_shared<astToken>(parser::parseStmt()));
    }
    parser::expect(TokenType::CloseBrace);

    return astToken{.kind = tokenKind::FunctionDeclaration,
        .body = body,
        .name = identifier,
        .params = params,
        .line = line};
}

astToken parser::parse_var_declaration() {
    int line = parser::at().line;
    parser::eat();
    std::string identifier = parser::expect(TokenType::Identifier).value;

    if (parser::at().type == TokenType::Equals) {
        parser::eat();
        parser::expect(TokenType::Any, "value");
        std::shared_ptr<astToken> value =
            std::make_shared<astToken>(parser::parseStmt());

        parser::skip_semi_colon();

        return astToken{.kind = tokenKind::VariableDeclaration,
            .right = value,
            .name = identifier,
            .line = line};
    };

    parser::skip_semi_colon();

    return astToken{.kind = tokenKind::VariableDeclaration,
        .right = std::make_shared<astToken>(
            astToken{.kind = tokenKind::NumberLiteral, .value = "0"}),
        .name = identifier,
        .line = line};
}

astToken parser::parse_assignment_expr() {
    astToken left = parser::parse_or_keyword();

    if (parser::at().type == TokenType::Equals) {
        std::string op = parser::eat().value;
        astToken right;
        parser::expect(TokenType::Any, "value");

        if (op[0] == '+' || op[0] == '-' || op[0] == '*' || op[0] == '/' ||
            op[0] == '^' || op[0] == '%') {
            right = astToken{.kind = tokenKind::BinaryExpr,
                .right = std::make_shared<astToken>(parser::parse_or_keyword()),
                .left = std::make_shared<astToken>(left),
                .op = std::string(1, op[0]),
                .line = left.line};
        } else {
            right = parser::parse_or_keyword();
        }

        parser::skip_semi_colon();

        return astToken{.kind = tokenKind::AssignmentExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .line = left.line};
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
        parser::expect(TokenType::Any, "value");
        astToken value = parser::parse_assignment_expr();

        return astToken{.kind = tokenKind::UnaryExpr,
            .right = std::make_shared<astToken>(value),
            .op = op,
            .line = value.line};
    }

    return parser::parse_assignment_expr();
}

astToken parser::parse_comparasion_expr() {
    astToken left = parser::parse_object_expr();

    while (parser::at().value == ">" || parser::at().value == "<" ||
           parser::at().value == ">=" || parser::at().value == "<=" ||
           parser::at().value == "==" || parser::at().value == "!=") {
        std::string op = parser::eat().value;
        parser::expect(TokenType::Any, "value");
        astToken right = parser::parse_object_expr();

        left = astToken{.kind = tokenKind::ComparisonExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = op,
            .line = left.line};
    }

    return left;
}

astToken parser::parse_power_expr() {
    astToken left = parser::parse_member_call_expr();

    while (parser::at().value == "^" || parser::at().value == "%") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_member_call_expr();

        left = astToken{.kind = tokenKind::BinaryExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = op,
            .line = left.line};
    }

    return left;
}

astToken parser::parse_division_expr() {
    astToken left = parser::parse_power_expr();

    while (parser::at().value == "/") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_power_expr();

        left = astToken{.kind = tokenKind::BinaryExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = op,
            .line = left.line};
    }

    return left;
}

astToken parser::parse_multiplicative_expr() {
    astToken left = parser::parse_division_expr();

    while (parser::at().value == "*") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_division_expr();

        left = astToken{.kind = tokenKind::BinaryExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = op,
            .line = left.line};
    }

    return left;
}

astToken parser::parse_subtraction_expr() {
    astToken left = parser::parse_multiplicative_expr();

    while (parser::at().value == "-") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_multiplicative_expr();

        left = astToken{.kind = tokenKind::BinaryExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = op,
            .line = left.line};
    }

    return left;
}

astToken parser::parse_additive_expr() {
    astToken left = parser::parse_subtraction_expr();

    while (parser::at().value == "+") {
        std::string op = parser::eat().value;
        astToken right = parser::parse_subtraction_expr();

        left = astToken{.kind = tokenKind::BinaryExpr,
            .right = std::make_shared<astToken>(right),
            .left = std::make_shared<astToken>(left),
            .op = op,
            .line = left.line};
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
        local args = this.at().type == "ClosedPar" and {} or
this.parse_arguments_list()

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
    std::vector<std::shared_ptr<astToken>> args =
        parser::at().type == TokenType::CloseParen
            ? std::vector<std::shared_ptr<astToken>>{}
            : parser::parse_arguments_list();

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
    int line = parser::at().line;
    astToken call_expr{.kind = tokenKind::CallExpr,
        .args = parser::parse_arguments(),
        .caller = caller,
        .line = line};

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

astToken parser::parse_object_expr() {
    if (parser::at().type != TokenType::OpenBrace) {
        return parser::parse_additive_expr();
    }
    int line = parser::at().line;
    parser::eat();

    std::vector<map_property> properties;

    while (parser::at().type != TokenType::EndOfFile &&
           parser::at().type != TokenType::CloseBrace) {
        std::shared_ptr<astToken> key =
            std::make_shared<astToken>(parser::parseStmt());

        if (parser::at().type == TokenType::Comma) {
            parser::eat();
            map_property token{.key = std::make_shared<astToken>(astToken{
                                   .kind = tokenKind::NumberLiteral,
                                   .value = std::to_string(properties.size())}),
                .value = key};
            properties.push_back(token);
            continue;
        } else if (parser::at().type == TokenType::CloseBrace) {
            map_property token{.key = std::make_shared<astToken>(astToken{
                                   .kind = tokenKind::NumberLiteral,
                                   .value = std::to_string(properties.size())}),
                .value = key};
            properties.push_back(token);
            continue;
        };

        parser::expect(TokenType::Colon);

        std::shared_ptr<astToken> value =
            std::make_shared<astToken>(parser::parseStmt());
        map_property token{.key = key, .value = value};

        properties.push_back(token);

        if (parser::at().type != TokenType::CloseBrace) {
            parser::expect(TokenType::Comma);
        }
    }

    parser::expect(TokenType::CloseBrace);
    parser::skip_semi_colon();

    return astToken{.kind = tokenKind::ObjectLiteral,
        .properties = properties,
        .line = line};
}

astToken parser::parse_member_expr() {
    int line = parser::at().line;
    astToken object = parser::parse_primary_expr();

    while (parser::at().type == TokenType::Dot ||
           parser::at().type == TokenType::OpenBracket) {
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

        astToken newObject{.kind = tokenKind::MemberExpr,
            .object = std::make_shared<astToken>(object),
            .property = std::make_shared<astToken>(property),
            .computed = computed,
            .line = line};

        object = newObject;
    }

    return object;
}
std::string replaceNewlines(const std::string &input) {
    std::string s = input;
    std::string from = "\\n";
    std::string to = "\n";

    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.length(), to);
        pos += to.length();
    }

    return s;
}
astToken parser::parse_primary_expr() {
    TokenType type = parser::at().type;
    switch (type) {
    case TokenType::Identifier: {
        auto token = parser::eat();

        return astToken{.kind = tokenKind::Identifier,
            .value = token.value,
            .line = token.line};
    }
    case TokenType::Number: {
        auto token = parser::eat();

        return astToken{.kind = tokenKind::NumberLiteral,
            .value = token.value,
            .line = token.line};
    }
    case TokenType::String: {
        auto token = parser::eat();

        return astToken{.kind = tokenKind::StringLiteral,
            .value = replaceNewlines(token.value),
            .line = token.line};
    }
    case TokenType::Boolean: {
        auto token = parser::eat();
        return astToken{.kind = tokenKind::BooleanLiteral,
            .value = token.value,
            .line = token.line};
    }
    case TokenType::OpenParen: {
        parser::eat();
        astToken value = parser::parseStmt();
        parser::expect(TokenType::CloseParen);
        return value;
    };
    default: {
        auto token = parser::eat();
        return astToken{.kind = tokenKind::Identifier,
            .value = token.value,
            .line = token.line};
    };
    }
}