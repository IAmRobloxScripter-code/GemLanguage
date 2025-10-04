#include "lexer.hpp"
#include "debugger.hpp"

#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <vector>

std::vector<std::string> splitString(const std::string &sourceCode) {
    std::vector<std::string> words;

    for (size_t i = 0; i < sourceCode.length(); i++) {
        words.push_back(std::string(1, sourceCode[i]));
    }

    return words;
}

std::string shift(std::deque<std::string> &src) {
    std::string value = src.front();
    src.pop_front();
    return value;
}

lexer_token token(
    std::string value, TokenType type, int line = 0, int column = 1) {
    lexer_token token;
    token.type = type;
    token.value = value;
    token.line = line;
    token.column = column;
    return token;
}

bool isNumber(std::string x) {
    std::string matches = "0123456789e._";
    return matches.find(x) != std::string::npos;
}

bool isAlpha(std::string x) {
    std::string matches =
        "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_";
    return matches.find(x) != std::string::npos;
}

std::unordered_map<std::string, TokenType> keywords = {{"var", TokenType::Var},
    {"if", TokenType::IfStmt},
    {"fn", TokenType::Function},
    {"else", TokenType::Keyword},
    {"elif", TokenType::Keyword},
    {"for", TokenType::ForLoop},
    {"while", TokenType::WhileLoop},
    {"continue", TokenType::Keyword},
    {"break", TokenType::Keyword},
    {"in", TokenType::In},
    {"return", TokenType::Return},
    {"reflect", TokenType::Reflect},
    {"shine", TokenType::Shine},
    {"extern", TokenType::Extern},
    {"true", TokenType::Boolean},
    {"false", TokenType::Boolean}};

bool isStringBody(std::string x) {
    return x == "'" || x == "`" || x == "\"";
}

bool isWhitespace(char x) {
    return std::isspace(static_cast<unsigned char>(x));
}

bool isSpecialString(const std::string &str) {
    for (char c : str) {
        if (!std::isalnum(static_cast<unsigned char>(c))) {
            return true;
        }
    }
    return false;
}

std::vector<lexer_token> tokenize(
    const std::string &sourceCode, const std::string &file_name) {
    std::vector<lexer_token> tokens;
    auto v = splitString(sourceCode + "\n");
    std::deque<std::string> src = std::deque(v.begin(), v.end());
    int line = 1;
    int column = 1;

    std::string invalid_characters;

    while (!src.empty()) {
        std::string c = src.front();
        column++;
        if (c == "\n" || c == "\r") {
            line++;
            column = 1;
        }
        if (isWhitespace(c[0])) {
            shift(src);
            continue;
        };

        if (c == "(") {
            tokens.push_back(
                token(shift(src), TokenType::OpenParen, line, column));
        }

        else if (c == ")") {
            tokens.push_back(
                token(shift(src), TokenType::CloseParen, line, column));
        }

        else if (c == "{") {
            tokens.push_back(
                token(shift(src), TokenType::OpenBrace, line, column));
        }

        else if (c == "}") {
            tokens.push_back(
                token(shift(src), TokenType::CloseBrace, line, column));
        }

        else if (c == "[") {
            tokens.push_back(
                token(shift(src), TokenType::OpenBracket, line, column));
        }

        else if (c == "]") {
            tokens.push_back(
                token(shift(src), TokenType::CloseBracket, line, column));
        } else if (c == "-" && src[1] == ">") {
            shift(src);
            shift(src);
            tokens.push_back(token("->", TokenType::Arrow, line, column));
        } else if (c == ">" || c == "<" || c == "!") {
            if (src[1] == "=") {
                std::string op = shift(src);
                shift(src);
                tokens.push_back(token(
                    op + "=", TokenType::ComparisonOperator, line, column));
                continue;
            }

            tokens.push_back(
                token(shift(src), TokenType::ComparisonOperator, line, column));
        } else if (c == "=") {
            if (src[1] == "=") {
                shift(src);
                shift(src);
                tokens.push_back(
                    token("==", TokenType::ComparisonOperator, line, column));
                continue;
            }
            tokens.push_back(
                token(shift(src), TokenType::Equals, line, column));
        }

        else if (c == "+" || c == "-" || c == "*" || c == "/" || c == "^" ||
                 c == "%" || c == "!") {
            if (src[1] == "=") {
                std::string op = shift(src);
                shift(src);
                tokens.push_back(
                    token(op + "=", TokenType::Equals, line, column));

                continue;
            }
            tokens.push_back(
                token(shift(src), TokenType::BinaryOperator, line, column));
        }

        else if (c == ".") {
            tokens.push_back(token(shift(src), TokenType::Dot, line, column));
        }

        else if (c == ":") {
            if (src[1] == ":") {
                shift(src);
                shift(src);
                tokens.push_back(
                    token("::", TokenType::DoubleColon, line, column));
                continue;
            };
            tokens.push_back(token(shift(src), TokenType::Colon, line, column));
        }

        else if (c == ";") {
            tokens.push_back(
                token(shift(src), TokenType::SemiColon, line, column));
        }

        else if (c == ",") {
            tokens.push_back(token(shift(src), TokenType::Comma, line, column));
        } else if (c + src[1] == "&&") {
            shift(src);
            shift(src);
            tokens.push_back(token("&&", TokenType::Keyword, line, column));
        } else if (c + src[1] == "||") {
            shift(src);
            shift(src);
            tokens.push_back(token("||", TokenType::Keyword, line, column));
        } else if (isStringBody(c)) {
            std::string body = shift(src);

            while (!src.empty() && src[0] != c) {
                std::string ch = shift(src);

                if (ch == "\\") {
                    if (src[0] == "n" || src[0] == "t" || src[0] == "r") {
                        body += "\\" + shift(src);
                    } else {
                        body += "\\" + shift(src);
                    }
                } else {
                    body += ch;
                }
            }

            body += shift(src);

            tokens.push_back(token(body, TokenType::String, line, column));
        }

        else if (isAlpha(c)) {
            std::string keyword;

            while (!src.empty() && isAlpha(src[0])) {
                keyword += shift(src);
            }

            if (keywords.find(keyword) != keywords.end()) {
                tokens.push_back(
                    token(keyword, keywords.at(keyword), line, column));
            } else {
                tokens.push_back(
                    token(keyword, TokenType::Identifier, line, column));
            }
        }

        else if (isNumber(c)) {
            std::string number;
            while (!src.empty() && isNumber(src[0])) {
                number += shift(src);
            }
            int len = number.size() - 1;

            if (number[len] == 'e' || number[len] == '.' ||
                number[len] == '_') {
                error(error_type::lexical_error,
                    add_pointers("^", number, len, len),
                    file_name,
                    line,
                    "Invalid number literal!");
                exit(1);
            }

            number.erase(
                std::remove(number.begin(), number.end(), '_'), number.end());
            int amnt_of_e = std::count(number.begin(), number.end(), 'e');
            int amnt_of_dots = std::count(number.begin(), number.end(), '.');

            if (amnt_of_e > 1 || amnt_of_dots > 1) {
                error(error_type::lexical_error,
                    add_pointers("~", number, 0, len),
                    file_name,
                    line,
                    "Invalid number literal!");
                exit(1);
            }

            tokens.push_back(token(number, TokenType::Number, line, column));

        } else if (c + src[1] == "##") {
            shift(src);
            shift(src);

            if (src[0] == "*") {
                std::string comment;
                shift(src);

                while (!src.empty() && (src[0] + src[1] + src[2]) != "*##") {
                    comment += shift(src);
                }

                shift(src);
                shift(src);
                shift(src);

                tokens.push_back(
                    token(comment, TokenType::Comment, line, column));
            } else {
                std::string comment;

                while (!src.empty() && src[0] != "\n" && src[0] != "\r") {
                    comment += shift(src);
                }

                tokens.push_back(
                    token(comment, TokenType::Comment, line, column));
            }
        } else {
            invalid_characters += shift(src);
            continue;
        }
    }
    tokens.push_back(token("eof", TokenType::EndOfFile, line, column));

    if (invalid_characters.size() > 0) {
        error(error_type::lexical_error,
            add_pointers(
                "^", invalid_characters, 0, invalid_characters.size() - 1),
            file_name,
            line,
            "Invalid character(s)!");
        exit(1);
    }
    return tokens;
}