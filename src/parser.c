#include "parser.h"
#include "diagnostic.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    Lexer *lexer;
    Token current;
    bool failed;
} Parser;

static SrcSpan joined_span(SrcSpan first, SrcSpan last) {
    return (SrcSpan){first.start, last.end};
}

static void advance(Parser *parser) {
    parser->current = lexer_next(parser->lexer);
    if (parser->current.kind == TK_ERROR) parser->failed = true;
}

static bool expect(Parser *parser, TokenKind kind, const char *description) {
    if (parser->current.kind == kind) {
        advance(parser);
        return true;
    }
    if (!parser->failed) {
        diag_error(parser->current.span, "expected %s", description);
        parser->failed = true;
    }
    return false;
}

static Expr *new_expr(Parser *parser, ExprKind kind, SrcSpan span) {
    Expr *expr = calloc(1, sizeof(*expr));
    if (expr == NULL) {
        diag_error(span, "out of memory while parsing expression");
        parser->failed = true;
        return NULL;
    }
    expr->kind = kind;
    expr->span = span;
    return expr;
}

static Expr *parse_expression(Parser *parser, int minimum_precedence);

static Expr *parse_primary(Parser *parser) {
    Token first = parser->current;
    if (first.kind == TK_INT_LITERAL) {
        Expr *expr = new_expr(parser, EXPR_INT_LITERAL, first.span);
        if (expr != NULL) expr->int_value = first.int_value;
        advance(parser);
        return expr;
    }
    if (first.kind == TK_LPAREN) {
        advance(parser);
        Expr *expr = parse_expression(parser, 1);
        Token closing = parser->current;
        if (!expect(parser, TK_RPAREN, "')'")) {
            ast_free(expr);
            return NULL;
        }
        if (expr != NULL) expr->span = joined_span(first.span, closing.span);
        return expr;
    }
    if (!parser->failed) {
        diag_error(first.span, "expected an integer expression");
        parser->failed = true;
    }
    return NULL;
}

static Expr *parse_unary(Parser *parser) {
    Token op = parser->current;
    if (op.kind != TK_PLUS && op.kind != TK_MINUS &&
        op.kind != TK_NOT && op.kind != TK_TILDE) {
        return parse_primary(parser);
    }

    advance(parser);
    Expr *operand = parse_unary(parser);
    if (operand == NULL) return NULL;
    Expr *expr = new_expr(parser, EXPR_UNARY, joined_span(op.span, operand->span));
    if (expr == NULL) {
        ast_free(operand);
        return NULL;
    }
    expr->unary.op = op.kind;
    expr->unary.operator_span = op.span;
    expr->unary.operand = operand;
    return expr;
}

static Expr *parse_expression(Parser *parser, int minimum_precedence) {
    Expr *left = parse_unary(parser);
    if (left == NULL) return NULL;

    for (;;) {
        Token op = parser->current;
        int precedence = token_binary_precedence(op.kind);
        if (precedence < minimum_precedence) return left;

        advance(parser);
        int right_precedence = precedence + (op.kind == TK_STAR_STAR ? 0 : 1);
        Expr *right = parse_expression(parser, right_precedence);
        if (right == NULL) {
            ast_free(left);
            return NULL;
        }
        Expr *combined = new_expr(parser, EXPR_BINARY, joined_span(left->span, right->span));
        if (combined == NULL) {
            ast_free(left);
            ast_free(right);
            return NULL;
        }
        combined->binary.op = op.kind;
        combined->binary.operator_span = op.span;
        combined->binary.left = left;
        combined->binary.right = right;
        left = combined;
    }
}

static bool is_word(Token token, const char *word) {
    size_t length = strlen(word);
    return token.kind == TK_IDENT && token.text_length == length &&
           memcmp(token.text, word, length) == 0;
}

static int escape_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char *decode_string(Parser *parser, Token token, size_t *decoded_length) {
    char *decoded = malloc(token.text_length + 1);
    if (decoded == NULL) {
        diag_error(token.span, "out of memory while decoding string");
        parser->failed = true;
        return NULL;
    }

    size_t input = 0;
    size_t output = 0;
    while (input < token.text_length) {
        char c = token.text[input++];
        if (c != '\\') {
            decoded[output++] = c;
            continue;
        }
        char escape = token.text[input++];
        switch (escape) {
        case 'n': decoded[output++] = '\n'; break;
        case 'r': decoded[output++] = '\r'; break;
        case 't': decoded[output++] = '\t'; break;
        case '0': decoded[output++] = '\0'; break;
        case '\\': decoded[output++] = '\\'; break;
        case '"': decoded[output++] = '"'; break;
        case 'x': {
            if (input + 2 > token.text_length) {
                diag_error(token.span, "expected two hexadecimal digits after '\\x'");
                free(decoded);
                parser->failed = true;
                return NULL;
            }
            int high = escape_digit(token.text[input]);
            int low = escape_digit(token.text[input + 1]);
            if (high < 0 || low < 0) {
                diag_error(token.span, "invalid hexadecimal string escape");
                free(decoded);
                parser->failed = true;
                return NULL;
            }
            decoded[output++] = (char)((high << 4) | low);
            input += 2;
            break;
        }
        default:
            diag_error(token.span, "unsupported string escape '\\%c'", escape);
            free(decoded);
            parser->failed = true;
            return NULL;
        }
    }
    decoded[output] = '\0';
    *decoded_length = output;
    return decoded;
}

static Stmt *new_statement(Parser *parser, StmtKind kind, SrcSpan span) {
    Stmt *statement = calloc(1, sizeof(*statement));
    if (statement == NULL) {
        diag_error(span, "out of memory while parsing statement");
        parser->failed = true;
        return NULL;
    }
    statement->kind = kind;
    statement->span = span;
    return statement;
}

static void append_statement(Program *program, Stmt *statement) {
    if (program->last == NULL) {
        program->first = statement;
    } else {
        program->last->next = statement;
    }
    program->last = statement;
}

static bool parse_print(Parser *parser, Program *program) {
    Token first = parser->current;
    advance(parser);
    if (!expect(parser, TK_DOT, "'.'") || !is_word(parser->current, "print")) {
        if (!parser->failed) {
            diag_error(parser->current.span, "expected 'print'");
            parser->failed = true;
        }
        return false;
    }
    advance(parser);
    if (!expect(parser, TK_LPAREN, "'('") || parser->current.kind != TK_STRING_LITERAL) {
        if (!parser->failed) {
            diag_error(parser->current.span, "expected a string literal");
            parser->failed = true;
        }
        return false;
    }

    Token string = parser->current;
    size_t length;
    char *bytes = decode_string(parser, string, &length);
    if (bytes == NULL) return false;
    advance(parser);
    Token closing = parser->current;
    if (!expect(parser, TK_RPAREN, "')'")) {
        free(bytes);
        return false;
    }
    if (parser->current.kind == TK_SEMI) advance(parser);

    Stmt *statement = new_statement(parser, STMT_PRINT, joined_span(first.span, closing.span));
    if (statement == NULL) {
        free(bytes);
        return false;
    }
    statement->print.bytes = bytes;
    statement->print.length = length;
    append_statement(program, statement);
    return true;
}

bool parse_program(Lexer *lexer, Program *program) {
    Parser parser = {lexer, {0}, false};
    *program = (Program){0};
    advance(&parser);

    bool io_linked = false;
    if (parser.current.kind == TK_LINK) {
        advance(&parser);
        if (!expect(&parser, TK_LT, "'<'") || !is_word(parser.current, "std")) {
            if (!parser.failed) {
                diag_error(parser.current.span, "expected 'std'");
                parser.failed = true;
            }
            return false;
        }
        advance(&parser);
        if (!expect(&parser, TK_DOT, "'.'") || !is_word(parser.current, "io")) {
            if (!parser.failed) {
                diag_error(parser.current.span, "expected 'io'");
                parser.failed = true;
            }
            return false;
        }
        advance(&parser);
        if (!expect(&parser, TK_GT, "'>'")) return false;
        io_linked = true;
        if (parser.current.kind == TK_SEMI) advance(&parser);
    }

    if (!expect(&parser, TK_FN, "'fn'") ||
        !expect(&parser, TK_MAIN, "'main'") ||
        !expect(&parser, TK_LPAREN, "'('") ||
        !expect(&parser, TK_RPAREN, "')'") ||
        !expect(&parser, TK_LBRACE, "'{'")) {
        return false;
    }

    while (parser.current.kind != TK_RBRACE && parser.current.kind != TK_EOF) {
        if (is_word(parser.current, "io")) {
            if (!io_linked) {
                diag_error(parser.current.span, "io.print requires link <std.io>");
                parser.failed = true;
                break;
            }
            if (!parse_print(&parser, program)) break;
            continue;
        }
        if (parser.current.kind == TK_RETURN) {
            Token first = parser.current;
            advance(&parser);
            Expr *value = parse_expression(&parser, 1);
            if (value == NULL) break;
            Stmt *statement = new_statement(&parser, STMT_RETURN,
                                            joined_span(first.span, value->span));
            if (statement == NULL) {
                ast_free(value);
                break;
            }
            statement->return_value = value;
            append_statement(program, statement);
            if (parser.current.kind == TK_SEMI) advance(&parser);
            if (parser.current.kind != TK_RBRACE) {
                diag_error(parser.current.span, "return must be the final statement");
                parser.failed = true;
            }
            break;
        }
        diag_error(parser.current.span, "expected io.print or return statement");
        parser.failed = true;
        break;
    }

    if (!expect(&parser, TK_RBRACE, "'}'") || !expect(&parser, TK_EOF, "end of file")) {
        ast_free_program(program);
        return false;
    }
    if (parser.failed || lexer->failed) {
        ast_free_program(program);
        return false;
    }
    return true;
}
