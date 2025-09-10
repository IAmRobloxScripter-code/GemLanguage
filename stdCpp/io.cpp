#include "../Gem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
cstring gem_tostring(double n) {
    static char buffer[64];
    snprintf(buffer, sizeof(buffer), "%f", n);
    return buffer;
}

double gem_tonumber(cstring s) {
    return atof(s);
}

cTable *gem_fopen(cstring path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        cTable *t = table_new();
        table_init(t);

        cTableNode n1;
        memset(&n1, 0, sizeof(cTableNode));
        n1.key.type = KEY_STRING;
        n1.key.s = strdup("content");
        n1.value.type = VAL_STRING;
        n1.value.s = strdup("");

        cTableNode n2;
        memset(&n2, 0, sizeof(cTableNode));
        n2.key.type = KEY_STRING;
        n2.key.s = strdup("exists");
        n2.value.type = VAL_BOOL;
        n2.value.b = false;

        table_push(t, n1);
        table_push(t, n2);

        return t;
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    cstring buffer = (cstring)malloc(size + 1);
    if (!buffer) {
        fclose(fp);
        cTable *t = table_new();
        table_init(t);

        cTableNode n1;
        memset(&n1, 0, sizeof(cTableNode));
        n1.key.type = KEY_STRING;
        n1.key.s = strdup("content");
        n1.value.type = VAL_STRING;
        n1.value.s = strdup("");

        cTableNode n2;
        memset(&n2, 0, sizeof(cTableNode));
        n2.key.type = KEY_STRING;
        n2.key.s = strdup("exists");
        n2.value.type = VAL_BOOL;
        n2.value.b = true;

        table_push(t, n1);
        table_push(t, n2);

        return t;
    }

    size_t read_bytes = fread(buffer, 1, size, fp);
    buffer[read_bytes] = '\0';
    fclose(fp);

    cTable *object = table_new();
    table_init(object);

    cTableNode n1;
    memset(&n1, 0, sizeof(cTableNode));
    n1.key.type = KEY_STRING;
    n1.key.s = strdup("content");
    n1.value.type = VAL_STRING;
    n1.value.s = strdup(buffer);

    cTableNode n2;
    memset(&n2, 0, sizeof(cTableNode));
    n2.key.type = KEY_STRING;
    n2.key.s = strdup("exists");
    n2.value.type = VAL_BOOL;
    n2.value.b = true;

    table_push(object, n1);
    table_push(object, n2);
    fflush(stdout);

    free(buffer);

    return object;
}
}