#pragma once
#include "../interpreter.hpp"

inline bool compare_number(double x, double y, const std::string &op) {
    if (op == "==")
        return x == y;
    else if (op == "!=")
        return x != y;
    else if (op == ">=")
        return x >= y;
    else if (op == "<=")
        return x <= y;
    else if (op == ">")
        return x > y;
    else if (op == "<")
        return x < y;
    else
        return false;
}

inline bool compare_string(std::string x, std::string y, const std::string &op) {
    if (op == "==")
        return x == y;
    else
        return false;
}

inline bool compare_bool(bool x, bool y, const std::string &op) {
    if (op == "==")
        return x == y;
    else
        return false;
}

inline bool compare_table(gem_table* x, gem_table* y, const std::string &op) {
    if (op == "==")
        return x == y;
    else
        return false;
}

inline bool compare_function(function* x, function* y, const std::string &op) {
    if (op == "==")
        return x == y;
    else
        return false;
}
