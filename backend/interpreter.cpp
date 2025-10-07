#define an_ptr std::unique_ptr<abstract_node>

#include "./magic_enum/magic_enum.hpp"
#include "./std/compare.hpp"
#include "debugger.hpp"
#include <cmath>
#include <iostream>
#include <memory>

void scope_erase(scope *env, scope *scope_env) {
    auto it = std::find(env->closures.begin(), env->closures.end(), scope_env);
    if (it != env->closures.end()) {
        env->closures.erase(it);
    }
}

void trace_back_me_rec(std::string &trace, astToken &node) {
    if (node.object->kind == tokenKind::MemberExpr) {
        trace_back_me_rec(trace, *node.object);
    } else {
        trace += node.object->value;
    }

    if (node.computed) {
        trace += "[" + node.property->value + "]";
    } else {
        trace += "." + node.property->value;
    }
}

std::string trace_back_member_expression(astToken &node) {
    std::string trace;
    trace_back_me_rec(trace, node);
    return trace;
}

bool is_truthy(gem_value *value) {
    if (value->value_type == gem_type::gem_null) {
        return false;
    } else if (value->value_type == gem_type::gem_bool) {
        return value->boolean;
    } else {
        return true;
    }
}

an_ptr interpret_program(astToken &node, scope *env) {
    an_ptr result = nullptr;

    for (auto &token : node.body) {
        an_ptr return_result = interpret(*token, env);

        if (dynamic_cast<return_literal *>(return_result.get())) {
            result = std::move(return_result);
            break;
        }
    }

    return result;
}

an_ptr interpret_body(std::vector<std::shared_ptr<astToken>> body, scope *env) {
    an_ptr result = nullptr;

    for (auto &token : body) {
        an_ptr return_result = interpret(*token, env);

        if (dynamic_cast<return_literal *>(return_result.get())) {
            result = std::move(return_result);
            break;
        }

        if (dynamic_cast<break_literal *>(return_result.get())) {
            result = std::move(return_result);
            break;
        }

        if (dynamic_cast<continue_literal *>(return_result.get())) {
            result = std::move(return_result);
            break;
        }
    }

    return result;
}

an_ptr interpret_function_declaration(astToken &node, scope *env) {
    gem_value *function_value = make_value(gem_type::gem_function, true, env);
    function *func = new function;
    func->function_type = gem_function_type::default_function;
    func->body = node.body;
    func->declaration_enviroment = env;
    func->params = node.params;

    function_value->func = func;

    an_ptr value = std::make_unique<abstract_node>();

    if (node.name == "") {
        value->value = function_value;
    } else {
        value->value = env->make_variable(node.name, function_value);
    }

    return value;
}

an_ptr interpret_call_expr(astToken &node, scope *env) {
    an_ptr fn = interpret(*node.caller, env);

    if (fn->value->value_type != gem_type::gem_function) {
        error(error_type::runtime_error,
            "",
            env->file_name,
            node.line,
            "Cannot call a non-function value(" +
                std::string(magic_enum::enum_name(fn->value->value_type)) +
                ")");
        exit(1);
    };

    std::vector<gem_value *> args;

    for (auto &value : node.args) {
        args.push_back(interpret(*value, env)->value);
    }

    an_ptr return_result = std::make_unique<abstract_node>();

    if (fn->value->func->function_type == gem_function_type::native_function) {
        gem_value *result = fn->value->func->caller(args, env, node.line);
        return_result->value =
            result != nullptr ? result : env->get_variable("null");
    } else if (fn->value->func->function_type ==
               gem_function_type::metadata_function) {
        args.insert(args.begin(), interpret(*node.caller->object, env)->value);
        gem_value *result = fn->value->func->caller(args, env, node.line);
        return_result->value =
            result != nullptr ? result : env->get_variable("null");
    } else {
        scope *scope_env = new scope(false);
        scope_env->file_name =
            fn->value->func->declaration_enviroment->file_name;
        scope_env->parent_env = fn->value->func->declaration_enviroment;
        mark_scope(scope_env);
        push_heap_closures(scope_env);

        fn->value->func->declaration_enviroment->closures.push_back(scope_env);

        int param_count = fn->value->func->params.size();

        for (int index = 0; index < param_count; ++index) {
            scope_env->make_variable(fn->value->func->params[index],
                args.size() >= (index + 1) ? args[index]
                                           : env->get_variable("null"));
        }

        an_ptr result = interpret_body(fn->value->func->body, scope_env);
        return_result->value =
            result != nullptr ? result->value : env->get_variable("null");
        scope_erase(fn->value->func->declaration_enviroment, scope_env);
    }

    return return_result;
}

an_ptr interpret_while_loop(astToken &node, scope *env) {
    scope *scope_env = new scope(false);
    scope_env->file_name = env->file_name;
    scope_env->parent_env = env;
    mark_scope(scope_env);
    push_heap_closures(scope_env);

    env->closures.push_back(scope_env);

    while (is_truthy(interpret(*node.left, scope_env)->value)) {
        an_ptr return_result = interpret_body(node.body, scope_env);

        if (dynamic_cast<return_literal *>(return_result.get())) {
            scope_erase(env, scope_env);

            return std::move(return_result);
        }

        if (dynamic_cast<break_literal *>(return_result.get())) {
            break;
        }

        if (dynamic_cast<continue_literal *>(return_result.get())) {
            continue;
        }
    }

    auto value = std::make_unique<abstract_node>();
    value->value = env->get_variable("null");

    scope_erase(env, scope_env);

    return value;
}

an_ptr interpret_for_loop(astToken &node, scope *env) {
    scope *scope_env = new scope(false);
    scope_env->file_name = env->file_name;
    scope_env->parent_env = env;
    mark_scope(scope_env);
    push_heap_closures(scope_env);

    env->closures.push_back(scope_env);

    auto iterator = node.iterator;

    if (std::holds_alternative<std::shared_ptr<astToken>>(iterator)) {
        // iterator function loop
    } else {
        // numerical loop
        auto new_iterator =
            std::get<std::vector<std::shared_ptr<astToken>>>(iterator);

        if (new_iterator.size() < 2) {
            error(error_type::runtime_error,
                "",
                scope_env->file_name,
                node.line,
                "Missing iterator in for loop declaration!");
            exit(1);
        }

        auto start = new_iterator[0];
        auto end = new_iterator[1];
        auto step = new_iterator.size() > 2
                        ? new_iterator[2]
                        : std::make_shared<astToken>(astToken{
                              .kind = tokenKind::NumberLiteral, .value = "1"});

        if (node.params.size() == 0) {
            error(error_type::runtime_error,
                "",
                scope_env->file_name,
                node.line,
                "Missing variable in for loop declaration!");
            exit(1);
        }
        std::stringstream sds;

        gem_value *start_value = scope_env->make_variable(
            node.params[0], interpret(*start, scope_env)->value);

        sds << start_value;
        scope_env->make_variable(sds.str(), start_value);
        sds.str("");
        sds.clear();
        
        gem_value *end_value = interpret(*end, scope_env)->value;

        sds << end_value;
        scope_env->make_variable(sds.str(), end_value);
        sds.str("");
        sds.clear();

        gem_value *step_value = interpret(*step, scope_env)->value;

        sds << step_value;
        scope_env->make_variable(sds.str(), step_value);
        sds.str("");
        sds.clear();

        if (start_value->value_type != gem_type::gem_number) {
            error(error_type::runtime_error,
                add_pointers("^", "(x, ?, ?)", 1, 1),
                scope_env->file_name,
                node.line,
                "Expected gem_number, got " + std::string(magic_enum::enum_name(
                                                  start_value->value_type)));
            exit(1);
        }

        if (end_value->value_type != gem_type::gem_number) {
            error(error_type::runtime_error,
                add_pointers("^", "(?, x, ?)", 4, 4),
                scope_env->file_name,
                node.line,
                "Expected gem_number, got " +
                    std::string(magic_enum::enum_name(end_value->value_type)));
            exit(1);
        }

        if (step_value->value_type != gem_type::gem_number) {
            error(error_type::runtime_error,
                add_pointers("^", "(?, ?, x)", 7, 7),
                scope_env->file_name,
                node.line,
                "Expected gem_number, got " +
                    std::string(magic_enum::enum_name(step_value->value_type)));
            exit(1);
        }

        while (start_value->number < end_value->number) {
            an_ptr return_result = interpret_body(node.body, scope_env);

            if (dynamic_cast<return_literal *>(return_result.get())) {
                scope_erase(env, scope_env);

                return std::move(return_result);
            }

            if (dynamic_cast<break_literal *>(return_result.get())) {
                break;
            }

            if (dynamic_cast<continue_literal *>(return_result.get())) {
                start_value->number += step_value->number;
                continue;
            };

            start_value->number += step_value->number;
        }
    }

    auto value = std::make_unique<abstract_node>();
    value->value = env->get_variable("null");
    scope_erase(env, scope_env);

    return value;
}

an_ptr interpret_keyword(astToken &node, scope *env) {
    if (node.value == "break") {
        auto break_value = std::make_unique<break_literal>();
        return break_value;
    } else if (node.value == "continue") {
        auto continue_value = std::make_unique<continue_literal>();
        return continue_value;
    } else {
        error(error_type::runtime_error,
            add_pointers("~", node.value, 0, node.value.size()),
            env->file_name,
            node.line,
            "Invalid keyword!");
        exit(1);
    }
}

an_ptr interpret_identifier(astToken &node, scope *env) {
    an_ptr value = std::make_unique<abstract_node>();
    value->value = env->get_variable(node.value);
    return value;
}

an_ptr interpret_assignment(astToken &node, scope *env) {
    if (node.left->kind == tokenKind::MemberExpr) {
        an_ptr left = interpret(*node.left, env);
        an_ptr right = interpret(*node.right, env);

        left->value = right->value;

        an_ptr value = std::make_unique<abstract_node>();
        value->value = left->value;

        return value;

    } else {
        auto left = node.left->value;
        an_ptr right = interpret(*node.right, env);
        gem_value *assigned = env->set_variable(left, right->value);

        if (assigned == nullptr) {
            std::string message =
                left + " " + node.op + " " +
                std::string(magic_enum::enum_name(right->value->value_type));

            error(error_type::runtime_error,
                add_pointers("~", message, 0, message.size()),
                env->file_name,
                node.line,
                "Non-existent variable!");
            exit(1);
        }

        an_ptr value = std::make_unique<abstract_node>();
        value->value = assigned;

        return value;
    }
}

an_ptr interpret_var_declaration(astToken &node, scope *env) {
    an_ptr value = std::make_unique<abstract_node>();
    value->value =
        env->make_variable(node.name, interpret(*node.right, env)->value);
    return value;
}

an_ptr interpret_return(astToken &node, scope *env) {
    std::unique_ptr<return_literal> return_token =
        std::make_unique<return_literal>();
    return_token->value = interpret(*node.right, env)->value;
    mark_value(return_token->value);

    return return_token;
}

std::unique_ptr<number_literal> interpret_numeric_literal(
    astToken &node, scope *env) {
    std::unique_ptr<number_literal> value = std::make_unique<number_literal>();
    value->value = make_value(gem_type::gem_number, true, env);
    value->value->number = std::stod(node.value);
    return value;
}

std::unique_ptr<string_literal> interpret_string_literal(
    astToken &node, scope *env) {
    std::unique_ptr<string_literal> value = std::make_unique<string_literal>();
    value->value = make_value(gem_type::gem_string, true, env);
    value->value->string = node.value;
    value->value->metadata = root->get_variable("string")->table;

    return value;
}

std::unique_ptr<boolean_literal> interpret_boolean_literal(
    astToken &node, scope *env) {
    std::unique_ptr<boolean_literal> value =
        std::make_unique<boolean_literal>();
    value->value = make_value(gem_type::gem_bool, true, env);
    value->value->boolean = (node.value == "false" ? false : true);
    return value;
}

gem_value *number_operation(
    an_ptr &left, an_ptr &right, std::string op, scope *env) {
    gem_value *value = make_value(gem_type::gem_number, true, env);

    if (op == "+")
        value->number = (left->value->number) + (right->value->number);
    else if (op == "-")
        value->number = (left->value->number) - (right->value->number);
    else if (op == "*")
        value->number = (left->value->number) * (right->value->number);
    else if (op == "*")
        value->number = (left->value->number) / (right->value->number);
    else if (op == "^")
        value->number = std::pow((left->value->number), (right->value->number));
    else if (op == "%")
        value->number =
            std::fmod((left->value->number), (right->value->number));
    return value;
}

an_ptr interpret_binary_operation(astToken &node, scope *env) {
    an_ptr left = interpret(*node.left, env);
    an_ptr right = interpret(*node.right, env);

    if (left->value->value_type == gem_type::gem_number &&
        right->value->value_type == gem_type::gem_number) {
        std::unique_ptr<number_literal> value =
            std::make_unique<number_literal>();
        value->value = number_operation(left, right, node.op, env);
        return value;
    } else if (left->value->value_type == gem_type::gem_string &&
               right->value->value_type == gem_type::gem_string &&
               node.op == "+") {
        std::unique_ptr<string_literal> value =
            std::make_unique<string_literal>();
        value->value = make_value(gem_type::gem_string, true, env);
        new (&value->value->string) std::string();

        value->value->string = (left->value->string) + (right->value->string);
        return value;
    } else {
        error(error_type::runtime_error,
            dynamic_format("Attempted to use the '{}' operator on {} and {}!",
                node.op,
                gem_type_tostring(left->value->value_type),
                gem_type_tostring(right->value->value_type)),
            env->file_name,
            node.line);
        exit(1);
    }
}

std::unique_ptr<boolean_literal> interpret_comparasion(
    astToken &node, scope *env) {
    an_ptr left = interpret(*node.left, env);
    an_ptr right = interpret(*node.right, env);

    auto boolean_value = std::make_unique<boolean_literal>();
    gem_type left_type = left->value->value_type;
    gem_type right_type = right->value->value_type;

    if (left_type == right_type) {
        if (left_type == gem_type::gem_number) {
            boolean_value->value =
                compare_number(
                    left->value->number, right->value->number, node.op)
                    ? env->get_variable("true")
                    : env->get_variable("false");
        } else if (left_type == gem_type::gem_string) {
            boolean_value->value =
                compare_string(
                    left->value->string, right->value->string, node.op)
                    ? env->get_variable("true")
                    : env->get_variable("false");
        } else if (left_type == gem_type::gem_bool) {
            boolean_value->value =
                compare_bool(
                    left->value->boolean, right->value->boolean, node.op)
                    ? env->get_variable("true")
                    : env->get_variable("false");
        } else if (left_type == gem_type::gem_table) {
            boolean_value->value =
                compare_table(left->value->table, right->value->table, node.op)
                    ? env->get_variable("true")
                    : env->get_variable("false");
        } else if (left_type == gem_type::gem_function) {
            boolean_value->value =
                compare_function(left->value->func, right->value->func, node.op)
                    ? env->get_variable("true")
                    : env->get_variable("false");
        } else {
            std::string message =
                std::string(magic_enum::enum_name(left_type)) + " " + node.op +
                " " + std::string(magic_enum::enum_name(right_type));
            std::string pointer_message =
                add_pointers("~", message, 0, message.size());
            error(error_type::runtime_error,
                pointer_message,
                env->file_name,
                node.line,
                "Invalid value comparassion!");
            exit(1);
        }
    } else {
        boolean_value->value = env->get_variable("false");
    }

    return boolean_value;
}

std::unique_ptr<boolean_literal> interpret_logic_gate(
    astToken &node, scope *env) {
    an_ptr left = interpret(*node.left, env);
    an_ptr right = interpret(*node.right, env);
    auto boolean_value = std::make_unique<boolean_literal>();

    if (node.op == "and") {
        if (left->value->value_type == gem_type::gem_null ||
            left->value->boolean == false) {
            boolean_value->value = left->value;
            return boolean_value;
        } else {
            boolean_value->value = right->value;
            return boolean_value;
        }
    } else if (node.op == "or") {
        if (left->value->value_type == gem_type::gem_null ||
            left->value->boolean == false) {
            boolean_value->value = right->value;
            return boolean_value;
        } else {
            boolean_value->value = left->value;
            return boolean_value;
        }
    } else {
        exit(1);
    }
}

std::unique_ptr<number_literal> interpret_unary(astToken &node, scope *env) {
    auto value = std::make_unique<number_literal>();
    auto original_value = interpret(*node.right, env)->value;

    if (node.op == "-") {
        if (original_value->value_type != gem_type::gem_number) {
            error(error_type::runtime_error,
                add_pointers("^", node.op + "x", 1, 1),
                env->file_name,
                node.line,
                "Invalid unary expression! Expected number, got " +
                    std::string(
                        magic_enum::enum_name(original_value->value_type)) +
                    "!");

            exit(1);
        }
        original_value->number *= -1;
        value->value = original_value;
    } else if (node.op == "!") {
        gem_value *truthy = is_truthy(original_value) == true
                                ? env->get_variable("false")
                                : env->get_variable("true");
        value->value = truthy;
    } else {
        error(error_type::runtime_error,
            add_pointers("^", node.op + "x", 0, 0),
            env->file_name,
            node.line,
            "Invalid unary expression!");
        exit(1);
    }

    return value;
}

an_ptr interpret_member_expression(astToken &node, scope *env) {
    an_ptr value = std::make_unique<abstract_node>();

    if (node.computed == true) {
        an_ptr obj = interpret(*node.object, env);
        an_ptr ident = interpret(*node.property, env);

        if (ident->value->value_type == gem_type::gem_number) {
            if (!obj->value || obj->value->table->array.size() == 0 ||
                (obj->value->table->array.size() - 1) < ident->value->number) {
                std::string last = "[" + node.property->value + "]";
                std::string nmb = trace_back_member_expression(node);
                error(error_type::runtime_error,
                    add_pointers("~",
                        nmb,
                        (nmb.size() - last.size()) + 1,
                        nmb.size() - 2),
                    env->file_name,
                    node.line,
                    "Out of bounds!");
                exit(1);
            }
            value->value = obj->value->table->array[ident->value->number];
        } else {
            gem_value *at_position_value =
                obj->value->table->hash_at(ident->value);
            if (at_position_value == nullptr) {
                if (obj->value->metadata == nullptr) {
                    error(error_type::runtime_error,
                        "",
                        env->file_name,
                        node.line,
                        "Attempted to index metadata of a non-metadata value!");
                    exit(1);
                }
                gem_value *meta = obj->value->metadata->hash_at(ident->value);

                value->value =
                    meta == nullptr ? env->get_variable("null") : meta;
            } else {
                value->value = at_position_value;
            }
        }
    } else {
        an_ptr obj = interpret(*node.object, env);
        std::string ident = node.property->value;

        if (obj->value->value_type != gem_type::gem_table) {
            error(error_type::runtime_error,
                "",
                env->file_name,
                node.line,
                "Expected table, got " +
                    gem_type_tostring(obj->value->value_type));
            exit(1);
        }

        auto key = make_value(gem_type::gem_string, true, env);
        key->string = ident;

        gem_value *returned = obj->value->table->hash_at(key);

        if (returned == nullptr) {
            if (obj->value->metadata == nullptr) {
                error(error_type::runtime_error,
                    "",
                    env->file_name,
                    node.line,
                    "Attempted to index metadata of a non-metadata value!");
                exit(1);
            }
            gem_value *meta = obj->value->metadata->hash_at(key);

            value->value = meta == nullptr ? env->get_variable("null") : meta;
        } else {
            value->value = returned;
        }
    }

    return value;
}

an_ptr interpret_table_expression(astToken &node, scope *env) {
    an_ptr value = std::make_unique<abstract_node>();
    gem_table *table = new gem_table;

    gem_value *object = make_value(gem_type::gem_table, true, env);
    object->table = table;

    object->metadata = root->get_variable("table")->table;

    for (auto &property : node.properties) {
        gem_value *key = nullptr;

        if (property.key->kind == tokenKind::Identifier) {
            key = make_value(gem_type::gem_string, true, env);

            key->string = property.key->value;
        } else {
            key = interpret(*property.key, env)->value;
        }
        gem_value *value_at_key = interpret(*property.value, env)->value;
        if (key->value_type == gem_type::gem_number) {
            if (key->number >= table->array.size()) {
                table->array.resize(key->number + 1, nullptr);
            }

            table->array[key->number] = value_at_key;
        } else {
            table->hash_make(key, value_at_key);
        }
    }

    value->value = object;

    return value;
}

an_ptr interpret(astToken &node, scope *env) {
    switch (node.kind) {
    case tokenKind::Program:
        return interpret_program(node, env);
    case tokenKind::Identifier:
        return interpret_identifier(node, env);
    case tokenKind::NumberLiteral:
        return interpret_numeric_literal(node, env);
    case tokenKind::StringLiteral:
        return interpret_string_literal(node, env);
    case tokenKind::BooleanLiteral:
        return interpret_boolean_literal(node, env);
    case tokenKind::VariableDeclaration:
        return interpret_var_declaration(node, env);
    case tokenKind::BinaryExpr:
        return interpret_binary_operation(node, env);
    case tokenKind::Keyword:
        return interpret_keyword(node, env);
    case tokenKind::WhileLoopStmt:
        return interpret_while_loop(node, env);
    case tokenKind::ComparisonExpr:
        return interpret_comparasion(node, env);
    case tokenKind::LogicGateExpr:
        return interpret_logic_gate(node, env);
    case tokenKind::AssignmentExpr:
        return interpret_assignment(node, env);
    case tokenKind::ForLoopStmt:
        return interpret_for_loop(node, env);
    case tokenKind::FunctionDeclaration:
        return interpret_function_declaration(node, env);
    case tokenKind::CallExpr:
        return interpret_call_expr(node, env);
    case tokenKind::ReturnStmt:
        return interpret_return(node, env);
    case tokenKind::UnaryExpr:
        return interpret_unary(node, env);
    case tokenKind::ObjectLiteral:
        return interpret_table_expression(node, env);
    case tokenKind::MemberExpr:
        return interpret_member_expression(node, env);
    default:
        error(error_type::runtime_error,
            std::string(magic_enum::enum_name(node.kind)),
            env->file_name,
            node.line,
            "Invalid AST!");
        exit(1);
    }
}

// GC

void mark_value(gem_value *value) {
    if (!value || value->marked)
        return;
    value->marked = true;

    if (value->value_type == gem_type::gem_function && value->func) {
        mark_scope(value->func->declaration_enviroment);
        return;
    }

    if (value->value_type == gem_type::gem_table && value->table) {
        auto t = value->table;
        for (gem_value *v : t->array) {
            mark_value(v);
        }

        for (gem_entry *head : t->buckets) {
            gem_entry *entry = head;
            while (entry) {
                mark_value(entry->value);
                entry = entry->next;
            }
        }
    }

    mark_scope(value->declaration_env);
}

void mark_scope(scope *env) {
    if (!env || env->marked)
        return;
    env->marked = true;

    for (auto &value : env->stack) {
        gem_value *second = value.second;

        mark_value(second);
    }

    for (auto &value : env->closures) {
        mark_scope(value);
    }
}

void garbage_collect() {
    mark_scope(root);

    int closure_deleted = 0;
    int objects_deleted = 0;

    for (auto it = gem_heap_closures.begin(); it != gem_heap_closures.end();) {
        scope *env = *it;
        if (!env->marked) {
            delete env;
            closure_deleted++;
            it = gem_heap_closures.erase(it);
        } else {
            env->marked = false;
            ++it;
        }
    }

    for (auto it = gem_heap_objects.begin(); it != gem_heap_objects.end();) {
        gem_value *value = *it;

        if (!value->marked) {
            delete value;
            objects_deleted++;
            it = gem_heap_objects.erase(it);
        } else {
            value->marked = false;
            ++it;
        }
    }

    std::cout << "There is " << gem_heap_objects.size() << " objects and "
              << gem_heap_closures.size() << " closures." << std::endl;

    std::cout << "Deleted " << objects_deleted << " objects and "
              << closure_deleted << " closures." << std::endl;
}

std::string gem_hash_tostring(gem_value *value) {
    switch (value->value_type) {
    case gem_type::gem_number:
        return "n" + std::to_string(value->number);
    case gem_type::gem_string:
        return "s" + value->string;
    case gem_type::gem_bool:
        return "b" + std::to_string(value->boolean);
    case gem_type::gem_table: {
        std::stringstream ss;
        ss << value->table;
        return "t" + ss.str();
    }
    case gem_type::gem_function: {
        std::stringstream ss;
        ss << value->func;
        return "f" + ss.str();
    }
    case gem_type::gem_null:
        return "";
    default:
        return "";
    }
}