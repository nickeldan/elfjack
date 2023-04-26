#pragma once

#include <stdbool.h>

#include "internal.h"

int
findLoadAddr32(const void *pheader, uint16_t phnum, unsigned int *load_addr) EJ_HIDDEN;

int
findShdrs32(ejElfInfo *info, const struct ehdrParams *params) EJ_HIDDEN;

bool
findSymbol32(const struct ejSymbolInfo *symbols, const char *func_name, uint16_t section_index, ejAddr *addr,
             uint64_t *symbol_index) EJ_HIDDEN;

ejAddr
findGotEntry32(const struct ejRelInfo *rels, uint64_t symbol_index) EJ_HIDDEN;
