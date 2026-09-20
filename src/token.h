#ifndef NYAN_TOKEN_H
#define NYAN_TOKEN_H

#include "diagnostic.h"
#include <stdint.h>

typedef enum {
    TK_ERROR,
    TK_EOF,
    TK_INT_LITERAL,
    TK_STRING_LITERAL,
    TK_IDENT,
    TK_FN,
    TK_LINK,
    TK_MAIN,
    TK_RETURN,
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_NOT,
    TK_TILDE,
    TK_STAR_STAR,
    TK_LT,
    TK_LTLT,
    TK_GT,
    TK_GTGT,
    TK_AMP,
    TK_CARET,
    TK_PIPE,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_SEMI,
    TK_DOT,
} TokenKind;

typedef struct {
    TokenKind kind;
    SrcSpan span;
    int64_t int_value;
    const char *text;
    size_t text_length;
} Token;

int token_binary_precedence(TokenKind kind);

#endif
