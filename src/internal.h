#pragma once

#include <elf.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <elfjack/elfjack.h>

#define AT_OFFSET(ptr, offset) ((void *)((unsigned char *)(ptr) + (offset)))

#define SHDR_SANITY_CHECK(shdr, map_size) ((shdr)->sh_offset + (shdr)->sh_size <= (map_size))

struct ehdrParams {
    uint64_t shoff;
    uint64_t shnum;
    uint16_t shstrndx;
    uint64_t phoff;
    uint32_t phnum;
    unsigned int _64 : 1;
};

void
ejEmitError(const char *format, ...);
