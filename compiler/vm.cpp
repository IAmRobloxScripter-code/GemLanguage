#include "vm.hpp"
#include <map>
#include <variant>
#include <algorithm>
#include <iostream>
#include <cctype>
#include <functional>
#include "prettyprint.hpp"

std::string shift(StringVector &tokens)
{
    std::string value = tokens[0];
    tokens.erase(tokens.begin());
    return value;
}

std::variant<callback, float, int, double, std::string, table> shiftStack(local_space<StringVector> &env)
{
    return env.pop();
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
        if (tokens[0] == "RET")
        {
            ends -= 1;
        }

        if (ends == 0)
        {
            break;
        }

        if (tokens[0] == "function")
        {
            ends += 1;
        }

        body.push_back(shift(tokens));
    }

    shift(tokens);

    callback token{
        {"body", body},
        {"name", name},
        {"declarationEnv", std::ref(env)},
        {"params", params},
        {"type", "fn"}};

    env.stack.push_back(token);
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

    if (operand == "ADD")
        env.add();
    else if (operand == "SUB")
        env.sub();
    else if (operand == "MUL")
        env.mul();
    else if (operand == "DIV")
        env.div();
    else if (operand == "POW")
        env.pow();
    else if (operand == "MOD")
        env.mod();
}

void eval_call(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    //std::cout << env.stack.size();
    callback fn = std::get<callback>(shiftStack(env));
    if (std::get<std::string>(fn.at("type")) == "native-fn")
    {   
        //fuckass classes and their pointers dude this bs is so much harder than TS or Lua
        std::function<void(local_space<StringVector>*)> call = std::get<std::function<void(local_space<StringVector>*)>>(fn.at("call"));
        call(&env);
    }
    else
    {
        auto &declaration = std::get<std::reference_wrapper<local_space<StringVector>>>(fn.at("declarationEnv"));

        local_space<StringVector> scope(std::make_shared<local_space<StringVector>>(declaration));
        auto body = std::get<StringVector>(fn.at("body"));
        auto params = std::get<StringVector>(fn.at("params"));

        for (auto &param : params)
        {
            scope.local_stack[param] = shiftStack(declaration);
        }

        while (!body.empty())
        {
            evalToken(body, scope);
        };

        if (scope.stack.size() > 0)
        {
            env.stack.push_back(shiftStack(scope));
        }
    }
}

tableNode getIndex(table &object, const std::variant<float, int, std::string> &key)
{
    auto it = std::find_if(object.begin(), object.end(),
                           [&key](const tableNode &s)
                           { return s.key == key; });

    if (it != object.end())
        return *it;

    throw std::runtime_error("Key not found");
}

void eval_load_local(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string identifier = shift(tokens);
    auto value = env.getVariable(identifier);
    env.stack.push_back(value);
}

void eval_table(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    table table;
    env.stack.push_back(table);
}

void eval_store_key(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::string key = shift(tokens);
    auto value = shiftStack(env);
    table &object = std::get<table>(env.stack.back());

    auto it = std::find_if(object.begin(), object.end(),
                           [&](const tableNode &node)
                           { return std::get<std::string>(node.key) == key; });

    if (it != object.end())
    {
        it->value = value;
    }
    else
    {
        object.push_back({.key = key, .value = value});
    }
}

void eval_load_key(StringVector &tokens, local_space<StringVector> &env)
{
    shift(tokens);
    std::variant<float, int, std::string> key = shift(tokens);
    table object = std::get<table>(shiftStack(env));
    /*
    for (auto &node : object) {
        std::cout << std::get<std::string>(node.key) << std::endl;
    };
    */
    env.stack.push_back(getIndex(object, key).value);
}

struct if_stmt {
    StringVector condition;
    StringVector body;
    std::unique_ptr<std::vector<std::shared_ptr<if_stmt>>> elifLabels;
    std::unique_ptr<StringVector> elseBody;
};

void eval_if_stmt(StringVector &tokens, local_space<StringVector> &env) {
    // parsing the labels
    if_stmt ast;
    shift(tokens);

    while (tokens[0] != "THEN") {
        ast.body.push_back(shift(tokens));
    }
    shift(tokens);
    
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
    }
    else if (current == "STORE_LOCAL")
    {
        eval_store_local(tokens, env);
    }
    else if (current == "ADD" || current == "SUB" || current == "MUL" || current == "DIV" || current == "POW" || current == "MOD")
    {
        eval_operand(tokens, env);
    }
    else if (current == "CALL")
    {
        eval_call(tokens, env);
    }
    else if (current == "LOAD_LOCAL")
    {
        eval_load_local(tokens, env);
    }
    else if (current == "NEW_TABLE")
    {
        eval_table(tokens, env);
    }
    else if (current == "STORE_KEY")
    {
        eval_store_key(tokens, env);
    }
    else if (current == "POP")
    {
        shift(tokens);
        shiftStack(env);
    }
    else if (current == "LOAD_KEY")
    {
        eval_load_key(tokens, env);
    } else if (current == "IF")
    {
        eval_if_stmt(tokens, env);
    }
    else
    {
        std::cout << "THIS FAILED: " + shift(tokens) << std::endl;
    }
}

void evaluate(std::string &source)
{
    local_space<StringVector> env;
    source += "\nLOAD_LOCAL main\nCALL";
    StringVector tokens = tokenize(source);

    while (!tokens.empty())
    {
        evalToken(tokens, env);
    }
    // std::cout << env.stack << std::endl;
    std::cout << "\nfinished" << std::endl;
}