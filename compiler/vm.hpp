#ifndef VM_HPP
#define VM_HPP

#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <sstream>
#include <map>
#include <variant>
#include <string>
#include <functional>

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

  LOAD_KEY_POP X
*/

using StringVector = std::vector<std::string>;

template <typename T>
class local_space;
using function = std::function<void(local_space<StringVector>*)>;
using callback = std::map<std::string, std::variant<StringVector, std::string, function, std::reference_wrapper<local_space<StringVector>>>>;

struct tableNode
{
    std::variant<float, int, std::string> key;
    std::variant<callback, float, int, double, std::string, std::vector<tableNode>> value;
};
using table = std::vector<tableNode>;
using valueVariant = std::variant<callback, float, int, double, std::string, table>;
using localStackType = std::map<std::string, valueVariant>;
using stackType = std::vector<valueVariant>;

inline StringVector tokenize(std::string &src)
{
    StringVector words;
    std::istringstream str(src);
    std::string word;
    while (str >> word)
    {
        words.push_back(word);
    }
    return words;
}

void evaluate(std::string &source);
// im tired boss
namespace print
{
    inline void printValue(const valueVariant &value, int depth = 0);
    inline void printTable(const table &tbl, int depth = 0);

    inline void printKey(const std::variant<float, int, std::string> &key)
    {
        std::visit([](auto &&k)
                   { std::cout << k; }, key);
    }

    inline void printTable(const table &tbl, int depth)
    {
        for (const auto &node : tbl)
        {
            std::cout << std::string(depth * 2, ' ');
            printKey(node.key);
            std::cout << " : ";
            printValue(node.value, depth);
            std::cout << "\n";
        }
    }

    inline void printValue(const valueVariant &value, int depth)
    {
        // std::visit pmo
        std::visit([depth](auto &&v)
                   {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, table>) {
                std::cout << "\n";
                printTable(v, depth + 1);
            }
            else if constexpr (std::is_same_v<T, callback>) {
                std::cout << "<function>";
            }
            else {
                std::cout << v;
            } }, value);
    }
}

template <typename T>
class local_space
{
public:
    stackType stack;
    localStackType local_stack;
    std::shared_ptr<local_space<T>> parent_local_space;

    local_space() : parent_local_space(nullptr)
    {
        local_stack["print"] = callback{
            {"type", "native-fn"},
            {"call", function{[this](local_space<StringVector>* env)
             {
                 while (!env->stack.empty())
                 {
                     print::printValue(env->stack.back(), 0);
                     env->stack.pop_back();
                 }
             }
            }
         }
        };
    }
    local_space(std::shared_ptr<local_space<T>> parent) : parent_local_space(parent) {}

    local_space &resolve(std::string identifier)
    {
        if (local_stack.find(identifier) != local_stack.end())
        {
            return *this;
        }
        else
        {
            if (!parent_local_space)
            {
                throw "Variable not found";
            }
            else
            {
                return parent_local_space->resolve(identifier);
            }
        }
    }

    valueVariant getVariable(std::string identifier)
    {
        local_space &env = resolve(identifier);
        return env.local_stack[identifier];
    }

    valueVariant pop()
    {
        if (stack.empty())
            throw std::runtime_error("Stack underflow");
        auto value = stack.back();
        stack.pop_back();
        return value;
    }

    float getFloat(const valueVariant &v)
    {
        if (auto pval = std::get_if<float>(&v))
            return *pval;
        throw std::runtime_error("Expected float value on stack");
    }

    void add()
    {
        auto x = pop();
        auto y = pop();
        float result = getFloat(x) + getFloat(y);
        stack.push_back(result);
    }

    void sub()
    {
        auto x = pop();
        auto y = pop();
        float result = getFloat(x) - getFloat(y);
        stack.push_back(result);
    }

    void mul()
    {
        auto x = pop();
        auto y = pop();
        float result = getFloat(x) * getFloat(y);
        stack.push_back(result);
    }

    void div()
    {
        auto x = pop();
        auto y = pop();
        float divisor = getFloat(y);
        if (divisor == 0.0f)
            throw std::runtime_error("Division by zero");
        float result = getFloat(x) / divisor;
        stack.push_back(result);
    }

    void pow()
    {
        auto x = pop();
        auto y = pop();
        float result = std::pow(getFloat(x), getFloat(y));
        stack.push_back(result);
    }

    void mod()
    {
        auto x = pop();
        auto y = pop();
        int a = static_cast<int>(getFloat(x));
        int b = static_cast<int>(getFloat(y));
        if (b == 0)
            throw std::runtime_error("Modulo by zero");
        int result = a % b;
        stack.push_back(static_cast<float>(result));
    }
};

void evalToken(StringVector &tokens, local_space<StringVector> &env);
void eval_call(StringVector &tokens, local_space<StringVector> &env);

#endif