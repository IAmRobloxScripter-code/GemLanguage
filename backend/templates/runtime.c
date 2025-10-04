#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    gem_number,
    gem_string,
    gem_bool,
		gem_function, 
		gem_table,
		gem_nil,
} gem_object_type;

typedef struct gem_object gem_object;

struct gem_object {
    gem_object_type object_type;
    uint64_t references;

    void (*deconstructor)(gem_object *);
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
} gem_object_nil;


typedef struct {
    gem_object base;
    uint64_t size;
    char *value;
} gem_object_string;



typedef struct{
	const char* Line_Info;
	bool native;
	uint64_t Line;
	const char* FileName;
} stack_trace_info;

typedef struct{
	uint64_t Size;
	uint64_t Cap;
	uint64_t Line;
	const char* FileName;
	stack_trace_info* Trace; 
} stack_trace; 


typedef struct {
    gem_object base;

		uint64_t incount;
		gem_object** internals;
		
		uint64_t expects;
		gem_object*(*FuncPtr)(stack_trace*, gem_object**, gem_object**);
} gem_object_function;


stack_trace* create_stack_trace(){
	stack_trace* st = malloc(sizeof(stack_trace));
	st->Size = 0;

	st->Cap = 100;
	st->Trace = malloc(sizeof(stack_trace_info) * 100);
	
	return st;
}

void trace_add_trace(stack_trace* st, uint64_t Line, const char* Line_Info, bool native, const char* filename){
	if(st->Size == st->Cap){
		uint64_t newcap = (uint64_t)(st->Cap * 1.7);
		stack_trace_info* Temp = malloc(sizeof(stack_trace_info*) * newcap);
		memmove(Temp, st->Trace, st->Cap * sizeof(stack_trace_info*));
		free(st->Trace); 
		st->Trace = Temp;
		st->Cap = newcap;
	}

	st->Trace[st->Size].Line_Info = Line_Info;
	st->Trace[st->Size].native = native;
	st->Trace[st->Size].Line = Line;
	st->Trace[st->Size].FileName = filename;
	++st->Size;
}
void back(stack_trace* st){
	--st->Size;
}

void print_trace(stack_trace* st){
	puts("stack traceback:");
	for(uint64_t i = st->Size - 1; i > 0; --i){
		if(st->Trace[i].native){
			printf("	[C]: %s\n", st->Trace[i].Line_Info);
		}else{
			printf("	%s:%" PRIu64 ": %s\n",  st->Trace[i].FileName,  st->Trace[i].Line, st->Trace[i].Line_Info);
		} 
	}
	if(st->Trace[0].native){
		printf("	[C]: %s\n", st->Trace[0].Line_Info);
	}else{
		printf("	%s:%" PRIu64 ": %s\n",  st->Trace[0].FileName,  st->Trace[0].Line, st->Trace[0].Line_Info);
	} 
}


void gem_check_free(gem_object*);

void gem_print(gem_object* value){
	switch(value->object_type){
		case gem_number:
			printf("%.12g", ((gem_object_number*)value)->value);
			break;
		case gem_string:
			printf("%s", ((gem_object_string*)value)->value);
			break;
		case gem_function:
			printf("Function: %p", ((gem_object_function*)value)->FuncPtr);
			break;
		case gem_bool:
			printf("%s", ((gem_object_bool*)value)->value ? "True" : "False");
			break;
		case gem_table:
			break;
		case gem_nil:
		default:
			printf("nil");
			break;
	}
	gem_check_free(value);
}

void destroy_stack(stack_trace* st){
	free(st->Trace);
	free(st);
}

const char* get_type_name(gem_object_type tp){
	switch(tp){
		case gem_number:
			return "number";
		case gem_string:
			return "string";
		case gem_function:
			return "function";
		case gem_bool:
			return "boolean";
		case gem_table:
			return "function";
		case gem_nil:
		default:
			return "nil";
	}
}
void gem_check_free(gem_object *obj) {
    if (obj->references == 0) {
#ifdef O_DEBUG
        printf("Freed %s Object Address: %p\n", get_type_name(obj->object_type), obj);
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

void gem_function_deconstructor(gem_object *self){
	for(uint64_t i = 0; i < ((gem_object_function*)self)->incount; ++i){
		((gem_object_function*)self)->internals[i]->references--;
		gem_check_free(((gem_object_function*)self)->internals[i]);
	}
	free(((gem_object_function*)self)->internals);
	free(self);
}

void gem_string_deconstructor(gem_object *self) {
    free((((gem_object_string *)self)->value));
    free(self);
}

gem_object *make_number(double value) {
    gem_object_number *ptr = malloc(sizeof(gem_object_number));
#ifdef O_DEBUG
    printf("New number Object At Address: %p\n", ptr);
#endif

    ptr->base.object_type = gem_number;
    ptr->value = value;
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_default_deconstructor;

    return (gem_object *)ptr;
}

gem_object *make_function(gem_object** internals, uint64_t incount, gem_object*(*FuncPtr)(stack_trace*, gem_object**, gem_object**), uint64_t expects){
	  gem_object_function *ptr = malloc(sizeof(gem_object_function));
		#ifdef O_DEBUG
    	printf("New function Object At Address: %p\n", ptr);
	#endif

    ptr->base.object_type = gem_function;
		ptr->expects = expects;
		ptr->FuncPtr = FuncPtr;
		ptr->internals = internals;
		ptr->incount = incount; 
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_function_deconstructor;
    return (gem_object *)ptr;
};

gem_object *make_nil() {
    gem_object_nil *ptr = malloc(sizeof(gem_object_nil));
#ifdef O_DEBUG
    printf("New nil Object At Address: %p\n", ptr);
#endif

    ptr->base.object_type = gem_nil;
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_default_deconstructor;

    return (gem_object *)ptr;
}
gem_object *make_bool(bool value) {
    gem_object_bool *ptr = malloc(sizeof(gem_object_bool));
#ifdef O_DEBUG
    printf("New bool Object At Address: %p\n", ptr);
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
    printf("New string Object At Address: %p\n", ptr);
#endif

    ptr->base.object_type = gem_string;
    ptr->value = malloc(strlen(value) + 1);
    ptr->size = strlen(value) + 1;
    memcpy(ptr->value, value, ptr->size);
    ptr->base.references = 0;
    ptr->base.deconstructor = gem_string_deconstructor;

    return (gem_object *)ptr;
}

gem_object *copy_object(gem_object* obj){
	switch(obj->object_type){
		case gem_number:
			return make_number( ((gem_object_number*)obj)->value );
		case gem_string:
			return make_string( ((gem_object_string*)obj)->value );
		case gem_function:
			gem_object** internals = malloc( sizeof(gem_object*) * ((gem_object_function*)obj)->incount );
			for(uint64_t i = 0; i < ((gem_object_function*)obj)->incount; ++i){
				internals[i] = copy_object( ((gem_object_function*)obj)->internals[i]);
				internals[i]->references++;
			}
			return make_function(internals, ((gem_object_function*)obj)->incount, ((gem_object_function*)obj)->FuncPtr, ((gem_object_function*)obj)->expects);
		case gem_bool:
			return make_bool( ((gem_object_string*)obj)->value);
		case gem_table:
		case gem_nil:
		default:
			return make_nil();
	}
}

gem_object* gem_call_func(gem_object* Func, stack_trace* st, uint64_t count, gem_object** args){
	gem_object** arguments = malloc( ((gem_object_function*)Func)->expects * sizeof(gem_object*) );
	for(uint64_t i = 0; i < count; ++i){
			if(i >= ((gem_object_function*)Func)->expects){
				gem_check_free(args[i]);
			}else{
				arguments[i] = args[i];	
			}
	}
	for(uint64_t i = count; i < ((gem_object_function*)Func)->expects; i++){
		arguments[i] = make_nil();
	}
	return ((gem_object_function*)Func)->FuncPtr(st, ((gem_object_function*)Func)->internals, arguments);
}
gem_object *gem_add(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type == gem_number && y->object_type == gem_number) {
    		gem_object *result;
        result = make_number(
            ((gem_object_number *)x)->value + ((gem_object_number *)y)->value);
 				gem_check_free(x);
    		gem_check_free(y);
				return result;
    } else if (x->object_type == gem_string && y->object_type == gem_string) {
        char *string = malloc(sizeof(char) * ((gem_object_string *)x)->size +
        ((gem_object_string *)y)->size - 1);
        memcpy(string,
            ((gem_object_string *)x)->value,
            ((gem_object_string *)x)->size - 1);
            memcpy(string + ((gem_object_string *)x)->size - 1,
            ((gem_object_string *)x)->value,
            ((gem_object_string *)y)->size);
            gem_object *temporary = make_string(string);
            free(string);
            
            gem_check_free(x);
        gem_check_free(y);

        return temporary;
    } else {
				printf("%s:%" PRIu64 ": attemped to add '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);
        exit(1);
    }

   	return make_nil();
}

// Number Operations

gem_object *number_sub(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to subtract '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);
				exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value - ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_mul(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to multiply '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);
				exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value * ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_div(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to divide '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);
				exit(1);
    };

    gem_object *result = make_number(
        ((gem_object_number *)x)->value / ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_pow(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to pow '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);
				exit(1);
    };

    gem_object *result = make_number(
        pow(((gem_object_number *)x)->value, ((gem_object_number *)y)->value));
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_mod(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to mod '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);
				exit(1);
    };

    gem_object *result = make_number(modf(
        ((gem_object_number *)x)->value, &((gem_object_number *)y)->value));
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

// String Operations


// Conditionals

gem_object *number_greaterThan(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to mod '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);;
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value > ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_lessThan(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to mod '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);;
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value < ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_greaterThanEqual(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to mod '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);;
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value >= ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *number_lessThanEqual(stack_trace* st, gem_object *x, gem_object *y) {
    if (x->object_type != gem_number || y->object_type != gem_number) {
        printf("%s:%" PRIu64 ": attemped to mod '%s' and '%s' \n", st->FileName, st->Line, get_type_name(x->object_type), get_type_name(y->object_type));
				print_trace(st);;
        exit(1);
    };

    gem_object *result = make_bool(
        ((gem_object_number *)x)->value <= ((gem_object_number *)y)->value);
    gem_check_free(x);
    gem_check_free(y);

    return result;
}

gem_object *gem_equal(stack_trace* st, gem_object *x, gem_object *y) {
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

gem_object *gem_notEqual(stack_trace* st, gem_object *x, gem_object *y) {
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

gem_object *gem_not_operator(stack_trace* st, gem_object *value) {
    switch (value->object_type) {
    case gem_number:
        return make_bool(!((gem_object_number *)value)->value);
    case gem_bool:
        return make_bool(!((gem_object_bool *)value)->value);
    case gem_string:
        return make_bool(!((gem_object_string *)value)->value);
    default:
				printf("%s:%" PRIu64 ": attemped to use not on '%s' \n", st->FileName, st->Line, get_type_name(value->object_type));
				print_trace(st);
        exit(1);
    }
}

// Utility

bool isTruthy(gem_object* bool_object) {
    return ((gem_object_bool*)bool_object)->value;
}
