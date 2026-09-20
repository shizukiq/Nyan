#include "ast.h"
#include "diagnostic.h"
#include "elf.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(FILE *stream) {
    fputs("usage: nyanc <input.nyan> [-o <output>]\n", stream);
}

static char *read_source(const char *path, size_t *length) {
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "error: cannot open '%s': %s\n", path, strerror(errno));
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: cannot seek '%s': %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    long size = ftell(file);
    if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "error: cannot read size of '%s': %s\n", path, strerror(errno));
        fclose(file);
        return NULL;
    }
    if ((unsigned long)size > SIZE_MAX - 1) {
        fprintf(stderr, "error: source file is too large\n");
        fclose(file);
        return NULL;
    }

    char *source = malloc((size_t)size + 1);
    if (source == NULL) {
        fprintf(stderr, "error: out of memory while reading '%s'\n", path);
        fclose(file);
        return NULL;
    }
    size_t bytes = fread(source, 1, (size_t)size, file);
    bool read_failed = bytes != (size_t)size;
    int close_result = fclose(file);
    if (read_failed) {
        fprintf(stderr, "error: short read from '%s'\n", path);
        free(source);
        return NULL;
    }
    if (close_result != 0) {
        fprintf(stderr, "error: cannot read '%s': %s\n", path, strerror(errno));
        free(source);
        return NULL;
    }
    source[bytes] = '\0';
    *length = bytes;
    return source;
}

static char *default_output_path(const char *input) {
    size_t length = strlen(input);
    bool has_suffix = length >= 5 && strcmp(input + length - 5, ".nyan") == 0;
    size_t output_length = has_suffix ? length - 5 : length + 4;
    char *output = malloc(output_length + 1);
    if (output == NULL) return NULL;
    if (has_suffix) {
        memcpy(output, input, output_length);
    } else {
        memcpy(output, input, length);
        memcpy(output + length, ".out", 4);
    }
    output[output_length] = '\0';
    return output;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        usage(stdout);
        return 0;
    }
    if (argc != 2 && argc != 4) {
        usage(stderr);
        return 2;
    }

    const char *input_path = argv[1];
    const char *output_path = NULL;
    char *owned_output_path = NULL;
    if (argc == 4) {
        if ((strcmp(argv[2], "-o") != 0 && strcmp(argv[2], "--output") != 0) ||
            argv[3][0] == '\0') {
            usage(stderr);
            return 2;
        }
        output_path = argv[3];
    } else {
        owned_output_path = default_output_path(input_path);
        if (owned_output_path == NULL) {
            fputs("error: out of memory while creating output path\n", stderr);
            return 1;
        }
        output_path = owned_output_path;
    }

    size_t source_length;
    char *source = read_source(input_path, &source_length);
    if (source == NULL) {
        free(owned_output_path);
        return 1;
    }
    if (memchr(source, '\0', source_length) != NULL) {
        fprintf(stderr, "error: '%s' contains a NUL byte\n", input_path);
        free(source);
        free(owned_output_path);
        return 1;
    }

    Lexer lexer = lexer_init(input_path, source, source_length);
    Program program;
    bool parsed = parse_program(&lexer, &program);
    int64_t value = 0;
    bool evaluated = parsed;
    if (parsed) {
        for (Stmt *statement = program.first; statement != NULL; statement = statement->next) {
            if (statement->kind == STMT_RETURN) {
                evaluated = evaluate_expression(statement->return_value, &value);
                break;
            }
        }
    }
    bool emitted = evaluated && emit_elf(output_path, &program, (uint8_t)value);

    ast_free_program(&program);
    free(source);
    free(owned_output_path);
    return emitted ? 0 : 1;
}
