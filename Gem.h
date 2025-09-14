#pragma once

#include <stddef.h>
#include <stdbool.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>

typedef enum {
    VAL_INT,
    VAL_DOUBLE,
    VAL_BOOL,
    VAL_STRING,
    VAL_CALLBACK,
    VAL_TABLE,
    VAL_FFI_FUNC
} cValueType;

typedef char* cstring;
typedef void (*cCallback)(void);

typedef void (*cFFI_FuncPtr)(void);

typedef enum { KEY_INT, KEY_DOUBLE, KEY_STRING } KeyType;

typedef struct {
    KeyType type;
    union {
        int i;
        double d;
        cstring s;
    };
} cKey;

struct cTableNode;

typedef struct {
    cValueType type;
    union {
        void* cb;                  
        int i;
        double d;
        bool b;
        const char* s;            
        struct cTableNode* table; 
    };
    size_t table_len;
} cValue;

typedef struct cTableNode {
    cKey key;
    cValue value;
} cTableNode;

typedef struct {
    cTableNode* nodes;
    size_t len;
} cTable;

inline void table_init(cTable* t) {
    t->nodes = NULL;
    t->len = 0;
}

inline void table_push(cTable* t, cTableNode node) {
    cTableNode* new_nodes = (cTableNode*)realloc(t->nodes, (t->len + 1) * sizeof(cTableNode));
    if (!new_nodes) {
        perror("realloc failed");
        exit(1);
    }
    t->nodes = new_nodes;
    t->nodes[t->len] = node;
    t->len += 1;
}

inline cTable* table_new() {
    return (cTable*)malloc(sizeof(cTable));
}