#ifndef NYAN_AST_H
#define NYAN_AST_H

#include "token.h"
#include <stddef.h>
#include <stdint.h>

typedef enum {
    EXPR_INT_LITERAL,
    EXPR_UNARY,
    EXPR_BINARY,
} ExprKind;

typedef struct Expr Expr;

struct Expr {
    ExprKind kind;
    SrcSpan span;
    union {
        int64_t int_value;
        struct {
            TokenKind op;
            SrcSpan operator_span;
            Expr *operand;
        } unary;
        struct {
            TokenKind op;
            SrcSpan operator_span;
            Expr *left;
            Expr *right;
        } binary;
    };
};

typedef enum {
    STMT_PRINT,
    STMT_RETURN,
} StmtKind;

typedef struct Stmt Stmt;

struct Stmt {
    StmtKind kind;
    SrcSpan span;
    Stmt *next;
    union {
        struct {
            char *bytes;
            size_t length;
        } print;
        Expr *return_value;
    };
};

typedef struct {
    Stmt *first;
    Stmt *last;
} Program;

void ast_free(Expr *expr);
void ast_free_program(Program *program);

#endif
