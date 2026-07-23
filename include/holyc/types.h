#ifndef HOLYC_TYPES_H
#define HOLYC_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    TYPE_VOID,
    TYPE_BOOL,
    TYPE_CHAR,
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_F64,
    TYPE_POINTER,
    TYPE_ARRAY,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_ENUM,
    TYPE_FUNCTION,
    TYPE_ERROR,
    TYPE_UNRESOLVED,
} TypeKind;

typedef struct Type Type;
typedef struct StructField StructField;
typedef struct FuncParam FuncParam;

struct StructField {
    const char *name;
    Type *type;
    StructField *next;
    uint64_t offset;
    uint32_t bit_width;
};

struct FuncParam {
    const char *name;
    Type *type;
    FuncParam *next;
};

struct Type {
    TypeKind kind;
    uint32_t size;
    uint32_t alignment;
    bool is_const;
    char *name;

    union {
        Type *base;
        struct {
            Type *base;
            uint64_t length;
        } array;
        struct {
            StructField *fields;
            bool complete;
        } aggregate;
        struct {
            Type *return_type;
            FuncParam *params;
            bool variadic;
        } function;
    };
};

Type *type_create(TypeKind kind);
void type_destroy(Type *type);

Type *type_void(void);
Type *type_bool(void);
Type *type_char(void);
Type *type_i8(void);
Type *type_i16(void);
Type *type_i32(void);
Type *type_i64(void);
Type *type_u8(void);
Type *type_u16(void);
Type *type_u32(void);
Type *type_u64(void);
Type *type_f64(void);

Type *type_pointer(Type *base);
Type *type_array(Type *base, uint64_t length);
Type *type_function(Type *return_type, FuncParam *params, bool variadic);

const char *type_kind_name(TypeKind kind);
const char *type_c_name(const Type *type);
uint32_t type_size(const Type *type);
uint32_t type_alignment(const Type *type);
bool type_equals(const Type *a, const Type *b);
bool type_is_integer(const Type *type);
bool type_is_floating(const Type *type);
bool type_is_numeric(const Type *type);
bool type_is_scalar(const Type *type);
bool type_is_aggregate(const Type *type);

#endif
