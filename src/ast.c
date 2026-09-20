#include "ast.h"
#include <stdlib.h>

void ast_free(Expr *expr) {
    if (expr == NULL) return;
    if (expr->kind == EXPR_UNARY) {
        ast_free(expr->unary.operand);
    } else if (expr->kind == EXPR_BINARY) {
        ast_free(expr->binary.left);
        ast_free(expr->binary.right);
    }
    free(expr);
}

void ast_free_program(Program *program) {
    Stmt *statement = program->first;
    while (statement != NULL) {
        Stmt *next = statement->next;
        if (statement->kind == STMT_PRINT) {
            free(statement->print.bytes);
        } else {
            ast_free(statement->return_value);
        }
        free(statement);
        statement = next;
    }
    program->first = NULL;
    program->last = NULL;
}
