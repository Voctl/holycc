#ifndef HOLYC_UTILS_H
#define HOLYC_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} StringBuffer;

StringBuffer string_buffer_create(void);
void string_buffer_destroy(StringBuffer *sb);
void string_buffer_reserve(StringBuffer *sb, size_t capacity);
void string_buffer_append_char(StringBuffer *sb, char c);
void string_buffer_append_str(StringBuffer *sb, const char *str, size_t len);
void string_buffer_append_cstr(StringBuffer *sb, const char *str);
void string_buffer_append_fmt(StringBuffer *sb, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
void string_buffer_indent(StringBuffer *sb, int level);
char *string_buffer_release(StringBuffer *sb);

char *read_file(const char *path, size_t *out_len);
bool write_file(const char *path, const char *data, size_t len);

#endif
