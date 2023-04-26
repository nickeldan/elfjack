#pragma once

#include <elf.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <elfjack/elfjack.h>

#ifdef __GNUC__
#define EJ_HIDDEN __attribute__((visibility("internal")))
#else
#define EJ_HIDDEN
#endif

#define AT_OFFSET(ptr, offset) ((void *)((unsigned char *)(ptr) + (offset)))

#define SHDR_SANITY_CHECK(shdr, map_size) ((shdr)->sh_offset + (shdr)->sh_size <= (map_size))

struct ehdrParams {
    uint64_t shnum;
    uint16_t shoff;
    uint16_t shstrndx;
    uint32_t phnum;
    uint16_t phoff;
    unsigned int _64 : 1;
};

void
emitError(const char *format, ...) EJ_HIDDEN;
