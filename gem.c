#define O_DEBUG
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    gem_number,
    gem_string,
    gem_bool
} gem_object_type;

typedef struct gem_object gem_object;

struct gem_object {
    gem_object_type object_type;
    uint64_t references;

    void (*deconstructor)(gem_object *);
    void (*print)(gem_object *);
};

typedef struct {
    gem_object base;
    double value;
} gem_object_number;

typedef struct {
    gem_object base;
    bool value;
} gem_object_bool;

typedef struct {
    gem_object base;
    uint64_t size;
    char *value;
} gem_object_string;

typedef struct {
    gem_object *key;
    gem_object *value;
} gem_object_table_node;

typedef struct {
    gem_object base;
    gem_object_table_node *contents[];

} gem_object_table;

void gem_check_free(gem_object *obj) {
    if (obj->references == 0) {
#ifdef O_DEBUG
        printf("Freed Address: %p\n", obj);
#endif
        obj->deconstructor(obj);
    }
}

gem_object *gem_assign(gem_object *self, gem_object *value) {
    self->references--;
    value->references++;

    gem_check_free(self);

    return value;
}

void gem_default_deconstructor(gem_object *self) {
    free(self);
}

void gem_string_deconstructor(gem_object *self) {
    free((((gem_object_string *)self)->value));
    free(self);
}

gem_object *make_number(double value) {
    gem_object_number *ptr = malloc(sizeof(gem_object_number));
#ifdef O_DEBUG
    printf("New Object At Address: %p\n", ptr);
#endif

    ptr->base.object_type = gem_number;
    ptr->value = value;
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_default_deconstructor;

    return (gem_object *)ptr;
}

gem_object *make_bool(bool value) {
    gem_object_bool *ptr = malloc(sizeof(gem_object_bool));
#ifdef O_DEBUG
    printf("New Object At Address: %p\n", ptr);
#endif

    ptr->base.object_type = gem_bool;
    ptr->value = value;
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_default_deconstructor;

    return (gem_object *)ptr;
}

gem_object *make_string(const char *value) {
    gem_object_string *ptr = malloc(sizeof(gem_object_string));
#ifdef O_DEBUG
    printf("New Object At Address: %p\n", ptr);
#endif

    ptr->base.object_type = gem_string;
    ptr->value = malloc(strlen(value) + 1);
    ptr->size = strlen(value) + 1;
    memcpy(ptr->value, value, ptr->size);
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_string_deconstructor;

    return (gem_object *)ptr;
}

gem_object *make_table(int size) {
}

// Number Operations

gem_object *number_add(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value + ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_sub(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value - ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_mul(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value * ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_div(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value / ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_pow(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_number(
        pow(((gem_object_number *)x)->value, ((gem_object_number *)y)->value));
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_mod(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_number(modf(
        ((gem_object_number *)x)->value, &((gem_object_number *)y)->value));
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

// String Operations

gem_object *string_add(gem_object *x, gem_object *y) {
    if (x->object_type != gem_string || y->object_type != gem_string) {
        printf("Invalid type operation!");
        exit(1);
    };

    char *result = malloc(sizeof(char) * ((gem_object_string *)x)->size +
                          ((gem_object_string *)y)->size - 1);
    memcpy(result,
        ((gem_object_string *)x)->value,
        ((gem_object_string *)x)->size - 1);
    memcpy(result + ((gem_object_string *)x)->size - 1,
        ((gem_object_string *)x)->value,
        ((gem_object_string *)y)->size);
    gem_object *temporary = make_string(result);
    free(result);

    gem_check_free(x);
    gem_check_free(y);

    return temporary;
}

// Conditionals

gem_object *number_greaterThan(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value > ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_lessThan(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value < ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_greaterThanEqual(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value >= ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_lessThanEqual(gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("Invalid type operation!");
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value <= ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *gem_equal(gem_object *x, gem_object *y) {
    bool equal = false;

    if (x->object_type == gem_number && y->object_type == gem_number) {
        equal =
            ((gem_object_number *)x)->value == ((gem_object_number *)y)->value;
    } else if (x->object_type == gem_string && y->object_type == gem_string) {
        equal = strcmp(((gem_object_string *)x)->value,
                    ((gem_object_string *)y)->value) == 0;
    } else if (x->object_type == gem_bool && y->object_type == gem_bool) {
        equal = ((gem_object_bool *)x)->value == ((gem_object_bool *)y)->value;
    } else {
        equal = false;
    }

    gem_object *result = make_bool(equal);

    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *gem_notEqual(gem_object *x, gem_object *y) {
    bool equal = false;

    if (x->object_type == gem_number && y->object_type == gem_number) {
        equal =
            ((gem_object_number *)x)->value != ((gem_object_number *)y)->value;
    } else if (x->object_type == gem_string && y->object_type == gem_string) {
        equal = strcmp(((gem_object_string *)x)->value,
                    ((gem_object_string *)y)->value) != 0;
    } else if (x->object_type == gem_bool && y->object_type == gem_bool) {
        equal = ((gem_object_bool *)x)->value != ((gem_object_bool *)y)->value;
    } else {
        equal = true;
    }

    gem_object *result = make_bool(equal);

    gem_check_free(x);
    gem_check_free(y);

    return result;
}

// Unary

gem_object *gem_not_operator(gem_object *value) {
    switch (value->object_type) {
    case gem_number:
        return make_bool(!((gem_object_number *)value)->value);
    case gem_bool:
        return make_bool(!((gem_object_bool *)value)->value);
    case gem_string:
        return make_bool(!((gem_object_string *)value)->value);
    default:
        printf("Invalid type for not operator!");
        exit(1);
    }
}

int main() {
if (gem_equal(number_add(make_number(10), make_number(10)), make_number(20))) {
gem_object* x = make_string("lol");
x->references++;

x->references--;
gem_check_free(x);
}
else if (gem_equal(number_add(make_number(10), make_number(10)), make_number(30))) {
gem_object* b = make_string("crazy");
b->references++;

b->references--;
gem_check_free(b);
}
else {
gem_object* c = make_string("Gem");
c->references++;

c->references--;
gem_check_free(c);
}


}