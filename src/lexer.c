#include "lexer.h"
#include "diagnostic.h"
#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static char lexer_current(const Lexer *lexer) {
    return lexer->pos < lexer->len ? lexer->src[lexer->pos] : 0;
}

static char lexer_advance(Lexer *lexer) {
    char c = lexer_current(lexer);
    if (c != 0) {
        lexer->pos++;
        if (c == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
    }
    return c;
}

static void lexer_skip_whitespace(Lexer *lexer) {
    while (lexer->pos < lexer->len) {
        char c = lexer_current(lexer);
        if (isspace((unsigned char)c)) {
            lexer_advance(lexer);
        } else if (c == '#') {
            while (lexer->pos < lexer->len && lexer_current(lexer) != '\n') {
                lexer_advance(lexer);
            }
        } else {
            break;
        }
    }
}

static SrcSpan lexer_span(const Lexer *lexer, size_t line, size_t column, size_t length) {
    SrcLoc start = {lexer->path, line, column};
    SrcLoc end = {lexer->path, line, column + (length == 0 ? 0 : length - 1)};
    return (SrcSpan){start, end};
}

static Token make_token(TokenKind kind, SrcSpan span) {
    return (Token){.kind = kind, .span = span};
}

static bool is_ident_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static TokenKind keyword_kind(const char *text, size_t len) {
    switch (len) {
    case 2:
        if (memcmp(text, "fn", 2) == 0) return TK_FN;
        break;
    case 4:
        if (memcmp(text, "link", 4) == 0) return TK_LINK;
        if (memcmp(text, "main", 4) == 0) return TK_MAIN;
        break;
    case 6:
        if (memcmp(text, "return", 6) == 0) return TK_RETURN;
        break;
    }
    return TK_IDENT;
}

static Token lex_ident_or_keyword(Lexer *lexer) {
    size_t start_pos = lexer->pos;
    size_t start_col = lexer->column;
    size_t start_line = lexer->line;
    while (lexer->pos < lexer->len && is_ident_char(lexer_current(lexer))) {
        lexer_advance(lexer);
    }
    size_t len = lexer->pos - start_pos;
    const char *text = lexer->src + start_pos;
    Token result = make_token(keyword_kind(text, len),
                              lexer_span(lexer, start_line, start_col, len));
    result.text = text;
    result.text_length = len;
    return result;
}

static Token lex_string(Lexer *lexer) {
    size_t start_pos = lexer->pos;
    size_t start_col = lexer->column;
    size_t start_line = lexer->line;
    lexer_advance(lexer);
    size_t content_pos = lexer->pos;

    while (lexer_current(lexer) != '"') {
        char c = lexer_current(lexer);
        if (c == '\0' || c == '\n') {
            SrcSpan span = lexer_span(lexer, start_line, start_col, lexer->pos - start_pos);
            diag_error(span, "unterminated string literal");
            lexer->failed = true;
            return make_token(TK_ERROR, span);
        }
        lexer_advance(lexer);
        if (c == '\\') {
            c = lexer_current(lexer);
            if (c == '\0' || c == '\n') {
                SrcSpan span = lexer_span(lexer, start_line, start_col, lexer->pos - start_pos);
                diag_error(span, "unterminated string escape");
                lexer->failed = true;
                return make_token(TK_ERROR, span);
            }
            lexer_advance(lexer);
        }
    }

    size_t content_length = lexer->pos - content_pos;
    lexer_advance(lexer);
    Token result = make_token(TK_STRING_LITERAL,
                              lexer_span(lexer, start_line, start_col,
                                         lexer->pos - start_pos));
    result.text = lexer->src + content_pos;
    result.text_length = content_length;
    return result;
}

static int hex_digit_value(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
    if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
    return -1;
}

static Token lex_number(Lexer *lexer) {
    size_t start_pos = lexer->pos;
    size_t start_col = lexer->column;
    size_t start_line = lexer->line;
    int base = 10;

    if (lexer_current(lexer) == '0' && lexer->pos + 1 < lexer->len) {
        char prefix = lexer->src[lexer->pos + 1];
        if (prefix == 'x' || prefix == 'X') base = 16;
        if (prefix == 'b' || prefix == 'B') base = 2;
        if (prefix == 'o' || prefix == 'O') base = 8;
        if (base != 10) {
            lexer_advance(lexer);
            lexer_advance(lexer);
        }
    }

    size_t digits = 0;
    uint64_t value = 0;
    while (lexer->pos < lexer->len) {
        int digit = hex_digit_value(lexer_current(lexer));
        if (digit < 0 || digit >= base) break;
        if (value > ((uint64_t)INT64_MAX - (uint64_t)digit) / (uint64_t)base) {
            while (isalnum((unsigned char)lexer_current(lexer))) lexer_advance(lexer);
            SrcSpan span = lexer_span(lexer, start_line, start_col, lexer->pos - start_pos);
            diag_error(span, "integer literal overflow");
            lexer->failed = true;
            return make_token(TK_ERROR, span);
        }
        value = value * (uint64_t)base + (uint64_t)digit;
        digits++;
        lexer_advance(lexer);
    }

    if (digits == 0) {
        SrcSpan span = lexer_span(lexer, start_line, start_col, lexer->pos - start_pos);
        diag_error(span, "expected digits after integer prefix");
        lexer->failed = true;
        return make_token(TK_ERROR, span);
    }
    if (isalnum((unsigned char)lexer_current(lexer)) || lexer_current(lexer) == '_') {
        while (isalnum((unsigned char)lexer_current(lexer)) || lexer_current(lexer) == '_') {
            lexer_advance(lexer);
        }
        SrcSpan span = lexer_span(lexer, start_line, start_col, lexer->pos - start_pos);
        diag_error(span, "invalid digit in integer literal");
        lexer->failed = true;
        return make_token(TK_ERROR, span);
    }

    size_t len = lexer->pos - start_pos;
    Token tok = make_token(TK_INT_LITERAL, lexer_span(lexer, start_line, start_col, len));
    tok.int_value = (int64_t)value;
    return tok;
}

static Token lex_operator(Lexer *lexer) {
    size_t start_pos = lexer->pos;
    size_t start_col = lexer->column;
    size_t start_line = lexer->line;
    
    char c = lexer_advance(lexer);
    TokenKind kind;
    
    switch (c) {
    case '+': kind = TK_PLUS; break;
    case '-': kind = TK_MINUS; break;
    case '*':
        if (lexer->pos < lexer->len && lexer_current(lexer) == '*') {
            lexer_advance(lexer);
            kind = TK_STAR_STAR;
        } else {
            kind = TK_STAR;
        }
        break;
    case '/':
        kind = TK_SLASH;
        break;
    case '%':
        kind = TK_PERCENT;
        break;
    case '!':
        kind = TK_NOT;
        break;
    case '~':
        kind = TK_TILDE;
        break;
    case '<':
        if (lexer->pos < lexer->len && lexer_current(lexer) == '<') {
            lexer_advance(lexer);
            kind = TK_LTLT;
        } else {
            kind = TK_LT;
        }
        break;
    case '>':
        if (lexer->pos < lexer->len && lexer_current(lexer) == '>') {
            lexer_advance(lexer);
            kind = TK_GTGT;
        } else {
            kind = TK_GT;
        }
        break;
    case '&':
        kind = TK_AMP;
        break;
    case '^':
        kind = TK_CARET;
        break;
    case '|':
        kind = TK_PIPE;
        break;
    case '(': kind = TK_LPAREN; break;
    case ')': kind = TK_RPAREN; break;
    case '{': kind = TK_LBRACE; break;
    case '}': kind = TK_RBRACE; break;
    case ';': kind = TK_SEMI; break;
    case '.': kind = TK_DOT; break;
    default:
        {
            SrcSpan span = lexer_span(lexer, start_line, start_col, 1);
            diag_error(span, "unexpected character 0x%02x", (unsigned char)c);
            lexer->failed = true;
            return make_token(TK_ERROR, span);
        }
    }
    
    size_t len = lexer->pos - start_pos;
    SrcSpan span = lexer_span(lexer, start_line, start_col, len);
    return make_token(kind, span);
}

Token lexer_next(Lexer *lexer) {
    lexer_skip_whitespace(lexer);
    
    if (lexer->pos >= lexer->len) {
        return make_token(TK_EOF, lexer_span(lexer, lexer->line, lexer->column, 0));
    }
    
    char c = lexer_current(lexer);
    
    if (isalpha((unsigned char)c) || c == '_') {
        return lex_ident_or_keyword(lexer);
    }
    
    if (isdigit((unsigned char)c)) {
        return lex_number(lexer);
    }

    if (c == '"') {
        return lex_string(lexer);
    }
    
    return lex_operator(lexer);
}

Lexer lexer_init(const char *path, const char *src, size_t len) {
    return (Lexer){path, src, len, 0, 1, 1, false};
}
