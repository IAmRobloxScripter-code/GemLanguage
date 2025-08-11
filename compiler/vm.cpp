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
    StringVector params;

    while (!tokens.empty() && tokens[0] != ":")
    {
        params.push_back(shift(tokens));
    }

    shift(tokens);

    StringVector body;
    int ends = 1;
    while (!tokens.empty())
    {
        if (tokens[0] == "RET") {
            ends -= 1;
        }

        if (ends == 0) {
           break;
        }

        if (tokens[0] == "function") {
            ends += 1;
        }

        body.push_back(shift(tokens));
    }

    shift(tokens);

    std::map<std::string, std::variant<StringVector, std::reference_wrapper<local_space<StringVector>>>> token {
        {"body", body},
        {"declarationEnv", std::ref(env)},
        {"params", params}
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

    env.local_stack[identifier] = shiftStack(env);
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

void eval_call(StringVector &tokens, local_space<StringVector> &env) {
    shift(tokens);
    
    int ID = static_cast<int>(env.function_name_ids.at(shift(tokens)));
    auto fn = env.function_stack[ID];

    auto &declaration = std::get<std::reference_wrapper<local_space<StringVector>>>(fn.at("declarationEnv"));

    local_space<StringVector> scope(std::make_shared<local_space<StringVector>>(declaration));
    auto body = std::get<StringVector>(fn.at("body"));
    auto params = std::get<StringVector>(fn.at("params"));

    for (auto &param : params) {
        scope.local_stack[param] = shiftStack(declaration);
    }
    
    while (!body.empty()) {
       evalToken(body, scope);
    };

    if (scope.stack.size() > 0) {
        env.stack.push_back(shiftStack(scope));
    }

    std::cout << scope.stack << std::endl;
}

void eval_load_local(StringVector &tokens, local_space<StringVector> &env) {
    shift(tokens);
    std::string identifier = shift(tokens);
    auto value = env.local_stack.at(identifier);
    env.stack.push_back(value);
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
    } else if (current == "CALL") {
        eval_call(tokens, env);
    } else if (current == "LOAD_LOCAL") {
        eval_load_local(tokens, env);
    } else {
        std::cout << "THIS FAILED: " + shift(tokens) << std::endl;
    }
}

void evaluate(std::string &source)
{
    local_space<StringVector> env;
    source += "\nCALL main";
    StringVector tokens = tokenize(source);

    while (!tokens.empty()) {
       evalToken(tokens, env);
    }
    std::cout << env.stack << std::endl;
    std::cout << "finished" << std::endl;
}