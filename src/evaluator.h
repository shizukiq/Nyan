#ifndef NYAN_EVALUATOR_H
#define NYAN_EVALUATOR_H

#include "ast.h"
#include <stdbool.h>
#include <stdint.h>

bool evaluate_expression(const Expr *expr, int64_t *value);

#endif
