#include "holyc_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

// HolyC Print() – format string to stdout
void Print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

// HolyC PrintLn() – Print with trailing newline
void PrintLn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

// Allocate memory (HolyC MAlloc → malloc)
void *MAlloc(uint64_t size) {
    return malloc((size_t)size);
}

// Free memory (HolyC Free → free)
void Free(void *ptr) {
    free(ptr);
}

// HolyC StrLen – custom strlen implementation
uint64_t StrLen(const char *str) {
    uint64_t len = 0;
    while (*str++) len++;
    return len;
}

// HolyC StrCompare – equality check (not strcmp!)
bool StrCompare(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return false;
        a++;
        b++;
    }
    return *a == *b;
}

// HolyC AtoI – string to int64_t
int64_t AtoI(const char *str) {
    return (int64_t)strtoll(str, NULL, 10);
}

// HolyC AtoF – string to double
double AtoF(const char *str) {
    return strtod(str, NULL);
}

// HolyC MemSet – byte-by-byte fill (no <string.h> reliance for generated code)
void MemSet(uint8_t *dst, uint8_t value, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        dst[i] = value;
    }
}

// HolyC MemCpy – byte-by-byte copy
void MemCpy(uint8_t *dst, const uint8_t *src, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        dst[i] = src[i];
    }
}

// HolyC MemCompare – byte-by-byte comparison
int64_t MemCompare(const uint8_t *a, const uint8_t *b, uint64_t count) {
    for (uint64_t i = 0; i < count; i++) {
        if (a[i] != b[i]) return (int64_t)a[i] - (int64_t)b[i];
    }
    return 0;
}

// HolyC MSize – malloc_usable_size wrapper
uint64_t MSize(void *ptr) {
    return malloc_usable_size(ptr);
}

// HolyC CDelay – millisecond sleep (usleep wrapper)
void CDelay(uint64_t ms) {
    usleep(ms * 1000);
}

// HolyC GetCh – single character input
int GetCh(void) {
    return getchar();
}

// HolyC PutChar – single character output
void PutChar(char c) {
    putchar(c);
}

// HolyC Exit – process exit
void Exit(int64_t code) {
    exit((int)code);
}

// HolyC SPrint – snprintf wrapper (safe bounded version)
int SPrint(char *buf, uint64_t size, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, size, fmt, args);
    va_end(args);
    return ret;
}

// ---------------------------------------------------------------------------
// Math library — thin wrappers around libm
// ---------------------------------------------------------------------------
#include <math.h>

// Trigonometric
double Sin(double x)  { return sin(x);  }
double Cos(double x)  { return cos(x);  }
double Tan(double x)  { return tan(x);  }

// Inverse trigonometric
double ASin(double x)           { return asin(x);       }
double ACos(double x)           { return acos(x);       }
double ATan(double x)           { return atan(x);       }
double ATan2(double y, double x){ return atan2(y, x);   }

// Hyperbolic
double SinH(double x) { return sinh(x); }
double CosH(double x) { return cosh(x); }
double TanH(double x) { return tanh(x); }

// Exponential / power / logarithm
double Exp(double x)              { return exp(x);        }
double Pow(double base, double e) { return pow(base, e);  }
double Sqrt(double x)             { return sqrt(x);       }
double Cbrt(double x)             { return cbrt(x);       }
double Log(double x)              { return log(x);        }
double Log2(double x)             { return log2(x);       }
double Log10(double x)            { return log10(x);      }

// Rounding
double Floor(double x) { return floor(x); }
double Ceil(double x)  { return ceil(x);  }
double Round(double x) { return round(x); }
double Trunc(double x) { return trunc(x); }

// Absolute value
double  FAbs(double x)  { return fabs(x);       }
int64_t Abs(int64_t x)  { return x < 0 ? -x : x; }

// Min / Max  (branchless-friendly for integers)
double  FMin(double  a, double  b) { return a < b ? a : b; }
double  FMax(double  a, double  b) { return a > b ? a : b; }
int64_t Min(int64_t  a, int64_t b) { return a < b ? a : b; }
int64_t Max(int64_t  a, int64_t b) { return a > b ? a : b; }

// Misc
double Hypot(double a, double b)  { return hypot(a, b);  }
double FMod(double x, double y)   { return fmod(x, y);   }

