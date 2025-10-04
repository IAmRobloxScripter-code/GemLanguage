#pragma once
#include "../interpreter.hpp"
#include <cmath>
#include <vector>

#include <string>

inline std::string format_number(double n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}

// console

inline void stdgem25_print_value(gem_value *value, std::string &buffer) {
    switch (value->value_type) {
    case gem_type::gem_number:
        buffer += format_number(value->number);
        break;
    case gem_type::gem_string:
        buffer += value->string;
        break;
    case gem_type::gem_bool:
        buffer += value->boolean == true ? "true" : "false";
        break;
    default:
        break;
    }
}

inline gem_value *stdgem25_console_out(
    std::vector<gem_value *> args, scope *env) {
    std::string buffer;
    for (auto &value : args) {
        stdgem25_print_value(value, buffer);
    }

    std::cout << buffer << std::endl;
    return env->get_variable("null");
}

inline gem_value *define_string_value(const std::string &src) {
    gem_value *value = make_value(gem_type::gem_string, true);
    value->string = src;
    return value;
}

inline gem_value *define_function_pointer_value(
    gem_value *(*caller)(std::vector<gem_value *>, scope *env)) {
    gem_value *value = make_value(gem_type::gem_function, true);
    function *callback = new function;
    callback->function_type = gem_function_type::native_function;
    callback->caller = caller;

    value->func = callback;

    return value;
}

inline gem_value *define_console() {
    gem_value *console_value = make_value(gem_type::gem_table, true);
    gem_table *methods = new gem_table;

    methods->hash_make(define_string_value("out"),
        define_function_pointer_value(stdgem25_console_out));

    console_value->table = methods;

    return console_value;
}

inline void define_globals(scope *enviroment) {
    gem_value *null_value = make_value(gem_type::gem_null, true);
    gem_value *false_value = make_value(gem_type::gem_bool, true);
    gem_value *true_value = make_value(gem_type::gem_bool, true);
    false_value->boolean = false;
    true_value->boolean = true;

    enviroment->make_variable("null", null_value);
    enviroment->make_variable("false", false_value);
    enviroment->make_variable("true", true_value);

    enviroment->make_variable("console", define_console());
};