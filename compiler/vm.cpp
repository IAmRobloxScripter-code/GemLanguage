#include "vm.hpp"
#include "../gemSettings.hpp"
#include "ctypes.hpp"
#include <algorithm>
#include <cctype>
#include <cxxabi.h>
#include <ffi.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <optional>
#include <variant>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#include <cstring>

std::unordered_map<std::string, void *> libraryPointers;

callback makeNative(std::function<void(local_space<StringVector> *)> fn) {
    return callback{
        {"type", std::string("native-fn")}, {"call", function{std::move(fn)}}};
}

std::string shiftTKS(StringVector &tokens) {
    if (tokens.empty())
        throw "Out of tokens";
    std::string value = std::move(tokens.front());
    tokens.pop_front();
    return value;
}
valueVariant shiftStack(local_space<StringVector> &env) {
    return env.pop();
}

void eval_function(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    std::string name = shiftTKS(tokens);
    StringVector params;

    while (!tokens.empty() && tokens[0] != ":") {
        params.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    StringVector body;
    int ends = 1;
    while (!tokens.empty()) {
        if (tokens[0] == "RET") {
            ends -= 1;
        }

        if (ends == 0) {
            break;
        }

        if (tokens[0] == "function") {
            ends += 1;
        }

        body.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    callbackVariant envVariant = std::any(
        std::shared_ptr<local_space<StringVector>>(&env, [](auto *) {}));

    std::map<std::string, callbackVariant> token{{"body", body},
        {"name", name},
        {"declarationEnv", envVariant},
        {"params", params},
        {"type", std::string("fn")}};

    env.stack.push_back(token);
}

bool is_double(const std::string &s) {
    std::istringstream iss(s);
    double d;
    char c;
    return iss >> d && !(iss >> c);
}

void eval_push(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    std::string val = shiftTKS(tokens);

    if (!val.empty() && is_double(val)) {
        env.stack.push_back(std::stod(val));
    } else {
        env.stack.push_back(val);
    }
}

void eval_store_local(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    std::string identifier = shiftTKS(tokens);
    auto scope = env.resolve(identifier);

    if (scope == nullptr) {
        env.local_stack[identifier] = shiftStack(env);
        return;
    }

    scope->local_stack[identifier] = shiftStack(env);
}

void eval_declare_local(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    std::string identifier = shiftTKS(tokens);

    env.local_stack[identifier] = shiftStack(env);
}

void eval_operand(StringVector &tokens, local_space<StringVector> &env) {
    std::string operand = shiftTKS(tokens);

    if (operand == "ADD")
        env.add();
    else if (operand == "SUB")
        env.sub();
    else if (operand == "MUL")
        env.mul();
    else if (operand == "DIV")
        env.div();
    else if (operand == "POW")
        env.pow();
    else if (operand == "MOD")
        env.mod();
}

valueVariant runFFI(const FFI_Func &f, std::vector<valueVariant> args) {
    ffi_cif cif;
    ffi_type *arg_types[f.arg_types.size()];
    void *values[args.size()];

    size_t argTypeIndex = 0;
    for (auto &argtype : f.arg_types) {
        arg_types[argTypeIndex] = argtype;
        argTypeIndex++;
    }

    std::vector<std::vector<char>> arg_storage(args.size());

    for (size_t index = 0; index < args.size(); ++index) {
        ffi_type *t = f.arg_types[index];
        arg_storage[index].resize(t->size);

        convertToC(f, index, t, arg_storage, values, args);
    }

    if (ffi_prep_cif(
            &cif, FFI_DEFAULT_ABI, args.size(), f.return_type, arg_types) ==
        FFI_OK) {
        if (f.return_type == &ffi_type_double) {
            double rc;
            ffi_call(&cif, FFI_FN(f.func_ptr), &rc, values);
            // cleanStorage(arg_storage, values, args.size());
            return valueVariant(rc);
        } else if (f.return_type == &ffi_type_sint) {
            int rc;
            ffi_call(&cif, FFI_FN(f.func_ptr), &rc, values);
            // cleanStorage(arg_storage, values, args.size());
            return valueVariant(rc);
        } else if (f.return_type == &ffi_type_pointer) {
            if (f.return_type_name == "table") {
                cTable *rc_ptr = nullptr;
                ffi_call(&cif, FFI_FN(f.func_ptr), &rc_ptr, values);

                cTable rc = *rc_ptr;
                table cppCopy = CToCppTable(rc);

                freeCtable(rc);
                free(rc_ptr);
                return valueVariant(std::move(cppCopy));
            } else {
                char *rc;
                ffi_call(&cif, FFI_FN(f.func_ptr), &rc, values);
                // cleanStorage(arg_storage, values, args.size());

                return valueVariant("\"" + std::string(rc) + "\"");
            }
        } else {
            throw "Unexpected return type";
        }
    }

    throw "ffi_prep_cif fail";
}

void eval_call(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    // std::cout << env.stack.size();
    if (std::holds_alternative<FFI_Func>(env.stack.back())) {
        FFI_Func fn = std::get<FFI_Func>(shiftStack(env));
        std::vector<valueVariant> args;

        for (size_t i = 0; i < fn.arg_types.size(); ++i) {
            if (env.stack.size() <= 0) {
                break;
            }
            args.push_back(shiftStack(env));
        }

        // std::reverse(args.begin(), args.end());

        valueVariant result = runFFI(fn, args);

        env.stack.push_back(result);
        return;
    }

    callback fn = std::get<callback>(shiftStack(env));
    if (std::get<std::string>(fn.at("type")) == "native-fn") {
        // fuckass classes and their pointers
        // dude, this bs is so much harder than TS
        // or Lua
        std::function<void(local_space<StringVector> *)> call =
            std::get<std::function<void(local_space<StringVector> *)>>(
                fn.at("call"));
        call(&env);
    } else {
        auto declarationAny = std::get<std::any>(fn.at("declarationEnv"));
        auto declaration =
            std::any_cast<std::shared_ptr<local_space<StringVector>>>(
                declarationAny);

        local_space<StringVector> scope(declaration);

        auto body = std::get<StringVector>(fn.at("body"));
        auto params = std::get<StringVector>(fn.at("params"));

        for (auto &param : params) {
            if (declaration->stack.size() == 0) {
                scope.local_stack[param] = env.getVariable("null");
                continue;
            }
            scope.local_stack[param] = shiftStack(*declaration);
        }

        bool shouldReturn = false;

        while (!body.empty()) {
            if (body[0] == "RETURN") {
                shouldReturn = true;
                break;
            }

            std::optional<bool> doReturn = evalToken(body, scope);
            if (doReturn == true) {
                shouldReturn = true;
                break;
            }
        };

        if (shouldReturn) {
            if (scope.stack.size() == 0) {
                env.stack.push_back(env.getVariable("null"));
            } else {
                env.stack.push_back(shiftStack(scope));
            }
        } else {
            scope.stack.clear();
        }
    }
}

std::optional<tableNode *> getIndex(
    table &object, const std::variant<double, int, std::string> &key) {
    auto it = std::find_if(object.begin(),
        object.end(),
        [&key](const tableNode &s) { return s.key == key; });

    if (it != object.end())
        return &(*it);

    return std::nullopt;
}

tableNode *getIndexWithSafeMode(table &object,
    const std::variant<double, int, std::string> &key,
    local_space<StringVector> &env) {
    auto it = getIndex(object, key);

    if (!it) {
        static tableNode null =
            tableNode{.key = "null", .value = env.getVariable("null")};
        return &null;
    }
    return *it;
}

void eval_load_local(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    std::string identifier = shiftTKS(tokens);
    auto value = env.getVariable(identifier);
    env.stack.push_back(value);
}

void eval_table(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    table table;
    env.stack.push_back(table);
}

void eval_store_key(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    auto key = shiftStack(env);
    auto value = shiftStack(env);
    table &object = std::get<table>(env.stack.back());

    auto it =
        std::find_if(object.begin(), object.end(), [&](const tableNode &node) {
            return std::get<std::string>(node.key) == key;
        });

    if (it != object.end()) {
        it->value = value;
    } else {
        object.push_back(
            tableNode{.key = std::get<std::string>(key), .value = value});
    }
}

std::optional<std::variant<double, int, std::string>> convert(
    const valueVariant &v) {
    return std::visit(
        [](auto &&arg)
            -> std::optional<std::variant<double, int, std::string>> {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double> ||
                          std::is_same_v<T, std::string>) {
                return arg;
            } else {
                return std::nullopt;
            }
        },
        v);
}

void eval_load_key(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);

    std::string key = std::visit(
        [](auto &&val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + val + "\"";
            } else {
                return "\"" + std::to_string((int)val) + "\"";
            }
        },
        convert(shiftStack(env)).value());

    auto value = shiftStack(env);
    if (std::holds_alternative<std::string>(value)) {
        env.stack.push_back(value);

        value = env.getVariable("string");
        table &object = std::get<table>(value);
        env.stack.push_back(getIndexWithSafeMode(object, key, env)->value);
    } else if (std::holds_alternative<table>(value)) {
        table &object = std::get<table>(value);
        auto member = getIndex(object, key);

        if (!member) {
            env.stack.push_back(value);
            value = env.getVariable("table");
            env.stack.push_back(
                getIndexWithSafeMode(std::get<table>(value), key, env)->value);
        } else {
            env.stack.push_back(member.value()->value);
        }
    }
}

struct if_stmt {
    StringVector condition;
    StringVector body;
    std::unique_ptr<std::vector<std::shared_ptr<if_stmt>>> elifLabels;
    std::unique_ptr<StringVector> elseBody;
};

bool isTruthy(auto &value) {
    if ((std::holds_alternative<std::string>(value) &&
            std::get<std::string>(value) == "__NULL__") ||
        (std::holds_alternative<bool>(value) &&
            std::get<bool>(value) == false)) {
        return false;
    } else {
        return true;
    }
}

bool eval_if_stmt(StringVector &tokens, local_space<StringVector> &env) {
    // parsing the labels
    if_stmt ast;
    shiftTKS(tokens);

    while (tokens[0] != "THEN") {
        ast.condition.push_back(shiftTKS(tokens));
    }
    shiftTKS(tokens);

    int ends = 1;
    while (!tokens.empty()) {
        if (tokens[0] == "ENDIF") {
            ends -= 1;
        }

        if (tokens[0] == "IF" || tokens[0] == "ELSE" || tokens[0] == "ELIF") {
            ends += 1;
        }
        if (ends <= 0) {
            break;
        }
        ast.body.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    if (tokens[0] == "ELIF") {
        ast.elifLabels =
            std::make_unique<std::vector<std::shared_ptr<if_stmt>>>();
    }

    while (tokens[0] == "ELIF") {
        if_stmt astELIF;
        shiftTKS(tokens);

        while (tokens[0] != "THEN") {
            astELIF.condition.push_back(shiftTKS(tokens));
        }
        shiftTKS(tokens);

        int ends = 1;
        while (!tokens.empty()) {
            if (tokens[0] == "ENDIF") {
                ends -= 1;
            }

            if (tokens[0] == "IF" || tokens[0] == "ELSE" ||
                tokens[0] == "ELIF") {
                ends += 1;
            }

            if (ends <= 0) {
                break;
            }

            astELIF.body.push_back(shiftTKS(tokens));
        }
        shiftTKS(tokens);
        ast.elifLabels->push_back(
            std::make_shared<if_stmt>(std::move(astELIF)));
    }

    if (tokens[0] == "ELSE") {
        ast.elseBody = std::make_unique<StringVector>();
        shiftTKS(tokens);

        int ends = 1;
        while (!tokens.empty()) {
            if (tokens[0] == "ENDIF") {
                ends -= 1;
            }

            if (tokens[0] == "IF" || tokens[0] == "ELSE" ||
                tokens[0] == "ELIF") {
                ends += 1;
            }

            if (ends <= 0) {
                break;
            }

            ast.elseBody->push_back(shiftTKS(tokens));
        }
        shiftTKS(tokens);
    }

    // parsing labels over
    // evaluating now

    auto envPtr = std::shared_ptr<local_space<StringVector>>(
        &env, [](local_space<StringVector> *) {});

    local_space<StringVector> scope(envPtr);
    while (!ast.condition.empty()) {
        evalToken(ast.condition, scope);
    }

    auto result = shiftStack(scope);
    bool shouldReturn = false;

    if (isTruthy(result)) {
        while (!ast.body.empty()) {
            if (ast.body[0] == "RETURN") {
                shouldReturn = true;
                break;
            }
            if (ast.body[0] == "CONTINUE") {
                shouldReturn = true;
                shiftTKS(ast.body);
                scope.stack.push_back("__CONTINUE__");
                break;
            }
            if (ast.body[0] == "BREAK") {
                scope.stack.push_back("__BREAK__");

                shouldReturn = true;
                break;
            }
            std::optional<bool> doReturn = evalToken(ast.body, scope);
            if (doReturn == true) {
                shouldReturn = true;
                break;
            }
        }
    } else {
        bool foundResult = false;

        if (ast.elifLabels) {
            for (auto &elifNode : *ast.elifLabels) {
                while (!elifNode->condition.empty()) {
                    evalToken(elifNode->condition, scope);
                }

                auto elifResult = shiftStack(scope);
                if (isTruthy(elifResult)) {
                    foundResult = true;
                    while (!elifNode->body.empty()) {
                        if (elifNode->body[0] == "RETURN") {
                            shouldReturn = true;
                            break;
                        }
                        if (elifNode->body[0] == "CONTINUE") {
                            shouldReturn = true;
                            shiftTKS(elifNode->body);
                            scope.stack.push_back("__CONTINUE__");

                            break;
                        }
                        if (elifNode->body[0] == "BREAK") {
                            scope.stack.push_back("__BREAK__");

                            shouldReturn = true;
                            break;
                        }
                        std::optional<bool> doReturn =
                            evalToken(elifNode->body, scope);
                        if (doReturn == true) {
                            shouldReturn = true;
                            break;
                        }
                    }
                } else {
                    continue;
                }
            }
        }

        if (foundResult == false && !ast.elseBody == false) {
            while (!(*ast.elseBody).empty()) {
                if ((*ast.elseBody)[0] == "RETURN") {
                    shouldReturn = true;
                    break;
                }
                if ((*ast.elseBody)[0] == "CONTINUE") {
                    shiftTKS(*ast.elseBody);
                    scope.stack.push_back("__CONTINUE__");

                    shouldReturn = true;
                    break;
                }
                if ((*ast.elseBody)[0] == "BREAK") {
                    scope.stack.push_back("__BREAK__");

                    shouldReturn = true;
                    break;
                }
                std::optional<bool> doReturn = evalToken(*ast.elseBody, scope);
                if (doReturn == true) {
                    shouldReturn = true;
                    break;
                }
            }
        }
    }

    if (shouldReturn) {
        if (scope.stack.size() == 0) {
            env.stack.push_back(env.getVariable("null"));
        } else {
            env.stack.push_back(shiftStack(scope));
        }
        return true;
    }

    return false;
}

/*
    return std:switch(operator, {
        ["&&"] = function()
            if left.type == "undefined" or
   left.value == false then return left else
                return this.evaluate(node.right,
   env) end end,
        ["||"] = function()
            if left.type == "undefined" or
   left.value == false then return
   this.evaluate(node.right, env) else return left
            end
        end,
        ["default"] = left,
    })
*/

void eval_logicgate_expr(StringVector &tokens, local_space<StringVector> &env) {
    std::string op = shiftTKS(tokens);
    auto y = shiftStack(env);
    auto x = shiftStack(env);

    if (op == "AND") {
        if (isTruthy(x) == false) {
            env.stack.push_back(x);
        } else {
            env.stack.push_back(y);
        }
    } else {
        if (isTruthy(x) == false) {
            env.stack.push_back(y);
        } else {
            env.stack.push_back(x);
        }
    }
}

void eval_comparison_expr(
    StringVector &tokens, local_space<StringVector> &env) {
    std::string operand = shiftTKS(tokens);

    if (operand == "EQ")
        env.equals();
    else if (operand == "GTE")
        env.greaterThanEquals();
    else if (operand == "LTE")
        env.lessThanEquals();
    else if (operand == "LT")
        env.lessThan();
    else if (operand == "GT")
        env.greaterThan();
    else if (operand == "NOE")
        env.notEqual();
}

bool eval_loop_stmt(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    StringVector body;

    while (!tokens.empty() && tokens[0] != "ENDLOOP") {
        body.push_back(shiftTKS(tokens));
    }

    shiftTKS(tokens);

    // body eval
    auto envPtr = std::shared_ptr<local_space<StringVector>>(
        &env, [](local_space<StringVector> *) {});

    local_space<StringVector> scope(envPtr);
    bool shouldReturn = false;

    auto evalBody = [&body, &shouldReturn, &scope]() {
        StringVector loopBody = body;
        while (!loopBody.empty()) {
            if (loopBody[0] == "RETURN") {
                shouldReturn = true;
                break;
            }
            if (loopBody[0] == "CONTINUE") {
                shiftTKS(body);
                break;
            }
            if (loopBody[0] == "BREAK") {
                break;
            }
            std::optional<bool> doReturn = evalToken(loopBody, scope);

            if (doReturn == true && scope.stack.back() == "__CONTINUE__") {
                shiftStack(scope);
                break;
            }

            if (doReturn == true && scope.stack.back() == "__BREAK__") {
                break;
            }

            if (doReturn == true) {
                shouldReturn = true;
                break;
            }
        }
    };

    auto null = scope.getVariable("null");
    while (scope.stack.size() == 0) {
        evalBody();
    }

    if (shouldReturn) {
        if (scope.stack.size() == 0) {
            env.stack.push_back(env.getVariable("null"));
        } else {
            env.stack.push_back(shiftStack(scope));
        }
        return true;
    }

    return false;
}

void eval_nested_assignment(
    StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);
    std::string depth = shiftTKS(tokens);

    table object = std::get<table>(shiftStack(env));
    table *dummy = &object;

    for (int i = 0; i < std::stoi(depth) - 1; i++) {
        std::string key = std::visit(
            [](auto &&val) -> std::string {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, std::string>) {
                    return "\"" + val + "\"";
                } else {
                    return "\"" + std::to_string((int)val) + "\"";
                }
            },
            convert(shiftStack(env)).value());
        auto nodeOpt = getIndex(*dummy, key);
        if (!nodeOpt)
            break;

        tableNode *node = *nodeOpt;

        if (auto t = std::get_if<table>(&node->value)) {
            dummy = t;
        } else {
            break;
        }
    }

    std::string lastKey = std::visit(
        [](auto &&val) -> std::string {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::string>) {
                return "\"" + val + "\"";
            } else {
                return "\"" + std::to_string((int)val) + "\"";
            }
        },
        convert(shiftStack(env)).value());
    auto nodeOpt = getIndex(*dummy, lastKey);
    if (!nodeOpt) {
        dummy->push_back(tableNode{.key = lastKey, .value = shiftStack(env)});
        env.stack.push_back(object);
        return;
    }

    tableNode *lastNode = *nodeOpt;
    if (lastNode) {
        lastNode->value = shiftStack(env);
    }

    env.stack.push_back(object);
}

ffi_type *get_ffi_type(const std::string &typeName) {
    static std::unordered_map<std::string, ffi_type *> types = {
        {"double", &ffi_type_double},
        {"float", &ffi_type_float},
        {"int", &ffi_type_sint},
        {"uint", &ffi_type_uint32},
        {"string", &ffi_type_pointer},
        {"table", &ffi_type_pointer},
        {"void", &ffi_type_void},
        {"bool", &ffi_type_uint8}};

    auto it = types.find(typeName);
    if (it != types.end())
        return it->second;
    return nullptr;
}

void eval_define(StringVector &tokens, local_space<StringVector> &env) {
    shiftTKS(tokens);

    std::string caller = shiftTKS(tokens);
    std::string path = shiftTKS(tokens);

    std::vector<ffi_type *> inputTypes;

    double arguments = env.getDouble(shiftStack(env));
    std::string thetype = std::get<std::string>(shiftStack(env));
    ffi_type *returnType = get_ffi_type(thetype);

    for (int i = 0; i < arguments; ++i) {
        inputTypes.push_back(
            get_ffi_type(std::get<std::string>(shiftStack(env))));
    }

#if defined(_WIN32) || defined(_WIN64)
    // Windows

    void *handle = nullptr;

    if (libraryPointers.find(path) == libraryPointers.end()) {
        libraryPointers.insert({path, LoadLibrary(path.c_str())});
        handle = libraryPointers.at(path);
    } else {
        handle = libraryPointers.at(path);
    }

    if (!handle) {
        throw "Failed to load DLL";
    }

    void *fn = GetProcAddress(hDLL, ("gem_" + caller).c_str());

    env.local_stack[caller] = FFI_Func{.func_ptr = fn,
        .return_type = returnType,
        .arg_types = inputTypes,
        .return_type_name = thetype};

#else
    // Linux / macOS
    void *handle = nullptr;

    if (libraryPointers.find(path) == libraryPointers.end()) {
        libraryPointers.insert({path, dlopen(path.c_str(), RTLD_LAZY)});
        handle = libraryPointers.at(path);
    } else {
        handle = libraryPointers.at(path);
    }

    if (!handle) {
        throw dlerror();
    }

    void *fn = dlsym(handle, ("gem_" + caller).c_str());

    env.local_stack[caller] = FFI_Func{.func_ptr = fn,
        .return_type = returnType,
        .arg_types = inputTypes,
        .return_type_name = thetype};
#endif
}

std::optional<bool> evalToken(
    StringVector &tokens, local_space<StringVector> &env) {
    std::string current = tokens[0];

    if (current == "function") {
        eval_function(tokens, env);
    } else if (current == "PUSH") {
        eval_push(tokens, env);
    } else if (current == "STORE_LOCAL") {
        eval_store_local(tokens, env);
    } else if (current == "DECLARE_LOCAL") {
        eval_declare_local(tokens, env);
    } else if (current == "ADD" || current == "SUB" || current == "MUL" ||
               current == "DIV" || current == "POW" || current == "MOD") {
        eval_operand(tokens, env);
    } else if (current == "CALL") {
        eval_call(tokens, env);
    } else if (current == "LOAD_LOCAL") {
        eval_load_local(tokens, env);
    } else if (current == "NEW_TABLE") {
        eval_table(tokens, env);
    } else if (current == "STORE_KEY") {
        eval_store_key(tokens, env);
    } else if (current == "POP") {
        shiftTKS(tokens);
        shiftStack(env);
    } else if (current == "LOAD_KEY") {
        eval_load_key(tokens, env);
    } else if (current == "IF") {
        return eval_if_stmt(tokens, env);
    } else if (current == "EQ" || current == "GTE" || current == "LTE" ||
               current == "GT" || current == "LT" || current == "NOE") {
        eval_comparison_expr(tokens, env);
    } else if (current == "NOT") {
        auto value = shiftStack(env);
        bool result = !isTruthy(value);
        env.stack.push_back(result);
    } else if (current == "AND" || current == "OR") {
        eval_logicgate_expr(tokens, env);
    } else if (current == "LOOP") {
        return eval_loop_stmt(tokens, env);
    } else if (current == "STORE_NESTED_ASSIGNMENT") {
        eval_nested_assignment(tokens, env);
    } else if (current == "DELETE") {
        shiftTKS(tokens);
        std::string fakeMemoryPtr = shiftTKS(tokens);
        env.local_stack.erase(fakeMemoryPtr);
    } else if (current == "DEFINE") {
        eval_define(tokens, env);
    } else {
        std::cout << "THIS FAILED: " + shiftTKS(tokens) << std::endl;
    }

    return std::nullopt;
}

/*
template <typename T>
std::string type_name() {
    int status = 0;
    char* demangled = abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr,
&status); std::string name = (status == 0) ? demangled : typeid(T).name();
    std::free(demangled);
    return name;
}

void printVariant(const valueVariant& v) {
    std::visit([](auto&& arg) {
        using T = std::decay_t<decltype(arg)>;
        std::cout << "Type: " << type_name<T>() << "\n";
    }, v);
}
*/

std::string makeTemplateString(const std::string &s) {
    return "\"\"" + s + "\"\"";
}

void initNatives(localStackType &local_stack) {
    local_stack["string"] = table{
        tableNode{.key = makeTemplateString("length"),
            .value = makeNative([](local_space<StringVector> *env) {
                std::string str = std::get<std::string>(env->pop());
                env->stack.push_back(static_cast<double>(str.size()) - 2);
            })},
        tableNode{.key = makeTemplateString("stopath"),
            .value = makeNative([](local_space<StringVector> *env) {
                std::string str = std::get<std::string>(env->pop());
                std::string importPath = std::get<std::string>(env->pop());

                str.erase(0, 1);
                str.erase(str.size() - 1, 1);
                importPath.erase(0, 1);
                importPath.erase(importPath.size() - 1, 1);

                std::filesystem::path currentDir = std::filesystem::path(str);
                std::filesystem::path resolved = currentDir / importPath;
                env->stack.push_back("\"" + resolved.string() + "\"");
            })},
        tableNode{.key = makeTemplateString("iterator"),
            .value = makeNative([](local_space<StringVector> *env) {
                std::string object = std::get<std::string>(env->pop());
                auto index = std::make_shared<size_t>(0);

                object.erase(0, 1);
                object.erase(object.size() - 1, 1);

                env->stack.push_back(
                    makeNative([object, index](local_space<StringVector> *env) {
                        if (*index >= object.size()) {
                            auto null = env->getVariable("null");
                            env->stack.push_back(std::get<std::string>(null));
                            env->stack.push_back(null);
                            return;
                        }

                        env->stack.push_back(static_cast<double>(*index));
                        env->stack.push_back(
                            "\"" + std::string(1, object[*index]) + "\"");
                        (*index)++;
                    }));
            })}};

    local_stack["table"] = table{
        tableNode{.key = makeTemplateString("length"),
            .value = makeNative([](local_space<StringVector> *env) {
                table object = std::get<table>(env->pop());
                env->stack.push_back(static_cast<double>(object.size()));
            })},
        tableNode{.key = makeTemplateString("iterator"),
            .value = makeNative([](local_space<StringVector> *env) {
                table object = std::get<table>(env->pop());
                auto index = std::make_shared<size_t>(0);

                env->stack.push_back(
                    makeNative([object, index](local_space<StringVector> *env) {
                        if (*index >= object.size()) {
                            auto null = env->getVariable("null");
                            env->stack.push_back(std::get<std::string>(null));
                            env->stack.push_back(null);
                            return;
                        }
                        std::string key =
                            std::get<std::string>(object[*index].key);

                        key.erase(0, 1);
                        key.erase(key.size() - 1, 1);

                        env->stack.push_back(key);
                        env->stack.push_back(object[*index].value);
                        (*index)++;
                    }));
            })}};
    local_stack["typeof"] = makeNative([](local_space<StringVector> *env) {
        std::string type = "null";
        valueVariant value = env->pop();

        if (std::holds_alternative<std::string>(value)) {
            type = "string";
        } else if (std::holds_alternative<double>(value) ||
                   std::holds_alternative<int>(value)) {
            type = "number";
        } else if (std::holds_alternative<table>(value)) {
            type = "table";
        } else if (std::holds_alternative<callback>(value)) {
            type = "function";
        } else if (std::holds_alternative<FFI_Func>(value)) {
            type = "C/C++ function";
        }

        env->stack.push_back("\"" + type + "\"");
    });
    local_stack["console"] =
        table{tableNode{.key = makeTemplateString("out"),
                  .value = makeNative([](local_space<StringVector> *env) {
                      std::vector<valueVariant> printQueue;

                      while (!env->stack.empty()) {
                          printQueue.push_back(env->stack.back());
                          env->pop();
                      }

                      for (auto &value : printQueue) {
                          print::printValue(value, 0);
                          std::cout << " ";
                      }

                      std::cout << "\n";
                  })},
            tableNode{.key = makeTemplateString("err"),
                .value = makeNative([](local_space<StringVector> *env) {
                    std::string text = std::get<std::string>(env->pop());
                    text.erase(0, 1);
                    text.erase(text.size() - 1, 1);
                    std::cout << paint(text, color::Red) << std::endl;
                })},
            tableNode{.key = makeTemplateString("in"),
                .value = makeNative([](local_space<StringVector> *env) {
                    std::string prompt = std::get<std::string>(env->pop());
                    prompt.erase(0, 1);
                    prompt.erase(prompt.size() - 1, 1);
                    std::cout << prompt;
                    std::cout.flush();
                    std::string input;
                    std::getline(std::cin, input);
                    env->stack.push_back("\"" + input + "\"");
                })}};
}

void evaluate(std::string &source) {
    if (settings.verbose)
        std::cout << "GVM(Gem Virtual Machine) "
                     "has started"
                  << std::endl;
    local_space<StringVector> env;
    initNatives(env.local_stack);
    if (settings.verbose)
        std::cout << "Enviroment has been initialized" << std::endl;
    source += "\nLOAD_LOCAL main\nCALL";
    if (settings.verbose)
        std::cout << "Bytecode tokenization has begun" << std::endl;
    StringVector tokens = tokenize(source);

    if (settings.verbose)
        std::cout << "Bytecode tokenization has ended" << std::endl;
    if (settings.verbose)
        std::cout << "GVM(Gem Virtual Machine) "
                     "has started evaluating"
                  << std::endl;

    while (!tokens.empty()) {
        evalToken(tokens, env);
    }

    if (settings.verbose)
        std::cout << "\nGVM(Gem Virtual Machine) "
                     "has finished evaluating"
                  << std::endl;
    // std::cout << '\n';
}