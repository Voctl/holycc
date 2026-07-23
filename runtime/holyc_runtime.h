#ifndef HOLYC_RUNTIME_H
#define HOLYC_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void Print(const char *fmt, ...);
void PrintLn(const char *fmt, ...);

void *MAlloc(uint64_t size);
void Free(void *ptr);

uint64_t StrLen(const char *str);
bool StrCompare(const char *a, const char *b);

int64_t AtoI(const char *str);
double AtoF(const char *str);

void MemSet(uint8_t *dst, uint8_t value, uint64_t count);
void MemCpy(uint8_t *dst, const uint8_t *src, uint64_t count);
int64_t MemCompare(const uint8_t *a, const uint8_t *b, uint64_t count);

#endif
