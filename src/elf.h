#ifndef NYAN_ELF_H
#define NYAN_ELF_H

#include "ast.h"
#include <stdbool.h>
#include <stdint.h>

bool emit_elf(const char *path, const Program *program, uint8_t exit_status);

#endif
