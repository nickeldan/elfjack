#pragma once

#include <stdbool.h>

#include "internal.h"

int
ejFindLoadAddr32(const struct ejIntHelpers *helpers, const void *pheader, uint32_t phnum,
                 unsigned int *load_addr);

int
ejFindShdrs32(ejElfInfo *info, const struct ehdrParams *params);

bool
ejFindSymbol32(const ejElfInfo *info, const char *func_name, uint16_t section_index, ejSymbolValue *value);

ejAddr
ejFindGotEntry32(const ejElfInfo *info, uint64_t symbol_index);
