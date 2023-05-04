#pragma once

#include <stdbool.h>

#include "internal.h"

int
ejFindLoadAddr32(const void *pheader, uint32_t phnum, unsigned int *load_addr);

int
ejFindShdrs32(ejElfInfo *info, const struct ehdrParams *params);

bool
ejFindSymbol32(const struct ejSymbolInfo *symbols, const char *func_name, uint16_t section_index,
               ejAddr *addr, uint64_t *symbol_index);

ejAddr
ejFindGotEntry32(const struct ejRelInfo *rels, uint64_t symbol_index);
