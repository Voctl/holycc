#include "holyc/diag.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DIAGNOSTICS 256

typedef struct DiagnosticList {
    Diagnostic items[MAX_DIAGNOSTICS];
    size_t count;
} DiagnosticList;

static void diag_error(SourceLocation loc, const char *fmt, ...);
static void diag_error_at(const char *filename, uint32_t line, uint32_t col, const char *fmt, ...);
static void diag_warning(SourceLocation loc, const char *fmt, ...);
static void diag_note(SourceLocation loc, const char *fmt, ...);
static void diag_ice(const char *fmt, ...);

static Diagnostics *active_diag = NULL;

Diagnostics diagnostics_create(void) {
    Diagnostics d;
    d.error = diag_error;
    d.error_at = diag_error_at;
    d.warning = diag_warning;
    d.note = diag_note;
    d.ice = diag_ice;
    d.had_error = false;
    d.list = calloc(1, sizeof(DiagnosticList));
    return d;
}

void diagnostics_destroy(Diagnostics *diag) {
    if (diag) {
        free(diag->list);
        diag->list = NULL;
    }
}

size_t diagnostics_count(const Diagnostics *diag) {
    return diag->list ? diag->list->count : 0;
}

const Diagnostic *diagnostics_get(const Diagnostics *diag, size_t index) {
    if (diag->list && index < diag->list->count) {
        return &diag->list->items[index];
    }
    return NULL;
}

void diagnostics_clear(Diagnostics *diag) {
    if (diag->list) {
        diag->list->count = 0;
    }
    diag->had_error = false;
}

static void diag_emit(Diagnostics *diag, DiagnosticLevel level, SourceLocation loc,
                      const char *fmt, va_list args) {
    if (!diag->list || diag->list->count >= MAX_DIAGNOSTICS) {
        return;
    }
    Diagnostic *d = &diag->list->items[diag->list->count++];
    d->filename = loc.filename;
    d->line = loc.line;
    d->column = loc.column;
    d->length = 0;
    d->level = level;

    static char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    d->message = strdup(buf);
}

static void diag_error(SourceLocation loc, const char *fmt, ...) {
    if (!active_diag) return;
    active_diag->had_error = true;
    va_list args;
    va_start(args, fmt);
    diag_emit(active_diag, DIAG_ERROR, loc, fmt, args);
    va_end(args);
}

static void diag_error_at(const char *filename, uint32_t line, uint32_t col, const char *fmt, ...) {
    SourceLocation loc = {filename, line, col};
    if (!active_diag) return;
    active_diag->had_error = true;
    va_list args;
    va_start(args, fmt);
    diag_emit(active_diag, DIAG_ERROR, loc, fmt, args);
    va_end(args);
}

static void diag_warning(SourceLocation loc, const char *fmt, ...) {
    if (!active_diag) return;
    va_list args;
    va_start(args, fmt);
    diag_emit(active_diag, DIAG_WARNING, loc, fmt, args);
    va_end(args);
}

static void diag_note(SourceLocation loc, const char *fmt, ...) {
    if (!active_diag) return;
    va_list args;
    va_start(args, fmt);
    diag_emit(active_diag, DIAG_NOTE, loc, fmt, args);
    va_end(args);
}

static void diag_ice(const char *fmt, ...) {
    fprintf(stderr, "INTERNAL COMPILER ERROR: ");
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    abort();
}

void diagnostics_print(const Diagnostics *diag, const char *source) {
    if (!diag->list) return;

    for (size_t i = 0; i < diag->list->count; i++) {
        const Diagnostic *d = &diag->list->items[i];
        const char *level_str = "error";
        switch (d->level) {
            case DIAG_ERROR:   level_str = "error"; break;
            case DIAG_WARNING: level_str = "warning"; break;
            case DIAG_NOTE:    level_str = "note"; break;
            case DIAG_ICE:     level_str = "ICE"; break;
        }

        fprintf(stderr, "%s:%u:%u: %s: %s\n",
                d->filename ? d->filename : "<unknown>",
                d->line, d->column,
                level_str, d->message ? d->message : "");

        if (source && d->filename && d->line > 0) {
            const char *p = source;
            uint32_t current_line = 1;
            while (current_line < d->line && *p) {
                if (*p == '\n') current_line++;
                p++;
            }
            if (*p) {
                const char *line_start = p;
                const char *line_end = p;
                while (*line_end && *line_end != '\n') line_end++;

                size_t line_len = (size_t)(line_end - line_start);
                fprintf(stderr, " %5u | %.*s\n", d->line, (int)line_len, line_start);
                fprintf(stderr, "      | ");
                for (uint32_t col = 1; col < d->column; col++) {
                    fprintf(stderr, " ");
                }
                fprintf(stderr, "^\n");
            }
        }
    }
}
