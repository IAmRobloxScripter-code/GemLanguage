#pragma once
#include "./magic_enum/magic_enum.hpp"
#include "parser.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
extern u_int64_t MAX_ALLOCATIONS;

inline u_int64_t allocations = 0;

class scope;

enum class gem_type {
    gem_number,
    gem_string,
    gem_bool,
    gem_table,
    gem_function,
    gem_temp_closure,
    gem_null,
    gem_any // this will be used just for expecting types
};

enum class gem_function_type {
    native_function,
    default_function,
    metadata_function,
};

static std::string gem_type_tostring(gem_type value_type) {
    switch (value_type) {
    case gem_type::gem_number:
        return "number";
    case gem_type::gem_string:
        return "string";
    case gem_type::gem_bool:
        return "boolean";
    case gem_type::gem_table:
        return "table";
    case gem_type::gem_function:
        return "function";
    case gem_type::gem_any:
        return "any";
    default:
        return "null";
    }
}

struct gem_value;

std::string gem_hash_tostring(gem_value *value);

inline unsigned long hash_string(const std::string &str) {
    unsigned long hash = 5381;
    for (unsigned char c : str)
        hash = ((hash << 5) + hash) + c;
    return hash;
};

struct gem_entry {
    std::string key;
    gem_value *value;
    gem_entry *next;
};

struct gem_table {
    std::vector<gem_value *> array;
    std::vector<gem_entry *> buckets;

    size_t hash_capacity;
    size_t hash_size;

    inline void push_back(gem_value *value) {
        array.push_back(value);
    };

    inline void push_front(gem_value *value) {
        array.insert(array.begin(), value);
    };

    inline gem_value *pop_back() {
        if (array.empty())
            return nullptr;
        gem_value *value = array.back();
        array.pop_back();
        return value;
    };

    inline gem_value *pop_front() {
        if (array.empty())
            return nullptr;
        gem_value *value = array.front();
        array.erase(array.begin());
        return value;
    };

    inline gem_value *hash_at(gem_value *key) {
        std::string hashed_key = gem_hash_tostring(key);

        unsigned long h = hash_string(hashed_key);
        size_t idx = h % hash_capacity;

        gem_entry *entry = buckets[idx];
        while (entry) {
            if (entry->key == hashed_key) {
                return entry->value;
            }
            entry = entry->next;
        }

        return nullptr;
    };

    inline void resize_and_rehash() {
        size_t new_capacity = hash_capacity * 2;
        std::vector<gem_entry *> new_buckets(new_capacity, nullptr);

        for (gem_entry *old_bucket : buckets) {
            gem_entry *entry = old_bucket;
            while (entry) {
                gem_entry *next = entry->next;

                unsigned long h = hash_string(entry->key);
                size_t idx = h % new_capacity;

                entry->next = new_buckets[idx];
                new_buckets[idx] = entry;

                entry = next;
            }
        }

        buckets = std::move(new_buckets);
        hash_capacity = new_capacity;
    };

    inline gem_value *hash_make(gem_value *key, gem_value *value) {
        if (hash_size > hash_capacity * 0.75) {
            resize_and_rehash();
        };

        std::string hashed_key = gem_hash_tostring(key);

        unsigned long h = hash_string(hashed_key);
        size_t idx = h % hash_capacity;

        gem_entry *entry = buckets[idx];
        while (entry) {
            if (entry->key == hashed_key) {
                entry->value = value;
                return entry->value;
            }
            entry = entry->next;
        }

        gem_entry *new_entry = new gem_entry{hashed_key, value, buckets[idx]};
        buckets[idx] = new_entry;
        hash_size++;

        return new_entry->value;
    };

    ~gem_table() {
        for (auto head : buckets) {
            gem_entry *entry = head;
            while (entry) {
                gem_entry *next = entry->next;
                delete entry;
                entry = next;
            }
        }
    };

    gem_table() {
        hash_capacity = 16;
        buckets.resize(hash_capacity, nullptr);
        hash_size = 0;
    }
};

struct function {
    gem_function_type function_type;
    std::vector<std::string> params;
    std::vector<std::shared_ptr<astToken>> body;
    scope *declaration_enviroment;
    gem_value *(*caller)(std::vector<gem_value *>, scope *env, u_int64_t line);
};

struct temp_closure {
    scope *enviroment;
};

struct gem_value {
    gem_type value_type = gem_type::gem_null;
    union {
        double number;
        std::string string;
        bool boolean;
        gem_table *table;
        function *func;
        temp_closure *closure;
    };
    bool marked = false;

    gem_table *metadata;

    gem_value() {};

    ~gem_value() {
        if (value_type == gem_type::gem_string)
            string.~basic_string();
        else if (value_type == gem_type::gem_function)
            delete func;
        else if (value_type == gem_type::gem_table)
            delete table;
        else if (value_type == gem_type::gem_temp_closure)
            delete closure;
    };
};

inline std::vector<scope *> gem_heap_closures;
inline std::vector<gem_value *> gem_heap_objects;

void mark_scope(scope *env);
void mark_value(gem_value *value);

void garbage_collect();

inline void push_heap_closures(scope *closure) {
    gem_heap_closures.push_back(closure);
    allocations++;

    if (allocations > MAX_ALLOCATIONS) {
        allocations = 0;
        garbage_collect();
    }
};

inline void push_heap_objects(gem_value *object) {
    gem_heap_objects.push_back(object);
    allocations++;

    if (allocations > MAX_ALLOCATIONS) {
        allocations = 0;
        garbage_collect();
    }
};

inline gem_value *make_value(gem_type value_type, bool marked = false) {
    gem_value *value = new gem_value{};
    value->marked = marked;
    value->value_type = value_type;

    if (value_type == gem_type::gem_string) {
        new (&value->string) std::string();
    }
    push_heap_objects(value);

    return value;
};

class scope {
  public:
    std::string file_name;
    scope *parent_env = nullptr;
    std::unordered_map<std::string, gem_value *> stack;
    bool marked = false;

    scope(bool should_allocate = true) {
        if (should_allocate == true) {
            push_heap_closures(this);
        }
    }

    ~scope() {
    }

  public:
    scope *resolve(const std::string &identifier) {
        if (stack.find(identifier) != stack.end()) {
            return this;
        } else {
            return this->parent_env == nullptr
                       ? nullptr
                       : this->parent_env->resolve(identifier);
        }
    }

    gem_value *get_variable(const std::string &identifier) {
        scope *enviroment = resolve(identifier);
        return !enviroment ? get_variable("null")
                           : enviroment->stack[identifier];
    }

    gem_value *make_variable(const std::string &identifier, gem_value *value) {
        this->stack[identifier] = value;
        return value;
    }

    gem_value *set_variable(const std::string &identifier, gem_value *value) {
        scope *env = this->resolve(identifier);
        if (env == nullptr) {
            return nullptr;
        };

        env->stack[identifier] = value;
        return value;
    }
};

struct abstract_node {
    gem_value *value;
    virtual ~abstract_node() = default;
};

struct number_literal : public abstract_node {};
struct string_literal : public abstract_node {};
struct boolean_literal : public abstract_node {};
struct table_literal : public abstract_node {};
struct return_literal : public abstract_node {};
struct continue_literal : public abstract_node {};
struct break_literal : public abstract_node {};

std::unique_ptr<abstract_node> interpret(astToken &node, scope *env);

extern scope *root;
