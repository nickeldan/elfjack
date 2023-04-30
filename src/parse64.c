#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "parse64.h"

static const char *
getShdrStrings(const struct ejMapInfo *map, const Elf64_Shdr *shdr, size_t *size)
{
    const char *strings;

    if (!SHDR_SANITY_CHECK(shdr, map->size)) {
        emitError("File is not big enough to contain the section header string table");
        return NULL;
    }

    strings = AT_OFFSET(map->data, shdr->sh_offset);
    *size = shdr->sh_size;
    if (*size == 0) {
        emitError("Section header string table is empty");
        return NULL;
    }
    if (strings[*size - 1] != '\0') {
        emitError("Section header string table is not null-terminated");
        return NULL;
    }

    return strings;
}

int
findLoadAddr64(const void *pheader, uint32_t phnum, unsigned int *load_addr)
{
    const Elf64_Phdr *phdr = pheader;

    for (uint32_t k = 0; k < phnum; k++) {
        if (phdr[k].p_type == PT_LOAD) {
            *load_addr = phdr[k].p_offset;
            return EJ_RET_OK;
        }
    }

    return EJ_RET_MISSING_INFO;
}

int
findShdrs64(ejElfInfo *info, const struct ehdrParams *params)
{
    size_t strings_size;
    const char *strings;
    const Elf64_Shdr *table = AT_OFFSET(info->map.data, params->shoff);

    strings = getShdrStrings(&info->map, &table[params->shstrndx], &strings_size);
    if (!strings) {
        return EJ_RET_MALFORMED_ELF;
    }

    for (uint64_t k = 1; k < params->shnum; k++) {
        const char *section_name;
        const void *section_start;
        const Elf64_Shdr *shdr = &table[k];

        if (k == params->shstrndx) {
            continue;
        }

        if (!SHDR_SANITY_CHECK(shdr, info->map.size) || shdr->sh_name >= strings_size) {
            emitError("File is not big enough to contain section #%llu", (unsigned long long)k);
            return EJ_RET_MALFORMED_ELF;
        }
        section_start = AT_OFFSET(info->map.data, shdr->sh_offset);
        section_name = strings + shdr->sh_name;

        if (!info->symbols.start && strcmp(section_name, ".dynsym") == 0) {
            info->symbols.start = section_start;
            info->symbols.count = shdr->sh_size / shdr->sh_entsize;
        }
        else if (!info->symbols.strings && strcmp(section_name, ".dynstr") == 0) {
            info->symbols.strings = section_start;
            info->symbols.strings_size = shdr->sh_size;
            if (info->symbols.strings_size == 0) {
                emitError(".dynstr is empty");
                return EJ_RET_MALFORMED_ELF;
            }
            if (info->symbols.strings[info->symbols.strings_size - 1] != '\0') {
                emitError(".dynstr is not null-terminated");
                return EJ_RET_MALFORMED_ELF;
            }
        }
        else if (!info->rels.start &&
                 (strcmp(section_name, ".rela.plt") == 0 || strcmp(section_name, ".rel.plt") == 0)) {
            info->rels.start = section_start;
            info->rels.count = shdr->sh_size / shdr->sh_entsize;
            if (shdr->sh_type == SHT_RELA) {
                info->rels.object_size = sizeof(Elf64_Rela);
                info->rels.info_offset = offsetof(Elf64_Rela, r_info);
            }
            else {
                info->rels.object_size = sizeof(Elf64_Rel);
                info->rels.info_offset = offsetof(Elf64_Rel, r_info);
            }
        }
        else if (shdr->sh_type == SHT_PROGBITS && shdr->sh_flags & SHF_EXECINSTR &&
                 strncmp(section_name, ".text", 5) == 0) {
            if (info->text.num_sections == info->text.capacity) {
                uint64_t *new_idxs;
                size_t new_capacity = (info->text.capacity == 0) ? 1 : (info->text.capacity + 2);

                new_idxs = realloc(info->text.section_idxs, sizeof(uint64_t) * new_capacity);
                if (!new_idxs) {
                    emitError("Failed to allocate %zu bytes", sizeof(uint64_t) * new_capacity);
                    return EJ_RET_OUT_OF_MEMORY;
                }
                info->text.section_idxs = new_idxs;
                info->text.capacity = new_capacity;
            }

            info->text.section_idxs[info->text.num_sections++] = k;
        }
    }

    return EJ_RET_OK;
}

bool
findSymbol64(const struct ejSymbolInfo *symbols, const char *func_name, uint64_t *section_index, ejAddr *addr,
             uint64_t *symbol_index)
{
    const Elf64_Sym *syms = symbols->start;

    for (uint64_t k = 0; k < symbols->count; k++) {
        const Elf64_Sym *sym = &syms[k];

        if (ELF64_ST_TYPE(sym->st_info) != STT_FUNC) {
            continue;
        }

        if (sym->st_name >= symbols->strings_size) {
            return false;
        }
        if (strcmp(symbols->strings + sym->st_name, func_name) == 0) {
            *section_index = sym->st_shndx;

            if (addr) {
                *addr = sym->st_value;
            }
            if (symbol_index) {
                *symbol_index = k;
            }
            return true;
        }
    }

    return false;
}

ejAddr
findGotEntry64(const struct ejRelInfo *rels, uint64_t symbol_index)
{
    const unsigned char *object = rels->start;

    for (uint64_t k = 0; k < rels->count; k++, object += rels->object_size) {
        uint64_t info = *(uint64_t *)(object + rels->info_offset);

        if (ELF64_R_SYM(info) == symbol_index) {
            return *(Elf64_Addr *)object;
        }
    }

    return EJ_ADDR_NOT_FOUND;
}
