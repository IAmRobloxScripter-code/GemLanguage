#pragma once
#ifndef NATIVE_HPP
#define NATIVE_HPP

#include "types.hpp"
#include <iostream>
#include <ostream>

callback makeNative(std::function<void(local_space<StringVector> *)> fn);

// im tired boss
namespace print
{
inline void printValue(const valueVariant &value, int depth = 0);
inline void printTable(const table &tbl, int depth = 0);

inline void printKey(const std::variant<double, int, std::string> &key)
{
    std::string s = std::get<std::string>(key);
    s.erase(0, 1);
    s.erase(s.size() - 1, 1);
    std::cout << s;
}

inline void printTable(const table &tbl, int depth)
{
    for (const auto &node : tbl)
    {
        std::cout << std::string(depth * 2, ' ');
        printKey(node.key);
        std::cout << ": ";
        printValue(node.value, depth);
        std::cout << "\n";
    }
}

inline void printValue(const valueVariant &value, int depth)
{
    // std::visit pmo
    std::visit(
        [depth](auto &&v)
        {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, table>)
            {
                std::cout << "{";
                std::cout << "\n";
                printTable(v, depth + 1);
                std::cout << std::string(depth * 2, ' ') + "}";
            }
            else if constexpr (std::is_same_v<T, callback>)
            {
                std::cout << "<function>";
            }
            else
            {
                if constexpr (std::is_same_v<T, std::string>)
                {
                    std::string s = static_cast<std::string>(v);
                    s.erase(0, 1);
                    s.erase(s.size() - 1, 1);
                    std::cout << s;
                }
                else
                {
                    std::cout << v;
                }
            }
        },
        value);
}
} // namespace print

#endif