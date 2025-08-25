#include "vm.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <variant>

#include "../gemSettings.hpp"
#include "prettyprint.hpp"

std::string shiftTKS(StringVector &tokens)
{
    if (tokens.empty())
        throw "Out of tokens";
    std::string value = std::move(tokens.front());
    tokens.pop_front();
    return value;
}
valueVariant shiftStack(local_space<StringVector> &env)
{
    return env.pop();
}

void eval_function(StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    std::string name = shiftTKS(tokens);
    StringVector params;

    while (!tokens.empty() && tokens[0] != ":")
    {
        params.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

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

        body.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    callback token{{"body", body},
        {"name", name},
        {"declarationEnv", std::ref(env)},
        {"params", params},
        {"type", "fn"}};

    env.stack.push_back(token);
}

bool is_double(const std::string &s)
{
    std::istringstream iss(s);
    double d;
    char c;
    return iss >> d && !(iss >> c);
}

void eval_push(StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    std::string val = shiftTKS(tokens);

    if (!val.empty() && is_double(val))
    {
        env.stack.push_back(std::stod(val));
    }
    else
    {
        env.stack.push_back(val);
    }
}

void eval_store_local(StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    std::string identifier = shiftTKS(tokens);

    env.local_stack[identifier] = shiftStack(env);
}

void eval_operand(StringVector &tokens, local_space<StringVector> &env)
{
    std::string operand = shiftTKS(tokens);

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
    shiftTKS(tokens);
    // std::cout << env.stack.size();
    callback fn = std::get<callback>(shiftStack(env));
    if (std::get<std::string>(fn.at("type")) == "native-fn")
    {
        // fuckass classes and their pointers
        // dude, this bs is so much harder than TS
        // or Lua
        std::function<void(local_space<StringVector> *)> call =
            std::get<std::function<void(local_space<StringVector> *)>>(
                fn.at("call"));
        call(&env);
    }
    else
    {
        auto &declaration =
            std::get<std::reference_wrapper<local_space<StringVector>>>(
                fn.at("declarationEnv"));

        local_space<StringVector> scope(
            std::make_shared<local_space<StringVector>>(declaration));
        auto body = std::get<StringVector>(fn.at("body"));
        auto params = std::get<StringVector>(fn.at("params"));

        for (auto &param : params)
        {
            scope.local_stack[param] = shiftStack(declaration);
        }

        bool shouldReturn = false;

        while (!body.empty())
        {
            if (body[0] == "RETURN")
            {
                shouldReturn = true;
                break;
            }

            std::optional<bool> doReturn = evalToken(body, scope);
            if (doReturn == true)
            {
                shouldReturn = true;
                break;
            }
        };

        if (scope.stack.size() > 0 && shouldReturn)
        {
            env.stack.push_back(shiftStack(scope));
        }
        else
        {
            scope.stack.clear();
        }
    }
}

std::optional<tableNode> getIndex(
    table &object, const std::variant<double, int, std::string> &key)
{
    auto it = std::find_if(object.begin(),
        object.end(),
        [&key](const tableNode &s) { return s.key == key; });

    if (it != object.end())
        return *it;

    return std::nullopt;
}

void eval_load_local(StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    std::string identifier = shiftTKS(tokens);
    auto value = env.getVariable(identifier);
    env.stack.push_back(value);
}

void eval_table(StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    table table;
    env.stack.push_back(table);
}

void eval_store_key(StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    std::string key = shiftTKS(tokens);
    auto value = shiftStack(env);
    table &object = std::get<table>(env.stack.back());

    auto it = std::find_if(object.begin(),
        object.end(),
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
    shiftTKS(tokens);
    std::variant<double, int, std::string> key = shiftTKS(tokens);
    auto value = shiftStack(env);
    if (std::holds_alternative<std::string>(value))
    {
        env.stack.push_back(value);
        value = env.getVariable("string");
        table &object = std::get<table>(value);
        env.stack.push_back(getIndex(object, key).value().value);
    }
    else if (std::holds_alternative<table>(value))
    {
        table &object = std::get<table>(value);
        auto member = getIndex(object, key);

        if (!member)
        {
            env.stack.push_back(value);
            value = env.getVariable("table");
            env.stack.push_back(getIndex(std::get<table>(value), key).value().value);
        }
        else
        {
            env.stack.push_back(member.value().value);
        }
    }
}

struct if_stmt
{
    StringVector condition;
    StringVector body;
    std::unique_ptr<std::vector<std::shared_ptr<if_stmt>>> elifLabels;
    std::unique_ptr<StringVector> elseBody;
};

bool isTruthy(auto &value)
{
    if ((std::holds_alternative<std::string>(value) &&
            std::get<std::string>(value) == "__NULL__") ||
        (std::holds_alternative<bool>(value) && std::get<bool>(value) == false))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool eval_if_stmt(StringVector &tokens, local_space<StringVector> &env)
{
    // parsing the labels
    if_stmt ast;
    shiftTKS(tokens);

    while (tokens[0] != "THEN")
    {
        ast.condition.push_back(shiftTKS(tokens));
    }
    shiftTKS(tokens);

    int ends = 1;
    while (!tokens.empty())
    {
        if (tokens[0] == "ENDIF")
        {
            ends -= 1;
        }

        if (tokens[0] == "IF" || tokens[0] == "ELSE" || tokens[0] == "ELIF")
        {
            ends += 1;
        }
        if (ends <= 0)
        {
            break;
        }
        ast.body.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    if (tokens[0] == "ELIF")
    {
        ast.elifLabels =
            std::make_unique<std::vector<std::shared_ptr<if_stmt>>>();
    }

    while (tokens[0] == "ELIF")
    {
        if_stmt astELIF;
        shiftTKS(tokens);

        while (tokens[0] != "THEN")
        {
            astELIF.condition.push_back(shiftTKS(tokens));
        }
        shiftTKS(tokens);

        int ends = 1;
        while (!tokens.empty())
        {
            if (tokens[0] == "ENDIF")
            {
                ends -= 1;
            }

            if (tokens[0] == "IF" || tokens[0] == "ELSE" || tokens[0] == "ELIF")
            {
                ends += 1;
            }

            if (ends <= 0)
            {
                break;
            }

            astELIF.body.push_back(shiftTKS(tokens));
        }
        shiftTKS(tokens);
        ast.elifLabels->push_back(
            std::make_shared<if_stmt>(std::move(astELIF)));
    }

    if (tokens[0] == "ELSE")
    {
        ast.elseBody = std::make_unique<StringVector>();
        shiftTKS(tokens);

        int ends = 1;
        while (!tokens.empty())
        {
            if (tokens[0] == "ENDIF")
            {
                ends -= 1;
            }

            if (tokens[0] == "IF" || tokens[0] == "ELSE" || tokens[0] == "ELIF")
            {
                ends += 1;
            }

            if (ends <= 0)
            {
                break;
            }

            ast.elseBody->push_back(shiftTKS(tokens));
        }
        shiftTKS(tokens);
    }

    // parsing labels over
    // evaluating now

    local_space<StringVector> scope(std::ref(env));

    while (!ast.condition.empty())
    {
        evalToken(ast.condition, scope);
    }

    auto result = shiftStack(scope);
    bool shouldReturn = false;

    if (isTruthy(result))
    {
        while (!ast.body.empty())
        {
            if (ast.body[0] == "RETURN")
            {
                shouldReturn = true;
                break;
            }
            std::optional<bool> doReturn = evalToken(ast.body, scope);
            if (doReturn == true)
            {
                shouldReturn = true;
                break;
            }
        }
    }
    else
    {
        bool foundResult = false;

        if (ast.elifLabels)
        {
            for (auto &elifNode : *ast.elifLabels)
            {
                while (!elifNode->condition.empty())
                {
                    evalToken(elifNode->condition, scope);
                }

                auto elifResult = shiftStack(scope);
                if (isTruthy(elifResult))
                {
                    foundResult = true;
                    while (!elifNode->body.empty())
                    {
                        if (elifNode->body[0] == "RETURN")
                        {
                            shouldReturn = true;
                            break;
                        }
                        std::optional<bool> doReturn =
                            evalToken(elifNode->body, scope);
                        if (doReturn == true)
                        {
                            shouldReturn = true;
                            break;
                        }
                    }
                }
                else
                {
                    continue;
                }
            }
        }

        if (foundResult == false && !ast.elseBody == false)
        {
            while (!(*ast.elseBody).empty())
            {
                if ((*ast.elseBody)[0] == "RETURN")
                {
                    shouldReturn = true;
                    break;
                }
                std::optional<bool> doReturn = evalToken(*ast.elseBody, scope);
                if (doReturn == true)
                {
                    shouldReturn = true;
                    break;
                }
            }
        }
    }

    if (scope.stack.size() > 0 && shouldReturn)
    {
        auto returnValue = shiftStack(scope);
        env.stack.push_back(returnValue);
        return true;
    };

    return false;
}

/*
    return std:switch(operator, {
        ["&&"] = function()
            if left.type == "undefined" or
   left.value == false then return left else
                return this.evaluate(node.right,
   env) end end,
        ["||"] = function()
            if left.type == "undefined" or
   left.value == false then return
   this.evaluate(node.right, env) else return left
            end
        end,
        ["default"] = left,
    })
*/

void eval_logicgate_expr(StringVector &tokens, local_space<StringVector> &env)
{
    std::string op = shiftTKS(tokens);
    auto y = shiftStack(env);
    auto x = shiftStack(env);

    if (op == "AND")
    {
        if (isTruthy(x) == false)
        {
            env.stack.push_back(x);
        }
        else
        {
            env.stack.push_back(y);
        }
    }
    else
    {
        if (isTruthy(x) == false)
        {
            env.stack.push_back(y);
        }
        else
        {
            env.stack.push_back(x);
        }
    }
}

void eval_comparison_expr(StringVector &tokens, local_space<StringVector> &env)
{
    std::string operand = shiftTKS(tokens);

    if (operand == "EQ")
        env.equals();
    else if (operand == "GTE")
        env.greaterThanEquals();
    else if (operand == "LTE")
        env.lessThanEquals();
    else if (operand == "LT")
        env.lessThan();
    else if (operand == "GT")
        env.greaterThan();
    else if (operand == "NOE")
        env.notEqual();
}

std::optional<bool> eval_loop_stmt(
    StringVector &tokens, local_space<StringVector> &env)
{
    shiftTKS(tokens);
    StringVector body;

    while (!tokens.empty() && tokens[0] != "ENDLOOP")
    {
        body.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    // body eval
    local_space<StringVector> scope(std::ref(env));
    bool shouldReturn = false;

    auto evalBody = [&body, &shouldReturn, &scope]()
    {
        StringVector loopBody = body;
        while (!loopBody.empty())
        {
            if (loopBody[0] == "RETURN")
            {
                shouldReturn = true;
                break;
            }
            std::optional<bool> doReturn = evalToken(loopBody, scope);
            if (doReturn == true)
            {
                shouldReturn = true;
                break;
            }
        }
    };

    auto null = scope.getVariable("null");
    while (scope.stack.size() == 0)
    {
        evalBody();
    }

    if (scope.stack.size() > 0 && shouldReturn)
    {
        auto returnValue = shiftStack(scope);
        env.stack.push_back(returnValue);
        return true;
    };

    return std::nullopt;
}

std::optional<bool> evalToken(
    StringVector &tokens, local_space<StringVector> &env)
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
    else if (current == "ADD" || current == "SUB" || current == "MUL" ||
             current == "DIV" || current == "POW" || current == "MOD")
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
        shiftTKS(tokens);
        shiftStack(env);
    }
    else if (current == "LOAD_KEY")
    {
        eval_load_key(tokens, env);
    }
    else if (current == "IF")
    {
        return eval_if_stmt(tokens, env);
    }
    else if (current == "EQ" || current == "GTE" || current == "LTE" ||
             current == "GT" || current == "LT" || current == "NOE")
    {
        eval_comparison_expr(tokens, env);
    }
    else if (current == "NOT")
    {
        auto value = shiftStack(env);
        bool result = !isTruthy(value);
        env.stack.push_back(result);
    }
    else if (current == "AND" || current == "OR")
    {
        eval_logicgate_expr(tokens, env);
    }
    else if (current == "LOOP")
    {
        return eval_loop_stmt(tokens, env);
    }
    else
    {
        std::cout << "THIS FAILED: " + shiftTKS(tokens) << std::endl;
    }

    return std::nullopt;
}

void initNatives(localStackType &local_stack)
{
    local_stack["string"] = table{
        tableNode{.key = "\"length\"",
            .value = callback{{"type", "native-fn"},
                {"call",
                    function{[](local_space<StringVector> *env)
                        {
                            std::string str = std::get<std::string>(env->pop());
                            env->stack.push_back(
                                static_cast<double>(str.size()) - 2);
                        }}}}},
    };

    local_stack["table"] = table{
        tableNode{.key = "\"length\"",
            .value = callback{{"type", "native-fn"},
                {"call",
                    function{[](local_space<StringVector> *env)
                        {
                            table object = std::get<table>(env->pop());
                            env->stack.push_back(
                                static_cast<double>(object.size()));
                        }}}}},
    };
}

void evaluate(std::string &source)
{
    if (settings.verbose)
        std::cout << "GVM(Gem Virtual Machine) "
                     "has started"
                  << std::endl;
    local_space<StringVector> env;
    initNatives(env.local_stack);
    if (settings.verbose)
        std::cout << "Enviroment has been initialized" << std::endl;
    source += "\nLOAD_LOCAL main\nCALL";
    if (settings.verbose)
        std::cout << "Bytecode tokenization has begun" << std::endl;
    StringVector tokens = tokenize(source);

    if (settings.verbose)
        std::cout << "Bytecode tokenization has ended" << std::endl;
    if (settings.verbose)
        std::cout << "GVM(Gem Virtual Machine) "
                     "has started evaluating"
                  << std::endl;

    while (!tokens.empty())
    {
        evalToken(tokens, env);
    }

    if (settings.verbose)
        std::cout << "\nGVM(Gem Virtual Machine) "
                     "has finished evaluating"
                  << std::endl;
    // std::cout << '\n';
}