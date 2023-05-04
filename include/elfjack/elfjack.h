#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define ELFJACK_VERSION "0.1.3"

#if defined(__GNUC__) && !defined(EJ_NO_EXPORT)
#define EJ_EXPORT __attribute__((visibility("default")))
#else
#define EJ_EXPORT
#endif

#ifdef __GNUC__
#define EJ_PURE __attribute__((pure))
#else
#define EJ_PURE
#endif

enum ejRetValue {
    EJ_RET_OK = 0,
    EJ_RET_BAD_USAGE,
    EJ_RET_READ_FAILURE,
    EJ_RET_MAP_FAIL,
    EJ_RET_NOT_ELF,
    EJ_RET_MISSING_INFO,
    EJ_RET_MALFORMED_ELF,
};

typedef unsigned long long ejAddr;
#define EJ_ADDR_NOT_FOUND ((ejAddr)-1)

struct ejMapInfo {
    const void *data;
    size_t size;
    size_t map_size;
};

struct ejSymbolInfo {
    const void *start;
    const char *strings;
    uint64_t count;
    size_t strings_size;
};

struct ejRelInfo {
    const void *start;
    uint64_t count;
    unsigned int object_size;
    unsigned int info_offset;
};

typedef struct ejElfInfo {
    bool (*find_symbol)(const struct ejSymbolInfo *, const char *, uint16_t, ejAddr *, uint64_t *);
    ejAddr (*find_got_entry)(const struct ejRelInfo *, uint64_t);
    struct ejMapInfo map;
    struct ejSymbolInfo symbols;
    struct ejRelInfo rels;
    unsigned int load_bias;
    uint16_t text_section_index;
    unsigned int dynamic : 1;
    struct {
        uint16_t machine;
        unsigned char pointer_size;
        unsigned int little_endian : 1;
    } visible;
} ejElfInfo;

#define EJ_ELF_INFO_INIT \
    (ejElfInfo)          \
    {                    \
        0                \
    }

int
ejParseElf(const char *path, ejElfInfo *info) EJ_EXPORT;

char *
ejGetError(void) EJ_PURE EJ_EXPORT;

void
ejReleaseInfo(ejElfInfo *info) EJ_EXPORT;

ejAddr
ejFindGotEntry(const ejElfInfo *info, const char *func_name) EJ_EXPORT EJ_PURE;

ejAddr
ejFindFunction(const ejElfInfo *info, const char *func_name) EJ_EXPORT EJ_PURE;

ejAddr
ejResolveAddress(const ejElfInfo *info, ejAddr addr, ejAddr file_start) EJ_EXPORT EJ_PURE;
