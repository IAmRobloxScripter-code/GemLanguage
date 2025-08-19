#ifndef TYPES_HPP
#define TYPES_HPP

#pragma once

#include <vector>
#include <string>
#include <functional>
#include <variant>
#include <map>

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

#endif