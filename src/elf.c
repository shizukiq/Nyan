#include "elf.h"
#include <linux/elf.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool emit_elf(const char *path, const Program *program, uint8_t exit_status) {
    const uint64_t image_base = 0x400000;
    const uint64_t code_offset = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
    unsigned char code[] = {
        0xb8, 0x01, 0x00, 0x00, 0x00,
        0xbf, 0x01, 0x00, 0x00, 0x00,
        0x48, 0xbe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xba, 0x00, 0x00, 0x00, 0x00,
        0x0f, 0x05,
        0xbf, exit_status, 0x00, 0x00, 0x00,
        0xb8, 0x3c, 0x00, 0x00, 0x00,
        0x0f, 0x05,
    };
    size_t output_length = 0;
    for (const Stmt *statement = program->first; statement != NULL; statement = statement->next) {
        if (statement->kind != STMT_PRINT) continue;
        if (statement->print.length > UINT32_MAX - output_length) {
            fputs("error: program output exceeds the x86-64 write limit\n", stderr);
            return false;
        }
        output_length += statement->print.length;
    }

    uint64_t data_address = image_base + code_offset + sizeof(code);
    uint32_t write_length = (uint32_t)output_length;
    memcpy(code + 12, &data_address, sizeof(data_address));
    memcpy(code + 21, &write_length, sizeof(write_length));
    const uint64_t file_size = code_offset + sizeof(code) + output_length;

    Elf64_Ehdr header = {0};
    memcpy(header.e_ident, ELFMAG, SELFMAG);
    header.e_ident[EI_CLASS] = ELFCLASS64;
    header.e_ident[EI_DATA] = ELFDATA2LSB;
    header.e_ident[EI_VERSION] = EV_CURRENT;
    header.e_ident[EI_OSABI] = ELFOSABI_NONE;
    header.e_type = ET_EXEC;
    header.e_machine = EM_X86_64;
    header.e_version = EV_CURRENT;
    header.e_entry = image_base + code_offset;
    header.e_phoff = sizeof(header);
    header.e_ehsize = sizeof(header);
    header.e_phentsize = sizeof(Elf64_Phdr);
    header.e_phnum = 1;

    Elf64_Phdr segment = {0};
    segment.p_type = PT_LOAD;
    segment.p_flags = PF_R | PF_X;
    segment.p_offset = 0;
    segment.p_vaddr = image_base;
    segment.p_paddr = image_base;
    segment.p_filesz = file_size;
    segment.p_memsz = file_size;
    segment.p_align = 0x1000;

    FILE *output = fopen(path, "wb");
    if (output == NULL) {
        fprintf(stderr, "error: cannot create '%s': %s\n", path, strerror(errno));
        return false;
    }

    bool written = fwrite(&header, sizeof(header), 1, output) == 1 &&
                   fwrite(&segment, sizeof(segment), 1, output) == 1 &&
                   fwrite(code, sizeof(code), 1, output) == 1;
    for (const Stmt *statement = program->first; written && statement != NULL;
         statement = statement->next) {
        if (statement->kind == STMT_PRINT && statement->print.length != 0) {
            written = fwrite(statement->print.bytes, 1, statement->print.length, output) ==
                      statement->print.length;
        }
    }
    int close_result = fclose(output);
    if (!written || close_result != 0) {
        fprintf(stderr, "error: cannot write '%s': %s\n", path, strerror(errno));
        return false;
    }
    if (chmod(path, 0755) != 0) {
        fprintf(stderr, "error: cannot make '%s' executable: %s\n", path, strerror(errno));
        return false;
    }
    return true;
}
