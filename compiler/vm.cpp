#include "vm.hpp"
#include <map>
#include <variant>
#include <algorithm>
#include <iostream>
#include <cctype>
#include "prettyprint.hpp"

std::string shift(StringVector &tokens)
{
    std::string value = tokens[0];
    tokens.erase(tokens.begin());
    return value;
}

std::variant<float, std::string> shiftStack(local_space<StringVector> &env)
{
    auto value = env.stack[0];
    env.stack.erase(env.stack.begin());
    return value;
}

void eval_function(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string name = shift(tokens);

    StringVector body;
    while (!tokens.empty() && tokens[0] != "RET")
    {
        body.push_back(shift(tokens));
    }

    shift(tokens);

    std::map<std::string, std::variant<StringVector, local_space<StringVector>>> token{
        {"body", body},
        {"declarationEnv", env}
    };

    env.function_name_ids[name] = env.function_stack.size();
    env.function_stack.push_back(token);
}

void eval_push(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string val = shift(tokens);

    if (!val.empty() && std::all_of(val.begin(), val.end(), ::isdigit))
    {
        env.stack.push_back(std::stof(val));
    }
    else
    {
        env.stack.push_back(val);
    }
}

void eval_store_local(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string identifier = shift(tokens);

    std::map<std::string, std::variant<float, std::string>> token {
        {identifier, shiftStack(env)}
    };

    env.local_stack.push_back(token);
}

void eval_operand(StringVector &tokens, local_space<StringVector> &env)
{
    std::string operand = shift(tokens);

    if (operand == "ADD") env.add();
    else if (operand == "SUB") env.sub();
    else if (operand == "MUL") env.mul();
    else if (operand == "DIV") env.div();
    else if (operand == "POW") env.pow();
    else if (operand == "MOD") env.mod();
}

void evalToken(StringVector &tokens, local_space<StringVector> &env)
{
    std::string current = tokens[0];

    if (current == "function") {
        eval_function(tokens, env);
    }
    else if (current == "PUSH") {
        eval_push(tokens, env);
    }
    else if (current == "STORE_LOCAL") {
        eval_store_local(tokens, env);
    }
    else if (current == "ADD" || current == "SUB" || current == "MUL" || current == "DIV" || current == "POW" || current == "MOD") {
        eval_operand(tokens, env);
    }
}

void evaluate(std::string &source)
{
    local_space<StringVector> env;
    StringVector tokens = tokenize(source);

    while (!tokens.empty()) {
       evalToken(tokens, env);
    }
    std::cout << env.local_stack << std::endl;
    std::cout << "finished" << std::endl;
}