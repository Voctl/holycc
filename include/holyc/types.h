#ifndef HOLYC_TYPES_H
#define HOLYC_TYPES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// All types recognised by the type system
typedef enum {
    TYPE_VOID,      // void
    TYPE_U0,        // HolyC U0 (maps to void)
    TYPE_BOOL,      // Bool
    TYPE_CHAR,      // Char
    TYPE_I8,        // int8_t
    TYPE_I16,       // int16_t
    TYPE_I32,       // int32_t
    TYPE_I64,       // int64_t
    TYPE_U8,        // uint8_t
    TYPE_U16,       // uint16_t
    TYPE_U32,       // uint32_t
    TYPE_U64,       // uint64_t
    TYPE_F64,       // double
    TYPE_POINTER,   // T*
    TYPE_ARRAY,     // T[n]
    TYPE_STRUCT,    // struct/class
    TYPE_UNION,     // union
    TYPE_ENUM,      // enum
    TYPE_FUNCTION,  // function signature
    TYPE_ERROR,     // sentinel for ill-typed expressions
    TYPE_UNRESOLVED, // forward reference; name stored in Type.name
} TypeKind;

typedef struct Type Type;
typedef struct StructField StructField;
typedef struct FuncParam FuncParam;

// A field inside a struct/union
struct StructField {
    const char *name;
    Type *type;
    StructField *next;
    uint64_t offset;    // byte offset in the aggregate
    uint32_t bit_width; // (unused currently)
};

struct FuncParam {
    const char *name;
    Type *type;
    FuncParam *next;
};

// Type descriptor – builtin or compound
struct Type {
    TypeKind kind;
    uint32_t size;      // byte size
    uint32_t alignment; // byte alignment
    bool is_const;
    char *name;         // type name (for named aggregates)

    union {
        Type *base;                     // for TYPE_POINTER
        struct {
            Type *base;
            uint64_t length;
        } array;                         // for TYPE_ARRAY
        struct {
            StructField *fields;
            bool complete;               // struct body parsed?
        } aggregate;                     // for TYPE_STRUCT / TYPE_UNION
        struct {
            Type *return_type;
            FuncParam *params;
            bool variadic;
        } function;                      // for TYPE_FUNCTION
    };
};

// Generic type creation / destruction
Type *type_create(TypeKind kind);
void type_destroy(Type *type);

// Builtin type factories
Type *type_void(void);
Type *type_u0(void);
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

// Compound type factories
Type *type_pointer(Type *base);
Type *type_array(Type *base, uint64_t length);
Type *type_function(Type *return_type, FuncParam *params, bool variadic);

// Type introspection
const char *type_kind_name(TypeKind kind);
const char *type_c_name(const Type *type);   // HolyC → C17 name
uint32_t type_size(const Type *type);
uint32_t type_alignment(const Type *type);
bool type_equals(const Type *a, const Type *b);

// Type predicates
bool type_is_integer(const Type *type);
bool type_is_floating(const Type *type);
bool type_is_numeric(const Type *type);
bool type_is_scalar(const Type *type);
bool type_is_aggregate(const Type *type);

#endif
