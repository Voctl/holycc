#ifndef HOLYC_DRIVER_H
#define HOLYC_DRIVER_H

#include <stdbool.h>

// CLI options parsed from argv
typedef struct {
    const char *input_file;
    const char *output_file;
    bool compile_only;
    bool compile_run;
    bool dump_tokens;
    bool dump_ast;
    bool show_help;
    bool show_version;
    bool keep_c;
} DriverOptions;

// Entry point for the CLI – orchestrates the entire compilation pipeline
int driver_main(int argc, char **argv);

#endif
