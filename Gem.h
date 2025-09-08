#ifndef TABLE_FFI_H
#define TABLE_FFI_H

#include <stddef.h>
#include <stdbool.h>

typedef enum {
    KEY_INT,
    KEY_DOUBLE,
    KEY_STRING
} KeyType;

typedef enum {
    VAL_INT,
    VAL_DOUBLE,
    VAL_BOOL,
    VAL_STRING,
    VAL_CALLBACK,
    VAL_TABLE_PTR,
    VAL_FFI_FUNC_PTR
} ValueType;

struct TableNode;
typedef struct TableNode TableNode;

typedef void (*Callback)(void);
typedef void* FFI_FuncPtr;
typedef char* string;

typedef struct {
    ValueType type;
    union {
        int i;
        double d;
        bool b;
        const string s;
        Callback cb;
        TableNode* table;
        FFI_FuncPtr ffi_func;
    } data;
} valueVariant;

struct TableNode {
    string key;
    valueVariant value;
};

typedef struct {
    TableNode* nodes;
    size_t size;
} table;

#endif
