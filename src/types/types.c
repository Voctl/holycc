#include "holyc/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Type *type_create(TypeKind kind) {
    Type *t = calloc(1, sizeof(Type));
    t->kind = kind;
    t->size = 0;
    t->alignment = 0;
    t->is_const = false;
    return t;
}

void type_destroy(Type *type) {
    if (!type) return;
    free(type->name);

    switch (type->kind) {
        case TYPE_POINTER:
            type_destroy(type->base);
            break;
        case TYPE_ARRAY:
            type_destroy(type->array.base);
            break;
        case TYPE_FUNCTION:
            type_destroy(type->function.return_type);
            {
                FuncParam *p = type->function.params;
                while (p) {
                    FuncParam *next = p->next;
                    free((void *)p->name);
                    type_destroy(p->type);
                    free(p);
                    p = next;
                }
            }
            break;
        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_ENUM:
            {
                StructField *f = type->aggregate.fields;
                while (f) {
                    StructField *next = f->next;
                    free((void *)f->name);
                    type_destroy(f->type);
                    free(f);
                    f = next;
                }
            }
            break;
        default:
            break;
    }
    free(type);
}

static void type_setup_builtin(Type *t, uint32_t size, uint32_t align) {
    t->size = size;
    t->alignment = align;
}

Type *type_void(void) {
    Type *t = type_create(TYPE_VOID);
    type_setup_builtin(t, 0, 1);
    return t;
}

Type *type_bool(void) {
    Type *t = type_create(TYPE_BOOL);
    type_setup_builtin(t, 1, 1);
    return t;
}

Type *type_char(void) {
    Type *t = type_create(TYPE_CHAR);
    type_setup_builtin(t, 1, 1);
    return t;
}

Type *type_i8(void) {
    Type *t = type_create(TYPE_I8);
    type_setup_builtin(t, 1, 1);
    return t;
}

Type *type_i16(void) {
    Type *t = type_create(TYPE_I16);
    type_setup_builtin(t, 2, 2);
    return t;
}

Type *type_i32(void) {
    Type *t = type_create(TYPE_I32);
    type_setup_builtin(t, 4, 4);
    return t;
}

Type *type_i64(void) {
    Type *t = type_create(TYPE_I64);
    type_setup_builtin(t, 8, 8);
    return t;
}

Type *type_u8(void) {
    Type *t = type_create(TYPE_U8);
    type_setup_builtin(t, 1, 1);
    return t;
}

Type *type_u16(void) {
    Type *t = type_create(TYPE_U16);
    type_setup_builtin(t, 2, 2);
    return t;
}

Type *type_u32(void) {
    Type *t = type_create(TYPE_U32);
    type_setup_builtin(t, 4, 4);
    return t;
}

Type *type_u64(void) {
    Type *t = type_create(TYPE_U64);
    type_setup_builtin(t, 8, 8);
    return t;
}

Type *type_f64(void) {
    Type *t = type_create(TYPE_F64);
    type_setup_builtin(t, 8, 8);
    return t;
}

Type *type_pointer(Type *base) {
    Type *t = type_create(TYPE_POINTER);
    t->base = base;
    t->size = 8;
    t->alignment = 8;
    return t;
}

Type *type_array(Type *base, uint64_t length) {
    Type *t = type_create(TYPE_ARRAY);
    t->array.base = base;
    t->array.length = length;
    t->size = base->size * (uint32_t)length;
    t->alignment = base->alignment;
    return t;
}

Type *type_function(Type *return_type, FuncParam *params, bool variadic) {
    Type *t = type_create(TYPE_FUNCTION);
    t->function.return_type = return_type;
    t->function.params = params;
    t->function.variadic = variadic;
    t->size = 0;
    t->alignment = 1;
    return t;
}

const char *type_kind_name(TypeKind kind) {
    switch (kind) {
        case TYPE_VOID:        return "void";
        case TYPE_BOOL:        return "Bool";
        case TYPE_CHAR:        return "Char";
        case TYPE_I8:          return "I8";
        case TYPE_I16:         return "I16";
        case TYPE_I32:         return "I32";
        case TYPE_I64:         return "I64";
        case TYPE_U8:          return "U8";
        case TYPE_U16:         return "U16";
        case TYPE_U32:         return "U32";
        case TYPE_U64:         return "U64";
        case TYPE_F64:         return "F64";
        case TYPE_POINTER:     return "pointer";
        case TYPE_ARRAY:       return "array";
        case TYPE_STRUCT:      return "struct";
        case TYPE_UNION:       return "union";
        case TYPE_ENUM:        return "enum";
        case TYPE_FUNCTION:    return "function";
        case TYPE_ERROR:       return "<error>";
        case TYPE_UNRESOLVED:  return "<unresolved>";
    }
    return "unknown";
}

const char *type_c_name(const Type *type) {
    switch (type->kind) {
        case TYPE_VOID:     return "void";
        case TYPE_BOOL:     return "bool";
        case TYPE_CHAR:     return "char";
        case TYPE_I8:       return "int8_t";
        case TYPE_I16:      return "int16_t";
        case TYPE_I32:      return "int32_t";
        case TYPE_I64:      return "int64_t";
        case TYPE_U8:       return "uint8_t";
        case TYPE_U16:      return "uint16_t";
        case TYPE_U32:      return "uint32_t";
        case TYPE_U64:      return "uint64_t";
        case TYPE_F64:      return "double";
        case TYPE_POINTER: {
            const char *base_name = type_c_name(type->base);
            static char buf[256];
            snprintf(buf, sizeof(buf), "%s*", base_name);
            return buf;
        }
        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_ENUM:
            return type->name ? type->name : "<anonymous>";
        default:
            return "<unknown>";
    }
}

uint32_t type_size(const Type *type) {
    return type->size;
}

uint32_t type_alignment(const Type *type) {
    return type->alignment;
}

bool type_equals(const Type *a, const Type *b) {
    if (!a || !b) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;

    switch (a->kind) {
        case TYPE_POINTER:
            return type_equals(a->base, b->base);
        case TYPE_ARRAY:
            return a->array.length == b->array.length &&
                   type_equals(a->array.base, b->array.base);
        case TYPE_FUNCTION:
            return type_equals(a->function.return_type, b->function.return_type);
        case TYPE_STRUCT:
        case TYPE_UNION:
        case TYPE_ENUM:
            if (a->name && b->name) {
                return strcmp(a->name, b->name) == 0;
            }
            return a == b;
        default:
            return true;
    }
}

bool type_is_integer(const Type *type) {
    switch (type->kind) {
        case TYPE_CHAR:
        case TYPE_I8:  case TYPE_I16: case TYPE_I32: case TYPE_I64:
        case TYPE_U8:  case TYPE_U16: case TYPE_U32: case TYPE_U64:
            return true;
        default:
            return false;
    }
}

bool type_is_floating(const Type *type) {
    return type->kind == TYPE_F64;
}

bool type_is_numeric(const Type *type) {
    return type_is_integer(type) || type_is_floating(type);
}

bool type_is_scalar(const Type *type) {
    return type_is_numeric(type) || type->kind == TYPE_POINTER || type->kind == TYPE_BOOL;
}

bool type_is_aggregate(const Type *type) {
    return type->kind == TYPE_STRUCT || type->kind == TYPE_UNION ||
           type->kind == TYPE_ARRAY;
}
