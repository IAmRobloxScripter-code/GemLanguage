#pragma once

#include <any>
#include <deque>
#include <ffi.h>
#include <functional>
#include <map>
#include <string>
#include <variant>
#include <vector>

struct FFI_Func {
    void *func_ptr;
    ffi_type *return_type;
    std::vector<ffi_type *> arg_types;
};

using StringVector = std::deque<std::string>;

template <typename T> class local_space;
using function = std::function<void(local_space<StringVector> *)>;
using callbackVariant =
    std::variant<StringVector, std::string, function, std::any>;
using callback = std::map<std::string, callbackVariant>;

struct tableNode {
    std::variant<double, int, std::string> key;
    std::variant<callback,
        double,
        int,
        bool,
        std::string,
        std::vector<tableNode>,
        FFI_Func>
        value;
};

using table = std::vector<tableNode>;
using valueVariant =
    std::variant<callback, double, int, bool, std::string, table, FFI_Func>;
using localStackType = std::map<std::string, valueVariant>;
using stackType = std::vector<valueVariant>;

enum class color { Black, Red, Green, Yellow, Blue, Magenta, Cyan, White, Reset };

static std::unordered_map<color, std::string> colors{
    {color::Black,   "\033[30m"},
    {color::Red,     "\033[31m"},
    {color::Green,   "\033[32m"},
    {color::Yellow,  "\033[33m"},
    {color::Blue,    "\033[34m"},
    {color::Magenta, "\033[35m"},
    {color::Cyan,    "\033[36m"},
    {color::White,   "\033[37m"},
    {color::Reset,   "\033[0m"}
};

inline std::string paint(const std::string &s, color color) {
    return colors[color] + s + colors[color::Reset];
}

inline bool operator==(const valueVariant &a, const valueVariant &b) {
    return std::visit(
        [](auto &&lhs, auto &&rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;

            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                return static_cast<double>(lhs) == static_cast<double>(rhs);
            } else if constexpr (std::is_same_v<L, bool> &&
                                 std::is_same_v<R, bool>) {
                return lhs == rhs;
            } else if constexpr (std::is_same_v<L, std::string> &&
                                 std::is_same_v<R, std::string>) {
                return lhs == rhs;
            } else {
                return false;
            }
        },
        a,
        b);
}

inline bool operator>=(const valueVariant &a, const valueVariant &b) {
    return std::visit(
        [](auto &&lhs, auto &&rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;

            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                return static_cast<double>(lhs) >= static_cast<double>(rhs);
            } else {
                return false;
            }
        },
        a,
        b);
}
inline bool operator<=(const valueVariant &a, const valueVariant &b) {
    return std::visit(
        [](auto &&lhs, auto &&rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;

            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                return static_cast<double>(lhs) <= static_cast<double>(rhs);
            } else {
                return false;
            }
        },
        a,
        b);
}

inline bool operator<(const valueVariant &a, const valueVariant &b) {
    return std::visit(
        [](auto &&lhs, auto &&rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;

            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                return static_cast<double>(lhs) < static_cast<double>(rhs);
            } else {
                return false;
            }
        },
        a,
        b);
}

inline bool operator>(const valueVariant &a, const valueVariant &b) {
    return std::visit(
        [](auto &&lhs, auto &&rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;

            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                return static_cast<double>(lhs) > static_cast<double>(rhs);
            } else {
                return false;
            }
        },
        a,
        b);
}

inline bool operator!=(const valueVariant &a, const valueVariant &b) {
    return std::visit(
        [](auto &&lhs, auto &&rhs) -> bool {
            using L = std::decay_t<decltype(lhs)>;
            using R = std::decay_t<decltype(rhs)>;

            if constexpr (std::is_arithmetic_v<L> && std::is_arithmetic_v<R>) {
                return static_cast<double>(lhs) != static_cast<double>(rhs);
            } else if constexpr (std::is_same_v<L, bool> &&
                                 std::is_same_v<R, bool>) {
                return lhs != rhs;
            } else if constexpr (std::is_same_v<L, std::string> &&
                                 std::is_same_v<R, std::string>) {
                return lhs != rhs;
            } else {
                return false;
            }
        },
        a,
        b);
}