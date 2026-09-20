#include "evaluator.h"
#include "diagnostic.h"
#include <limits.h>

static bool multiply(int64_t left, int64_t right, int64_t *result) {
    return !__builtin_mul_overflow(left, right, result);
}

static bool power(int64_t base, int64_t exponent, int64_t *result) {
    int64_t value = 1;
    while (exponent > 0) {
        if ((exponent & 1) != 0 && !multiply(value, base, &value)) return false;
        exponent >>= 1;
        if (exponent != 0 && !multiply(base, base, &base)) return false;
    }
    *result = value;
    return true;
}

static bool report(SrcSpan span, const char *message) {
    diag_error(span, "%s", message);
    return false;
}

bool evaluate_expression(const Expr *expr, int64_t *value) {
    if (expr->kind == EXPR_INT_LITERAL) {
        *value = expr->int_value;
        return true;
    }

    if (expr->kind == EXPR_UNARY) {
        int64_t operand;
        if (!evaluate_expression(expr->unary.operand, &operand)) return false;
        switch (expr->unary.op) {
        case TK_PLUS: *value = operand; return true;
        case TK_MINUS:
            if (operand == INT64_MIN) return report(expr->unary.operator_span, "integer overflow");
            *value = -operand;
            return true;
        case TK_NOT: *value = operand == 0; return true;
        case TK_TILDE: *value = ~operand; return true;
        default: return report(expr->unary.operator_span, "invalid unary operator");
        }
    }

    int64_t left;
    int64_t right;
    int64_t result;
    if (!evaluate_expression(expr->binary.left, &left) ||
        !evaluate_expression(expr->binary.right, &right)) {
        return false;
    }

    switch (expr->binary.op) {
    case TK_STAR_STAR:
        if (right < 0) return report(expr->binary.operator_span, "negative exponent");
        if (!power(left, right, &result)) break;
        *value = result;
        return true;
    case TK_STAR:
        if (!multiply(left, right, &result)) break;
        *value = result;
        return true;
    case TK_SLASH:
        if (right == 0) return report(expr->binary.operator_span, "division by zero");
        if (left == INT64_MIN && right == -1) break;
        *value = left / right;
        return true;
    case TK_PERCENT:
        if (right == 0) return report(expr->binary.operator_span, "modulo by zero");
        if (left == INT64_MIN && right == -1) break;
        *value = left % right;
        return true;
    case TK_PLUS:
        if (__builtin_add_overflow(left, right, &result)) break;
        *value = result;
        return true;
    case TK_MINUS:
        if (__builtin_sub_overflow(left, right, &result)) break;
        *value = result;
        return true;
    case TK_LTLT:
        if (right < 0 || right >= 64) {
            return report(expr->binary.operator_span, "shift count must be between 0 and 63");
        }
        result = left;
        for (int64_t i = 0; i < right; i++) {
            if (__builtin_add_overflow(result, result, &result)) {
                return report(expr->binary.operator_span, "integer overflow");
            }
        }
        *value = result;
        return true;
    case TK_GTGT:
        if (right < 0 || right >= 64) {
            return report(expr->binary.operator_span, "shift count must be between 0 and 63");
        }
        *value = left >> right;
        return true;
    case TK_AMP: *value = left & right; return true;
    case TK_CARET: *value = left ^ right; return true;
    case TK_PIPE: *value = left | right; return true;
    default: return report(expr->binary.operator_span, "invalid binary operator");
    }

    return report(expr->binary.operator_span, "integer overflow");
}
