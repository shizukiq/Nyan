#ifndef NYAN_DIAGNOSTIC_H
#define NYAN_DIAGNOSTIC_H

#include <stddef.h>

typedef struct {
    const char *path;
    size_t line;
    size_t column;
} SrcLoc;

typedef struct {
    SrcLoc start;
    SrcLoc end;
} SrcSpan;

void diag_error(SrcSpan span, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#endif
