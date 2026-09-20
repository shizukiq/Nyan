#ifndef NYAN_PARSER_H
#define NYAN_PARSER_H

#include "ast.h"
#include "lexer.h"
#include <stdbool.h>

bool parse_program(Lexer *lexer, Program *program);

#endif
