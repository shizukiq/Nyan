#ifndef NYAN_LEXER_H
#define NYAN_LEXER_H

#include "token.h"
#include <stdbool.h>

typedef struct {
    const char *path;
    const char *src;
    size_t len;
    size_t pos;
    size_t line;
    size_t column;
    bool failed;
} Lexer;

Lexer lexer_init(const char *path, const char *src, size_t len);
Token lexer_next(Lexer *lexer);

#endif
