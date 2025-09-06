#pragma once
#ifndef VM_HPP
#define VM_HPP

#include "native.hpp"
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <variant>
#include <vector>
/*
  PUSH X
  STORE_LOCAL X
  LOAD_LOCAL X
  ADD
  SUB
  MUL
  DIV
  POW
  MOD

  function ANY X Y Z :
  RET

  CALL N

  NEW_TABLE
  PUSH 50
  STORE_KEY x ## {x: 50}
  NEW_TABLE
  NEW_TABLE
  STORE_KEY foo ## {foo: {}}
  STORE_LOCAL TABLE


  LOAD_LOCAL TABLE
  LOAD_KEY X
  POP

  PUSH 0
  STORE_LOCAL I

  LOOP
    IF
      LOAD_LOCAL I
      PUSH 10
      GTE
    THEN
      BREAK
    ENDIF
    --whatever
  ENDLOOP

  LOOP
    IF
      LOAD_LOCAL TABLE
      LOCAL_KEY ITERATOR
      CALL
    THEN
      --doostuff
    ENDIF
    BREAK
  ENDLOOP
*/

inline StringVector tokenize(std::string &src)
{
    StringVector tokens;
    std::string current;
    bool inQuotes = false;

    for (size_t i = 0; i < src.size(); ++i)
    {
        char c = src[i];

        if (c == '"')
        {
            inQuotes = !inQuotes;
            current += c;
        }
        else if (c == ' ' && !inQuotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else if ((c == '\n' || c == '\r') && !inQuotes)
        {
            if (!current.empty())
            {
                tokens.push_back(current);
                current.clear();
            }
        }
        else
        {
            if (c == '\\' && inQuotes && i + 1 < src.size())
            {
                current += src[i + 1];
                ++i;
            }
            else
            {
                current += c;
            }
        }
    }

    if (!current.empty())
    {
        tokens.push_back(current);
    }

    return tokens;
}

void evaluate(std::string &source);

template <typename T> class local_space
{
  public:
    stackType stack;
    localStackType local_stack;
    std::shared_ptr<local_space<T>> parent_local_space;

    local_space() : parent_local_space(nullptr)
    {
        local_stack["false"] = false;
        local_stack["true"] = true;
        local_stack["null"] = "__NULL__";
    }
    local_space(std::shared_ptr<local_space<T>> parent)
        : parent_local_space(parent)
    {
    }

    local_space *resolve(std::string identifier)
    {
        if (local_stack.find(identifier) != local_stack.end())
        {
            return this;
        }
        else
        {
            if (!parent_local_space)
            {
                return nullptr;
            }
            else
            {
                return parent_local_space->resolve(identifier);
            }
        }
    }

    valueVariant getVariable(std::string identifier)
    {
        local_space *env = resolve(identifier);

        if (env == nullptr)
        {
            return resolve("null")->local_stack["null"];
        }
        return env->local_stack[identifier];
    }

    valueVariant pop()
    {
        if (stack.empty())
            throw std::runtime_error("Stack underflow");
        valueVariant value = std::move(stack.back());
        stack.pop_back();
        return value;
    }

    float getDouble(const valueVariant &v)
    {
        if (auto pval = std::get_if<double>(&v))
            return *pval;
        throw "Expected double value on stack";
    }

    void add()
    {
        auto y = pop();
        auto x = pop();

        if (std::holds_alternative<std::string>(x) &&
            std::holds_alternative<std::string>(y))
        {
            (std::get<std::string>(x)).erase(0, 1);
            (std::get<std::string>(x))
                .erase((std::get<std::string>(x)).size() - 1, 1);
            (std::get<std::string>(y)).erase(0, 1);
            (std::get<std::string>(y))
                .erase((std::get<std::string>(y)).size() - 1, 1);
            stack.push_back("\"" + std::get<std::string>(x) +
                            std::get<std::string>(y) + "\"");
            return;
        }

        double result = getDouble(x) + getDouble(y);
        stack.push_back(result);
    }

    void sub()
    {
        auto y = pop();
        auto x = pop();
        double result = getDouble(x) - getDouble(y);
        stack.push_back(result);
    }

    void mul()
    {
        auto y = pop();
        auto x = pop();
        double result = getDouble(x) * getDouble(y);
        stack.push_back(result);
    }

    void div()
    {
        auto y = pop();
        auto x = pop();
        double divisor = getDouble(y);
        if (divisor == 0.0f)
            throw "Division by zero";
        double result = getDouble(x) / divisor;
        stack.push_back(result);
    }

    void pow()
    {
        auto y = pop();
        auto x = pop();
        double result = std::pow(getDouble(x), getDouble(y));
        stack.push_back(result);
    }

    void mod()
    {
        auto y = pop();
        auto x = pop();
        int a = static_cast<int>(getDouble(x));
        int b = static_cast<int>(getDouble(y));
        if (b == 0)
            throw "Modulo by zero";
        int result = a % b;
        stack.push_back(static_cast<double>(result));
    }

    void equals()
    {
        auto y = pop();
        auto x = pop();

        stack.push_back(y == x);
    }

    void greaterThanEquals()
    {
        auto y = pop();
        auto x = pop();

        stack.push_back(x >= y);
    }

    void lessThanEquals()
    {
        auto y = pop();
        auto x = pop();

        stack.push_back(x <= y);
    }

    void greaterThan()
    {
        auto y = pop();
        auto x = pop();

        stack.push_back(x > y);
    }

    void lessThan()
    {
        auto y = pop();
        auto x = pop();

        stack.push_back(x < y);
    }

    void notEqual()
    {
        auto y = pop();
        auto x = pop();

        stack.push_back(x != y);
    }
};

std::optional<bool> evalToken(
    StringVector &tokens, local_space<StringVector> &env);
void eval_call(StringVector &tokens, local_space<StringVector> &env);

#endif