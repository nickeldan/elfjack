#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define ELFJACK_VERSION "0.1.1"

#ifdef __GNUC__
#define EJ_PURE __attribute__((pure))
#else
#define EJ_PURE
#endif

enum ejRetValue {
    EJ_RET_OK = 0,
    EJ_RET_BAD_USAGE,
    EJ_RET_READ_FAILURE,
    EJ_RET_OUT_OF_MEMORY,
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

struct ejTextInfo {
    uint64_t *section_idxs;
    size_t num_sections;
    size_t capacity;
};

typedef struct ejElfInfo {
    bool (*find_symbol)(const struct ejSymbolInfo *, const char *, uint64_t *, ejAddr *, uint64_t *);
    ejAddr (*find_got_entry)(const struct ejRelInfo *, uint64_t);
    struct ejMapInfo map;
    struct ejSymbolInfo symbols;
    struct ejRelInfo rels;
    struct ejTextInfo text;
    unsigned int load_bias;
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
ejParseElf(const char *path, ejElfInfo *info);

char *
ejGetError(void) EJ_PURE;

void
ejReleaseInfo(ejElfInfo *info);

ejAddr
ejFindGotEntry(const ejElfInfo *info, const char *func_name) EJ_PURE;

ejAddr
ejFindFunction(const ejElfInfo *info, const char *func_name) EJ_PURE;

ejAddr
ejResolveAddress(const ejElfInfo *info, ejAddr addr, ejAddr file_start) EJ_PURE;
