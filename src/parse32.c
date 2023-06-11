#include <stddef.h>
#include <string.h>

#include "parse32.h"

static bool
shdrSanityCheck(const ejElfInfo *info, const Elf32_Shdr *shdr)
{
    return info->helpers.get_u32(&shdr->sh_offset) + info->helpers.get_u32(&shdr->sh_size) <= info->map.size;
}

static const char *
getShdrStrings(const ejElfInfo *info, const Elf32_Shdr *shdr, size_t *size)
{
    const char *strings;

    if (!shdrSanityCheck(info, shdr)) {
        ejEmitError("File is not big enough to contain the section header string table");
        return NULL;
    }

    strings = AT_OFFSET(info->map.data, info->helpers.get_u32(&shdr->sh_offset));
    *size = shdr->sh_size;
    if (*size == 0) {
        ejEmitError("Section header string table is empty");
        return NULL;
    }
    if (strings[*size - 1] != '\0') {
        ejEmitError("Section header string table is not null-terminated");
        return NULL;
    }

    return strings;
}

int
ejFindLoadAddr32(const struct ejIntHelpers *helpers, const void *pheader, uint32_t phnum,
                 unsigned int *load_addr)
{
    const Elf32_Phdr *phdr = pheader;

    for (uint32_t k = 0; k < phnum; k++) {
        if (helpers->get_u32(&phdr[k].p_type) == PT_LOAD) {
            *load_addr = helpers->get_u32(&phdr[k].p_offset);
            return EJ_RET_OK;
        }
    }

    return EJ_RET_MISSING_INFO;
}

int
ejFindShdrs32(ejElfInfo *info, const struct ehdrParams *params)
{
    unsigned int num_found = 0;
    size_t strings_size;
    const char *strings;
    const Elf32_Shdr *table = AT_OFFSET(info->map.data, params->shoff);

    strings = getShdrStrings(info, &table[params->shstrndx], &strings_size);
    if (!strings) {
        return EJ_RET_MALFORMED_ELF;
    }

    for (uint64_t k = 1; k < params->shnum; k++) {
        uint32_t name, size, entsize;
        const char *section_name;
        const void *section_start;
        const Elf32_Shdr *shdr = &table[k];

        if (k == params->shstrndx) {
            continue;
        }

        if (!shdrSanityCheck(info, shdr) || (name = info->helpers.get_u32(&shdr->sh_name)) >= strings_size) {
            ejEmitError("File is not big enough to contain section #%llu", (unsigned long long)k);
            return EJ_RET_MALFORMED_ELF;
        }
        section_start = AT_OFFSET(info->map.data, shdr->sh_offset);
        section_name = strings + shdr->sh_name;

        size = info->helpers.get_u32(&shdr->sh_size);
        entsize = info->helpers.get_u32(&shdr->sh_entsize);
        if (!info->symbols.start && strcmp(section_name, ".dynsym") == 0) {
            if (entsize == 0) {
                ejEmitError(".dynsym section has invalid sh_entsize");
                return EJ_RET_MALFORMED_ELF;
            }
            info->symbols.start = section_start;
            info->symbols.count = size / entsize;
        }
        else if (!info->symbols.strings && strcmp(section_name, ".dynstr") == 0) {
            info->symbols.strings = section_start;
            info->symbols.strings_size = size;
            if (info->symbols.strings_size == 0) {
                ejEmitError(".dynstr is empty");
                return EJ_RET_MALFORMED_ELF;
            }
            if (info->symbols.strings[info->symbols.strings_size - 1] != '\0') {
                ejEmitError(".dynstr is not null-terminated");
                return EJ_RET_MALFORMED_ELF;
            }
        }
        else if (!info->rels.start &&
                 (strcmp(section_name, ".rela.plt") == 0 || strcmp(section_name, ".rel.plt") == 0)) {
            if (entsize == 0) {
                ejEmitError(".rela.plt section has invalid sh_entsize");
                return EJ_RET_MALFORMED_ELF;
            }
            info->rels.start = section_start;
            info->rels.count = size / entsize;
            if (shdr->sh_type == SHT_RELA) {
                info->rels.object_size = sizeof(Elf32_Rela);
                info->rels.info_offset = offsetof(Elf32_Rela, r_info);
            }
            else {
                info->rels.object_size = sizeof(Elf32_Rel);
                info->rels.info_offset = offsetof(Elf32_Rel, r_info);
            }
        }
        else if (info->text_section_index == 0 && strcmp(section_name, ".text") == 0) {
            info->text_section_index = k;
        }
        else {
            continue;
        }

        if (++num_found == 4) {
            break;
        }
    }

    return EJ_RET_OK;
}

bool
ejFindSymbol32(const ejElfInfo *info, const char *func_name, uint16_t section_index, ejSymbolValue *value)
{
    const Elf32_Sym *syms = info->symbols.start;

    for (uint64_t k = 0; k < info->symbols.count; k++) {
        uint32_t name;
        const Elf32_Sym *sym = &syms[k];

        if (ELF32_ST_TYPE(sym->st_info) != STT_FUNC ||
            info->helpers.get_u16(&sym->st_shndx) != section_index) {
            continue;
        }

        name = info->helpers.get_u32(&sym->st_name);
        if (name >= info->symbols.strings_size) {
            return false;
        }
        if (strcmp(info->symbols.strings + name, func_name) == 0) {
            if (section_index == 0) {
                value->index = k;
            }
            else {
                value->addr = info->helpers.get_u32(&sym->st_value);
            }
            return true;
        }
    }

    return false;
}

ejAddr
ejFindGotEntry32(const ejElfInfo *info, uint64_t symbol_index)
{
    const unsigned char *object = info->rels.start;

    for (uint64_t k = 0; k < info->rels.count; k++, object += info->rels.object_size) {
        uint32_t rel_info;

        rel_info = info->helpers.get_u32(object + info->rels.info_offset);
        if (ELF32_R_SYM(rel_info) == symbol_index) {
            return info->helpers.get_u32(object);
        }
    }

    return EJ_ADDR_NOT_FOUND;
}
