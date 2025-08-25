#ifndef TYPES_HPP
#define TYPES_HPP

#pragma once

#include <vector>
#include <string>
#include <functional>
#include <variant>
#include <map>
#include <deque>
using StringVector = std::deque<std::string>;

template <typename T>
class local_space;
using function = std::function<void(local_space<StringVector>*)>;
using callback = std::map<std::string, std::variant<StringVector, std::string, function, std::reference_wrapper<local_space<StringVector>>>>;

struct tableNode
{
    std::variant<double, int, std::string> key;
    std::variant<callback, double, int, bool, std::string, std::vector<tableNode>> value;
};

using table = std::vector<tableNode>;
using valueVariant = std::variant<callback, double, int, bool, std::string, table>;
using localStackType = std::map<std::string, valueVariant>;
using stackType = std::vector<valueVariant>;

inline bool operator==(const valueVariant& a, const valueVariant& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
            return static_cast<double>(lhs) == static_cast<double>(rhs);
        } else if constexpr (std::is_same_v<L, bool> && std::is_same_v<R, bool>) {
            return lhs == rhs;
        }
        else if constexpr (std::is_same_v<L, std::string> && std::is_same_v<R, std::string>) {
            return lhs == rhs;
        }
        else {
            return false;
        }
    }, a, b);
}

inline bool operator>=(const valueVariant& a, const valueVariant& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
            return static_cast<double>(lhs) >= static_cast<double>(rhs);
        }
        else {
            return false;
        }
    }, a, b);
}
inline bool operator<=(const valueVariant& a, const valueVariant& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
            return static_cast<double>(lhs) <= static_cast<double>(rhs);
        }
        else {
            return false;
        }
    }, a, b);
}

inline bool operator<(const valueVariant& a, const valueVariant& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
            return static_cast<double>(lhs) < static_cast<double>(rhs);
        }
        else {
            return false;
        }
    }, a, b);
}

inline bool operator>(const valueVariant& a, const valueVariant& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
            return static_cast<double>(lhs) > static_cast<double>(rhs);
        }
        else {
            return false;
        }
    }, a, b);
}

inline bool operator!=(const valueVariant& a, const valueVariant& b) {
    return std::visit([](auto&& lhs, auto&& rhs) -> bool {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
            return static_cast<double>(lhs) != static_cast<double>(rhs);
        } else if constexpr (std::is_same_v<L, bool> && std::is_same_v<R, bool>) {
            return lhs != rhs;
        }
        else if constexpr (std::is_same_v<L, std::string> && std::is_same_v<R, std::string>) {
            return lhs != rhs;
        }
        else {
            return false;
        }
    }, a, b);
}

#endif