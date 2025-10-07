#pragma once
#include "../debugger.hpp"
#include "../interpreter.hpp"
#include <cmath>
#include <string>
#include <vector>

inline std::string format_number(double n) {
    std::stringstream stream;
    stream << n;
    return stream.str();
}

inline void metadata_cleanup(std::vector<gem_value *> &args, int argcount = 0) {
    if (argcount == 0) {
        args.erase(args.begin());
        return;
    }
    if (args.size() > argcount) {
        args.erase(args.begin());
    }
}

inline void expect_args(std::vector<gem_value *> &args,
    const std::vector<gem_type> &expect,
    const std::string &file_name,
    u_int64_t line) {

    if (args.size() > expect.size()) {
        error(error_type::runtime_error,
            "",
            file_name,
            line,
            "Too many arguments! Expected " + std::to_string(expect.size()) +
                ", got " + std::to_string(args.size()));
        exit(1);
    }

    for (size_t i = 0; i < expect.size(); i++) {
        if (i >= args.size()) {
            continue;
        }

        if (expect[i] == gem_type::gem_any) {
            continue;
        }

        if (args[i]->value_type != expect[i]) {
            error(error_type::runtime_error,
                "",
                file_name,
                line,
                "Invalid argument at position " + std::to_string(i) +
                    "! Expected " + gem_type_tostring(expect[i]) + ", got " +
                    gem_type_tostring(args[i]->value_type));
            exit(1);
        }
    }
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
    case gem_type::gem_function: {
        std::stringstream ss;
        ss << value->func;
        buffer += "<function " + ss.str() + ">";
        break;
    }
    case gem_type::gem_table: {
        std::stringstream ss;
        ss << value->table;
        buffer += "<table " + ss.str() + ">";
        break;
    }
    default:
        buffer += "null";
        break;
    }
}

inline gem_value *stdgem25_console_out(
    std::vector<gem_value *> args, scope *env, u_int64_t line) {
    std::string buffer;
    metadata_cleanup(args);
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

inline gem_value *define_function_pointer_value(gem_value *(*caller)(
    std::vector<gem_value *>, scope *env, u_int64_t line)) {
    gem_value *value = make_value(gem_type::gem_function, true);
    function *callback = new function;
    callback->function_type = gem_function_type::metadata_function;
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

// table

inline gem_value *stdgem25_table_push_back(
    std::vector<gem_value *> args, scope *env, u_int64_t line) {
    metadata_cleanup(args, 2);

    auto expected =
        std::vector<gem_type>{gem_type::gem_table, gem_type::gem_any};

    expect_args(args, expected, env->file_name, line);

    args[0]->table->push_back(args[1]);

    return env->get_variable("null");
}

inline gem_value *stdgem25_table_push_front(
    std::vector<gem_value *> args, scope *env, u_int64_t line) {
    metadata_cleanup(args, 2);

    auto expected =
        std::vector<gem_type>{gem_type::gem_table, gem_type::gem_any};

    expect_args(args, expected, env->file_name, line);

    args[0]->table->push_front(args[1]);

    return env->get_variable("null");
}

inline gem_value *stdgem25_table_pop_back(
    std::vector<gem_value *> args, scope *env, u_int64_t line) {
    metadata_cleanup(args, 1);

    auto expected = std::vector<gem_type>{gem_type::gem_table};

    expect_args(args, expected, env->file_name, line);

    return args[0]->table->pop_back();
}

inline gem_value *stdgem25_table_pop_front(
    std::vector<gem_value *> args, scope *env, u_int64_t line) {
    metadata_cleanup(args, 1);

    auto expected = std::vector<gem_type>{gem_type::gem_table};

    expect_args(args, expected, env->file_name, line);

    return args[0]->table->pop_front();
}

inline gem_value *define_table() {
    gem_value *table_value = make_value(gem_type::gem_table, true);
    gem_table *methods = new gem_table;

    methods->hash_make(define_string_value("push_back"),
        define_function_pointer_value(stdgem25_table_push_back));
    methods->hash_make(define_string_value("push_front"),
        define_function_pointer_value(stdgem25_table_push_front));

    methods->hash_make(define_string_value("pop_back"),
        define_function_pointer_value(stdgem25_table_pop_back));
    methods->hash_make(define_string_value("pop_front"),
        define_function_pointer_value(stdgem25_table_pop_front));

    table_value->table = methods;

    return table_value;
}

// init

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
    enviroment->make_variable("table", define_table());
};