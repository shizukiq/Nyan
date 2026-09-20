#include "diagnostic.h"
#include <stdarg.h>
#include <stdio.h>

void diag_error(SrcSpan span, const char *fmt, ...) {
    va_list args;

    fputs("error: ", stderr);
    if (span.start.path != NULL) {
        fprintf(stderr, "%s:%zu:%zu: ", span.start.path, span.start.line, span.start.column);
    }
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}
