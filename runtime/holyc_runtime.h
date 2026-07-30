#ifndef HOLYC_RUNTIME_H
#define HOLYC_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// I/O
void Print(const char *fmt, ...);   // HolyC Print()
void PrintLn(const char *fmt, ...); // Print with newline

// Memory management
void *MAlloc(uint64_t size);
void Free(void *ptr);

// String utilities
uint64_t StrLen(const char *str);
bool StrCompare(const char *a, const char *b);

// Conversion
int64_t AtoI(const char *str);
double AtoF(const char *str);

// Memory operations
void MemSet(uint8_t *dst, uint8_t value, uint64_t count);
void MemCpy(uint8_t *dst, const uint8_t *src, uint64_t count);
int64_t MemCompare(const uint8_t *a, const uint8_t *b, uint64_t count);

// Other
uint64_t MSize(void *ptr);
void CDelay(uint64_t ms);  // millisecond sleep
int GetCh(void);
void PutChar(char c);
void Exit(int64_t code);
int SPrint(char *buf, uint64_t size, const char *fmt, ...);

// ---------------------------------------------------------------------------
// Math library  (all operate on F64 / double)
// ---------------------------------------------------------------------------

// Trigonometric
double Sin(double x);       // sine (radians)
double Cos(double x);       // cosine (radians)
double Tan(double x);       // tangent (radians)

// Inverse trigonometric
double ASin(double x);      // arc-sine   -> [-π/2, π/2]
double ACos(double x);      // arc-cosine -> [0, π]
double ATan(double x);      // arc-tangent -> (-π/2, π/2)
double ATan2(double y, double x); // two-argument arc-tangent

// Hyperbolic
double SinH(double x);
double CosH(double x);
double TanH(double x);

// Exponential / power / logarithm
double Exp(double x);       // e^x
double Pow(double base, double exp); // base^exp  (same as backtick operator)
double Sqrt(double x);      // square root
double Cbrt(double x);      // cube root
double Log(double x);       // natural logarithm
double Log2(double x);      // base-2 logarithm
double Log10(double x);     // base-10 logarithm

// Rounding
double Floor(double x);     // round toward -inf
double Ceil(double x);      // round toward +inf
double Round(double x);     // round to nearest (ties away from zero)
double Trunc(double x);     // round toward zero

// Absolute value
double FAbs(double x);      // |x| for F64
int64_t Abs(int64_t x);     // |x| for I64

// Min / Max
double FMin(double a, double b);
double FMax(double a, double b);
int64_t Min(int64_t a, int64_t b);
int64_t Max(int64_t a, int64_t b);

// Misc
double Hypot(double a, double b);   // sqrt(a²+b²) without overflow
double FMod(double x, double y);    // floating-point remainder

// Constants (as inline double values)
#define HC_PI   3.14159265358979323846
#define HC_E    2.71828182845904523536
#define HC_TAU  6.28318530717958647692
#define HC_SQRT2 1.41421356237309504880

#endif
