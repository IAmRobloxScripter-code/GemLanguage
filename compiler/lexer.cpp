#include "lexer.hpp"

#include <iostream>
#include <sstream>
#include <cstring>
#include <vector>
#include <variant>
#include <map>
#include "prettyprint.hpp"
#include <deque>

std::vector<std::string> splitString(const std::string &sourceCode)
{
    std::vector<std::string> words;

    for (size_t i = 0; i < sourceCode.length(); i++)
    {
        words.push_back(std::string(1, sourceCode[i]));
    }

    return words;
}

std::string shift(std::deque<std::string> &src)
{
    std::string value = src.front();
    src.pop_front();
    return value;
}

lexer_token token(std::string value, TokenType type)
{
    lexer_token token;
    token.type = type;
    token.value = value;
    return token;
}

bool isNumber(std::string x)
{
    std::string matches = "0123456789e.";
    return matches.find(x) != std::string::npos;
}

bool isAlpha(std::string x)
{
    std::string matches = "qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM_";
    return matches.find(x) != std::string::npos;
}

std::map<std::string, TokenType> keywords = {
    {"var", TokenType::Var},
    {"if", TokenType::IfStmt},
    {"fn", TokenType::Function},
    {"else", TokenType::Keyword},
    {"elif", TokenType::Keyword},
    {"for", TokenType::ForLoop},
    {"while", TokenType::WhileLoop},
    {"in", TokenType::In}};

bool isStringBody(std::string x)
{
    return x == "'" || x == "`" || x == "\"";
}

bool isWhitespace(char x)
{
    return std::isspace(static_cast<unsigned char>(x));
}

std::vector<lexer_token> tokenize(const std::string &sourceCode)
{
    std::vector<lexer_token> tokens;
    auto v = splitString(sourceCode);
    std::deque<std::string> src = std::deque(v.begin(), v.end());

    while (!src.empty())
    {
        std::string c = src.front();

        if (isWhitespace(c[0]))
        {
            shift(src);
            continue;
        };

        if (c == "(")
        {
            tokens.push_back(token(shift(src), TokenType::OpenParen));
        }

        else if (c == ")")
        {
            tokens.push_back(token(shift(src), TokenType::CloseParen));
        }

        else if (c == "{")
        {
            tokens.push_back(token(shift(src), TokenType::OpenBrace));
        }

        else if (c == "}")
        {
            tokens.push_back(token(shift(src), TokenType::CloseBrace));
        }

        else if (c == "[")
        {
            tokens.push_back(token(shift(src), TokenType::OpenBracket));
        }

        else if (c == "]")
        {
            tokens.push_back(token(shift(src), TokenType::CloseBracket));
        }
        else if (c == ">" || c == "<" || c == "!")
        {
            if (src[1] == "=")
            {
                std::string op = shift(src);
                shift(src);
                tokens.push_back(token(op + "=", TokenType::ComparisonOperator));
                continue;
            }

            tokens.push_back(token(shift(src), TokenType::ComparisonOperator));
        }
        else if (c == "=")
        {
            if (src[1] == "=")
            {
                shift(src);
                shift(src);
                tokens.push_back(token("==", TokenType::ComparisonOperator));
                continue;
            }
            tokens.push_back(token(shift(src), TokenType::Equals));
        }

        else if (c == "+" || c == "-" || c == "*" || c == "/" || c == "^" || c == "%" || c == "!")
        {
            tokens.push_back(token(shift(src), TokenType::BinaryOperator));
        }

        else if (c == "=")
        {
            tokens.push_back(token(shift(src), TokenType::Equals));
        }

        else if (c == ".")
        {
            tokens.push_back(token(shift(src), TokenType::Dot));
        }

        else if (c == ":")
        {
            if (src[1] == ":")
            {
                shift(src);
                shift(src);
                tokens.push_back(token("::", TokenType::DoubleColon));
                continue;
            };
            tokens.push_back(token(shift(src), TokenType::Colon));
        }

        else if (c == ";")
        {
            tokens.push_back(token(shift(src), TokenType::SemiColon));
        }

        else if (c == ",")
        {
            tokens.push_back(token(shift(src), TokenType::Comma));
        }
        else if (c + src[1] == "&&")
        {
            shift(src);
            shift(src);
            tokens.push_back(token("&&", TokenType::Keyword));
        }
        else if (c + src[1] == "||")
        {
            shift(src);
            shift(src);
            tokens.push_back(token("||", TokenType::Keyword));
        }
        else if (isStringBody(c))
        {
            std::string body;

            shift(src);

            while (!src.empty() && src[0] != c)
            {
                body += shift(src);
            }

            shift(src);

            tokens.push_back(token(body, TokenType::String));
        }

        else if (isAlpha(c))
        {
            std::string keyword;

            while (!src.empty() && isAlpha(src[0]))
            {
                keyword += shift(src);
            }

            if (keywords.find(keyword) != keywords.end())
            {
                tokens.push_back(token(keyword, keywords.at(keyword)));
            }
            else
            {
                tokens.push_back(token(keyword, TokenType::Identifier));
            }
        }

        else if (isNumber(c))
        {
            std::string number;

            while (!src.empty() && isNumber(src[0]))
            {
                number += shift(src);
            }

            tokens.push_back(token(number, TokenType::Number));
        }
        else if (c + src[1] == "##")
        {
            shift(src);
            shift(src);

            if (src[0] == "*")
            {
                std::string comment;
                shift(src);

                while (!src.empty() && (src[0] + src[1] + src[2]) != "*##")
                {
                    comment += shift(src);
                }

                shift(src);
                shift(src);
                shift(src);

                tokens.push_back(token(comment, TokenType::Comment));
            }
            else
            {
                std::string comment;

                while (!src.empty() && src[0] != "\n" && src[0] != "\r")
                {
                    comment += shift(src);
                }

                tokens.push_back(token(comment, TokenType::Comment));
            }
        }
        else
        {
            throw "Invalid character " + c;
        }
    }

    tokens.push_back(token("eof", TokenType::EndOfFile));

    return tokens;
}