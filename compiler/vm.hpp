#ifndef VM_HPP
#define VM_HPP

#include <vector>
#include <memory>
#include <cmath>
#include <sstream>
#include <map>
#include <variant>
#include <string>

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

*/

using StringVector = std::vector<std::string>;

template <typename T>
class local_space;

using Abomination = std::vector<std::map<std::string, std::variant<StringVector, std::reference_wrapper<local_space<StringVector>>>>>;
using localStackType = std::map<std::string, std::variant<float, std::string>>;
using fnNameIdsType = std::map<std::string, int>;
using stackType = std::vector<std::variant<float, std::string>>;

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

template <typename T>
class local_space
{
public:
    stackType stack;
    fnNameIdsType function_name_ids;
    localStackType local_stack;
    Abomination function_stack;
    std::shared_ptr<local_space<T>> parent_local_space;

    local_space() : parent_local_space(nullptr) {}
    local_space(std::shared_ptr<local_space<T>> parent) : parent_local_space(parent) {}

    std::variant<float, std::string> pop()
    {
        if (stack.empty())
            throw std::runtime_error("Stack underflow");
        auto value = stack.back();
        stack.pop_back();
        return value;
    }

    float getFloat(const std::variant<float, std::string> &v)
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