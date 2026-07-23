#ifndef HOLYC_DIAG_H
#define HOLYC_DIAG_H

#include "holyc/token.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    DIAG_ERROR,
    DIAG_WARNING,
    DIAG_NOTE,
    DIAG_ICE,
} DiagnosticLevel;

typedef struct {
    const char *filename;
    uint32_t line;
    uint32_t column;
    uint32_t length;
    DiagnosticLevel level;
    const char *message;
} Diagnostic;

typedef struct DiagnosticList DiagnosticList;

typedef struct {
    void (*error)(SourceLocation loc, const char *fmt, ...);
    void (*error_at)(const char *filename, uint32_t line, uint32_t col, const char *fmt, ...);
    void (*warning)(SourceLocation loc, const char *fmt, ...);
    void (*note)(SourceLocation loc, const char *fmt, ...);
    void (*ice)(const char *fmt, ...);
    bool had_error;
    DiagnosticList *list;
} Diagnostics;

Diagnostics diagnostics_create(void);
void diagnostics_destroy(Diagnostics *diag);
size_t diagnostics_count(const Diagnostics *diag);
const Diagnostic *diagnostics_get(const Diagnostics *diag, size_t index);
void diagnostics_print(const Diagnostics *diag, const char *source);
void diagnostics_clear(Diagnostics *diag);

#endif
