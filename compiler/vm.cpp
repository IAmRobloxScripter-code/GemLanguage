#include "vm.hpp"
#include <map>
#include <variant>
#include <algorithm>
#include "prettyprint.hpp"
#include <iostream>

std::string shift(StringVector &tokens)
{
    std::string value = tokens[0];
    tokens.erase(tokens.begin());
    return value;
}

std::variant<std::string, float> shiftStack(local_space<StringVector> &env)
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

    std::map<std::string, std::variant<
                              StringVector,
                              local_space<StringVector>>>
        token = {
            {"body", body},
            {"declarationEnv", env}};

    env.function_name_ids[name] = env.function_stack.size();
    env.function_stack.push_back(token);
}

void eval_push(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string number = tokens[0];

    if (std::all_of(number.begin(), number.end(), ::isdigit) == true)
    {
        number = std::stoi(number);
    }

    env.stack.push_back(number);
}

void eval_store_local(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string identifier = tokens[0];

    std::map<std::string, std::variant<float, std::variant<std::string, float>>> token{
        {identifier, shiftStack(env)}};

    env.local_stack.push_back(token);
}

void eval_operand(StringVector &tokens, local_space<StringVector> &env)
{
    std::string operand = shift(tokens);

    if (operand == "ADD")
    {
        env.add();
    }
    else if (operand == "SUB")
    {
        env.sub();
    }
    else if (operand == "SUB")
    {
        env.sub();
    }
    else if (operand == "MUL")
    {
        env.mul();
    }
    else if (operand == "DIV")
    {
        env.div();
    }
    else if (operand == "POW")
    {
        env.pow();
    }
    else if (operand == "MOD")
    {
        env.mod();
    }
}

void evalToken(StringVector &tokens, local_space<StringVector> &env)
{
    std::string current = tokens[0];

    if (current == "function")
    {
        eval_function(tokens, env);
    }
    else if (current == "PUSH")
    {
        eval_push(tokens, env);
    } else if (current == "STORE_LOCAL")
    {
        eval_store_local(tokens, env);
    }
    else if (current == "ADD"
        || current == "SUB" 
        || current == "MUL" 
        || current == "DIV" 
        || current == "POW" 
        || current == "MOD")
    {
        eval_operand(tokens, env);
    }
}

void evaluate(std::string &source)
{
    local_space<StringVector> env;
    StringVector tokens = tokenize(source);

    std::cout << env.local_stack;
}