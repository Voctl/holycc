#ifndef HOLYC_DIAG_H
#define HOLYC_DIAG_H

#include "holyc/token.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Severity levels for diagnostics
typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_ICE, // Internal Compiler Error
} DiagnosticLevel;

// A single diagnostic message
typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
    uint32_t length;
    DiagnosticLevel level;
    const char *message;
} Diagnostic;

// Opaque list of diagnostics
typedef struct DiagnosticList DiagnosticList;

// Function-pointer interface for emitting diagnostics
typedef struct {
    void (*error)(SourceLocation loc, const char *fmt, ...);
    void (*error_at)(const char *filename, uint32_t line, uint32_t col, const char *fmt, ...);
    void (*warning)(SourceLocation loc, const char *fmt, ...);
    void (*note)(SourceLocation loc, const char *fmt, ...);
    void (*ice)(const char *fmt, ...);
    bool had_error;         // true once any error has been emitted
    DiagnosticList *list;   // stored diagnostics
} Diagnostics;

// Create / destroy / query / print diagnostics
Diagnostics diagnostics_create(void);
void diagnostics_destroy(Diagnostics *diag);
size_t diagnostics_count(const Diagnostics *diag);
const Diagnostic *diagnostics_get(const Diagnostics *diag, size_t index);
void diagnostics_print(const Diagnostics *diag, const char *source);
void diagnostics_clear(Diagnostics *diag);

#endif
