#include "holyc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void Print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void PrintLn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

void *MAlloc(uint64_t size) {
    return malloc((size_t)size);
}

void Free(void *ptr) {
    free(ptr);
}

uint64_t StrLen(const char *str) {
    uint64_t len = 0;
    while (*str++) len++;
    return len;
}

bool StrCompare(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    return *a == *b;
}

int64_t AtoI(const char *str) {
    return (int64_t)strtoll(str, NULL, 10);
}

double AtoF(const char *str) {
    return strtod(str, NULL);
}

void MemSet(uint8_t *dst, uint8_t value, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        dst[i] = value;
    }
}

void MemCpy(uint8_t *dst, const uint8_t *src, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

int64_t MemCompare(const uint8_t *a, const uint8_t *b, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        if (a[i] != b[i]) return (int64_t)a[i] - (int64_t)b[i];
    }
    return 0;
}
