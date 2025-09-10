#pragma once
#include "../Gem.h"
#include "ffi.h"
#include "types.hpp"
#include <cstring>
#include <variant>

table CToCppTable(const cTable &ctable) {
    table cpp_table;
    cpp_table.reserve(ctable.len);

    for (size_t i = 0; i < ctable.len; ++i) {
        const cTableNode cnode = ctable.nodes[i];
        tableNode node;

        switch (cnode.key.type) {
        case KEY_INT:
            node.key = cnode.key.i;
            break;
        case KEY_DOUBLE:
            node.key = cnode.key.d;
            break;
        case KEY_STRING:
            node.key = "\"\"" + std::string(cnode.key.s) + "\"\"";
            break;
        }

        const cValue &cval = cnode.value;
        switch (cval.type) {
        case VAL_INT:
            node.value = cval.i;
            break;
        case VAL_DOUBLE:
            node.value = cval.d;
            break;
        case VAL_BOOL:
            node.value = cval.b;
            break;
        case VAL_STRING:
            node.value = "\"" + std::string(cval.s) + "\"";
            break;
        case VAL_TABLE: {
            cTable nested_c{cval.table, cval.table_len};
            node.value = CToCppTable(nested_c);
            break;
        }
        default:
            throw "Type not allowed";
        }

        cpp_table.push_back(std::move(node));
    }

    return cpp_table;
}

cTable cppToCTable(const table &cpp_table) {
    cTable ctable;
    ctable.len = cpp_table.size();
    ctable.nodes = (cTableNode *)malloc(sizeof(cTableNode) * ctable.len);

    for (size_t i = 0; i < ctable.len; ++i) {
        const auto &node = cpp_table[i];

        if (std::holds_alternative<int>(node.key)) {
            ctable.nodes[i].key.type = KEY_INT;
            ctable.nodes[i].key.i = std::get<int>(node.key);
        } else if (std::holds_alternative<double>(node.key)) {
            ctable.nodes[i].key.type = KEY_DOUBLE;
            ctable.nodes[i].key.d = std::get<double>(node.key);
        } else {
            ctable.nodes[i].key.type = KEY_STRING;
            ctable.nodes[i].key.s =
                strdup(std::get<std::string>(node.key).c_str());
        }

        const auto &val = node.value;
        if (std::holds_alternative<int>(val)) {
            ctable.nodes[i].value.type = VAL_INT;
            ctable.nodes[i].value.i = std::get<int>(val);
        } else if (std::holds_alternative<double>(val)) {
            ctable.nodes[i].value.type = VAL_DOUBLE;
            ctable.nodes[i].value.d = std::get<double>(val);
        } else if (std::holds_alternative<bool>(val)) {
            ctable.nodes[i].value.type = VAL_BOOL;
            ctable.nodes[i].value.b = std::get<bool>(val);
        } else if (std::holds_alternative<std::string>(val)) {
            ctable.nodes[i].value.type = VAL_STRING;
            ctable.nodes[i].value.s =
                strdup(std::get<std::string>(val).c_str());
        } else if (std::holds_alternative<table>(val)) {
            ctable.nodes[i].value.type = VAL_TABLE;
            table nested = std::get<table>(val);
            cTable nested_c = cppToCTable(nested);
            ctable.nodes[i].value.table = nested_c.nodes;
            ctable.nodes[i].value.table_len = nested_c.len;
        } else {
            throw "callback is not allowed";
        }
    }

    return ctable;
}

void freeCtable(const cTable &t) {
    for (size_t i = 0; i < t.len; ++i) {
        if (t.nodes[i].key.type == KEY_STRING) {
            free((void *)t.nodes[i].key.s);
        }

        auto &val = t.nodes[i].value;
        if (val.type == VAL_STRING) {
            free((void *)val.s);
        } else if (val.type == VAL_TABLE) {
            freeCtable({val.table, val.table_len});
        }
    }
    free(t.nodes);
}

inline void cleanStorage(
    std::vector<std::vector<char>> &arg_storage, void *values[], size_t n) {
    for (size_t i = 0; i < n; ++i) {
        void *ptr = nullptr;

        std::memcpy(&ptr, arg_storage[i].data(), sizeof(void *));
        if (ptr) {
            free(ptr);
        }

        values[i] = nullptr;
        arg_storage[i].clear();
    }
}

inline void convertToC(const FFI_Func &f,
    size_t &index,
    ffi_type *t,
    std::vector<std::vector<char>> &arg_storage,
    void *values[],
    std::vector<valueVariant> &args) {
    if (t == &ffi_type_double) {
        double val = std::get<double>(args[index]);
        std::memcpy(arg_storage[index].data(), &val, sizeof(double));
        values[index] = arg_storage[index].data();

    } else if (t == &ffi_type_sint) {
        int val = std::get<int>(args[index]);
        std::memcpy(arg_storage[index].data(), &val, sizeof(int));
        values[index] = arg_storage[index].data();
    } else if (t == &ffi_type_pointer) {
        if (std::holds_alternative<std::string>(args[index])) {
            std::string str = std::get<std::string>(args[index]);
            str.erase(0, 1);
            str.erase(str.size() - 1, 1);

            char *cstr_copy = (char *)malloc(str.size() + 1);
            std::memcpy(cstr_copy, str.c_str(), str.size());
            cstr_copy[str.size()] = '\0';

            std::memcpy(
                arg_storage[index].data(), &cstr_copy, sizeof(const char *));
            values[index] = arg_storage[index].data();
        } else if (std::holds_alternative<table>(args[index])) {
            table &object = std::get<table>(args[index]);
            cTable ptr = cppToCTable(object);

            std::memcpy(arg_storage[index].data(), &ptr, sizeof ptr);
            values[index] = arg_storage[index].data();
        } else {
            throw "Unexpected pointer argument";
        }
    }
}