#include "holyc/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

StringBuffer string_buffer_create(void) {
    StringBuffer sb = {0};
    sb.capacity = 256;
    sb.data = malloc(sb.capacity);
    sb.data[0] = '\0';
    return sb;
}

void string_buffer_destroy(StringBuffer *sb) {
    free(sb->data);
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
}

void string_buffer_reserve(StringBuffer *sb, size_t capacity) {
    if (capacity <= sb->capacity) return;
    size_t new_cap = sb->capacity * 2;
    if (new_cap < capacity) new_cap = capacity;
    char *new_data = realloc(sb->data, new_cap);
    if (!new_data) return;
    sb->data = new_data;
    sb->capacity = new_cap;
}

void string_buffer_append_char(StringBuffer *sb, char c) {
    string_buffer_reserve(sb, sb->length + 2);
    sb->data[sb->length++] = c;
    sb->data[sb->length] = '\0';
}

void string_buffer_append_str(StringBuffer *sb, const char *str, size_t len) {
    string_buffer_reserve(sb, sb->length + len + 1);
    memcpy(sb->data + sb->length, str, len);
    sb->length += len;
    sb->data[sb->length] = '\0';
}

void string_buffer_append_cstr(StringBuffer *sb, const char *str) {
    string_buffer_append_str(sb, str, strlen(str));
}

void string_buffer_append_fmt(StringBuffer *sb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int needed = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (needed <= 0) return;

    string_buffer_reserve(sb, sb->length + (size_t)needed + 1);

    va_start(args, fmt);
    vsnprintf(sb->data + sb->length, (size_t)needed + 1, fmt, args);
    va_end(args);

    sb->length += (size_t)needed;
}

void string_buffer_indent(StringBuffer *sb, int level) {
    for (int i = 0; i < level; i++) {
        string_buffer_append_str(sb, "    ", 4);
    }
}

char *string_buffer_release(StringBuffer *sb) {
    char *result = sb->data;
    sb->data = NULL;
    sb->length = 0;
    sb->capacity = 0;
    return result;
}

char *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }
    fseek(f, 0, SEEK_SET);

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    size_t read = fread(buf, 1, (size_t)size, f);
    fclose(f);

    buf[read] = '\0';
    if (out_len) *out_len = read;
    return buf;
}

bool write_file(const char *path, const char *data, size_t len) {
    FILE *f = fopen(path, "wb");
    if (!f) return false;

    size_t written = fwrite(data, 1, len, f);
    fclose(f);
    return written == len;
}
