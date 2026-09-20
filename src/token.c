#include "token.h"

int token_binary_precedence(TokenKind kind) {
    switch (kind) {
    case TK_STAR_STAR: return 7;
    case TK_STAR:
    case TK_SLASH:
    case TK_PERCENT: return 6;
    case TK_PLUS:
    case TK_MINUS: return 5;
    case TK_LTLT:
    case TK_GTGT: return 4;
    case TK_AMP: return 3;
    case TK_CARET: return 2;
    case TK_PIPE: return 1;
    default: return 0;
    }
}
