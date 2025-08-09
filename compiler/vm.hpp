#ifndef VM_HPP
#define VM_HPP

#include <vector>
#include <memory>
#include <cmath>
#include <sstream>
using StringVector = std::vector<std::string>;
using Abomination = std::vector<std::map<std::string, std::variant<std::string, local_space<StringVector>>>>;
using localStackType = std::vector<std::map<std::string, std::variant<float, std::string>>>;
using fnNameIdsType = std::map<std::string, int>;
using stackType = std::vector<std::variant<float, std::string>>;

std::vector<std::string> tokenize(std::string &sentence) {
    std::vector<std::string> words;
    std::istringstream iss(sentence);
    std::string word;

    while (iss >> word) {
        words.push_back(word);
    };

    return words;
};
void eval_function(std::vector<std::string> &tokens, auto &env)
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

    local_space() : parent_local_space(nullptr) {};
    local_space(std::shared_ptr<local_space<T>> parent) : parent_local_space(parent) {};

    T pop() {
        T value = stack.back();
        stack.pop_back();
        return value;
    }

    void add() {
        T x = pop();
        T y = pop();

        T value = x + y;
        stack.push_back(value);
    };

    void sub() {
        T x = pop();
        T y = pop();

        T value = x - y;
        stack.push_back(value);
    };

    void mul() {
        T x = pop();
        T y = pop();

        T value = x * y;
        stack.push_back(value);
    };

    void div() {
        T x = pop();
        T y = pop();

        T value = x / y;
        stack.push_back(value);
    };

    void pow() {
        T x = pop();
        T y = pop();

        T value = std::pow(x, y);
        stack.push_back(value);
    };

    void mod() {
        T x = pop();
        T y = pop();

        T value = x % y;
        stack.push_back(value);
    };
};

#endif