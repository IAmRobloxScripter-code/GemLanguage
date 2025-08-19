#ifndef NATIVE_HPP
#define NATIVE_HPP

#pragma once

#include <ostream>
#include "types.hpp"
#include <iostream>

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
            std::cout << ": ";
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
                std::cout << "{";
                std::cout << "\n";
                printTable(v, depth + 1);
                std::cout << std::string(depth * 2, ' ') + "}";
            }
            else if constexpr (std::is_same_v<T, callback>) {
                std::cout << "<function>";
            }
            else {
                std::cout << v;
            } }, value);
    }
}

#endif