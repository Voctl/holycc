#ifndef HOLYC_DRIVER_H
#define HOLYC_DRIVER_H

#include <stdbool.h>

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

int driver_main(int argc, char **argv);

#endif
