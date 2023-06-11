#pragma once

#include <stdbool.h>

#include "internal.h"

int
ejFindLoadAddr64(const struct ejIntHelpers *helpers, const void *pheader, uint32_t phnum,
                 unsigned int *load_addr);

int
ejFindShdrs64(ejElfInfo *info, const struct ehdrParams *params);

bool
ejFindSymbol64(const ejElfInfo *info, const char *func_name, uint16_t section_index, ejSymbolValue *value);

ejAddr
ejFindGotEntry64(const ejElfInfo *info, uint64_t symbol_index);
