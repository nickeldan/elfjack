#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "internal.h"
#include "parse32.h"
#include "parse64.h"

#define INFO_INITIALIZED(info) ((info) && (info)->map.data)

static size_t
pageSize(void)
{
    static _Thread_local size_t page_size;

    if (page_size == 0) {
        page_size = sysconf(_SC_PAGESIZE);
    }
    return page_size;
}

static size_t
roundUpToPageSize(size_t size)
{
    size_t page_size, rounded;

    page_size = pageSize();
    rounded = (size + page_size - 1);
    rounded -= rounded % page_size;
    return rounded;
}

static int
parseElfHeader(ejElfInfo *info, struct ehdrParams *params)
{
    uint16_t phentsize, shentsize;
    const Elf64_Ehdr *ehdr_64 = info->map.data;
    const Elf32_Ehdr *ehdr_32 = info->map.data;

    if (info->map.size <= EI_DATA || memcmp(info->map.data, ELFMAG, SELFMAG) != 0) {
        emitError("Not an ELF file");
        return EJ_RET_NOT_ELF;
    }

    switch (ehdr_64->e_ident[EI_CLASS]) {
    case ELFCLASS32: params->_64 = false; break;
    case ELFCLASS64: params->_64 = true; break;
    default: emitError("Invalid EI_CLASS in ELF header"); return EJ_RET_MALFORMED_ELF;
    }

    switch (ehdr_64->e_ident[EI_DATA]) {
    case ELFDATA2LSB: info->visible.little_endian = true; break;
    case ELFDATA2MSB: info->visible.little_endian = false; break;
    default: emitError("Invalid EI_DATA in ELF header"); return EJ_RET_MALFORMED_ELF;
    }

    if (info->map.size < (params->_64 ? sizeof(Elf64_Ehdr) : sizeof(Elf32_Ehdr))) {
        emitError("File is not big enough to contain the ELF header");
        return EJ_RET_MALFORMED_ELF;
    }

    info->visible.machine = ehdr_64->e_machine;

    if (params->_64) {
        params->phoff = ehdr_64->e_phoff;
        params->phnum = ehdr_64->e_phnum;
        params->shoff = ehdr_64->e_shoff;
        params->shnum = ehdr_64->e_shnum;
        params->shstrndx = ehdr_64->e_shstrndx;
        phentsize = ehdr_64->e_phentsize;
        shentsize = ehdr_64->e_shentsize;
    }
    else {
        params->phoff = ehdr_32->e_phoff;
        params->phnum = ehdr_32->e_phnum;
        params->shoff = ehdr_32->e_shoff;
        params->shnum = ehdr_32->e_shnum;
        params->shstrndx = ehdr_32->e_shstrndx;
        phentsize = ehdr_32->e_phentsize;
        shentsize = ehdr_32->e_shentsize;
    }

    if (params->shstrndx >= params->shnum) {
        emitError("Invalid e_shstrndx in ELF header");
        return EJ_RET_MALFORMED_ELF;
    }

    switch (ehdr_64->e_type) {
    case ET_DYN: info->dynamic = true; break;
    case ET_EXEC: break;
    default: emitError("File is neither an executable nor a shared object"); return EJ_RET_NOT_ELF;
    }

    if (params->shnum >= SHN_LORESERVE) {
        const void *sheader = AT_OFFSET(info->map.data, params->shoff);

        if ((unsigned long)params->shoff + shentsize > info->map.size) {
            emitError("File is not big enough to contain the section header table");
            return EJ_RET_MALFORMED_ELF;
        }

        if (params->_64) {
            const Elf64_Shdr *shdr = sheader;

            params->shnum = shdr->sh_size;
        }
        else {
            const Elf32_Shdr *shdr = sheader;

            params->shnum = shdr->sh_size;
        }
    }

    if (params->shnum == 0) {
        emitError("File contains no section headers");
        return EJ_RET_MISSING_INFO;
    }
    if ((unsigned long)params->shoff + shentsize * params->shnum > info->map.size) {
        emitError("File is not big enough to contain the section header table");
        return EJ_RET_MALFORMED_ELF;
    }

    if (params->phnum >= PN_XNUM) {
        unsigned int info_offset =
            params->_64 ? offsetof(Elf64_Shdr, sh_info) : offsetof(Elf32_Shdr, sh_info);

        params->phnum = *(uint32_t *)AT_OFFSET(info->map.data, params->shoff + info_offset);
    }

    if (params->phnum == 0) {
        emitError("File contains no program headers");
        return EJ_RET_MISSING_INFO;
    }
    if ((unsigned long)(params->phoff + phentsize * params->phnum) > info->map.size) {
        emitError("File is not big enough to contain the program header table");
        return EJ_RET_MALFORMED_ELF;
    }

    return EJ_RET_OK;
}

int
ejParseElf(const char *path, ejElfInfo *info)
{
    int ret, fd, local_errno;
    unsigned int load_addr;
    size_t page_mask;
    struct stat fs;
    struct ehdrParams params;
    int (*find_load_addr)(const void *, uint16_t, unsigned int *);
    int (*find_shdrs)(ejElfInfo *, const struct ehdrParams *);

    if (!path || !info) {
        emitError("The arguments cannot be NULL");
        return EJ_RET_BAD_USAGE;
    }

    *info = EJ_ELF_INFO_INIT;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        emitError("open: %s", strerror(errno));
        return EJ_RET_READ_FAILURE;
    }
    if (fstat(fd, &fs) != 0) {
        emitError("fstat: %s", strerror(errno));
        close(fd);
        return EJ_RET_READ_FAILURE;
    }

    info->map.size = fs.st_size;
    info->map.map_size = roundUpToPageSize(fs.st_size);
    info->map.data = mmap(NULL, info->map.map_size, PROT_READ, MAP_PRIVATE, fd, 0);
    local_errno = errno;
    close(fd);
    if (info->map.data == MAP_FAILED) {
        emitError("mmap: %s", strerror(local_errno));
        info->map.data = NULL;
        return EJ_RET_MAP_FAIL;
    }

    ret = parseElfHeader(info, &params);
    if (ret != EJ_RET_OK) {
        goto error;
    }

    if (params._64) {
        find_load_addr = findLoadAddr64;
        find_shdrs = findShdrs64;
        info->find_symbol = findSymbol64;
        info->find_got_entry = findGotEntry64;
        info->visible.pointer_size = 8;
    }
    else {
        find_load_addr = findLoadAddr32;
        find_shdrs = findShdrs32;
        info->find_symbol = findSymbol32;
        info->find_got_entry = findGotEntry32;
        info->visible.pointer_size = 4;
    }

    ret = find_load_addr(AT_OFFSET(info->map.data, params.phoff), params.phnum, &load_addr);
    if (ret != EJ_RET_OK) {
        emitError("No LOAD segment found");
        goto error;
    }

    page_mask = ~(pageSize() - 1);
    info->load_bias = load_addr & page_mask;

    ret = find_shdrs(info, &params);
    if (ret != EJ_RET_OK) {
        goto error;
    }

    if (!info->symbols.start) {
        emitError(".dynsym not found");
    }
    else if (!info->symbols.strings) {
        emitError(".dynstr not found");
    }
    else if (info->text_section_index == 0) {
        emitError(".text not found");
    }
    else {
        return EJ_RET_OK;
    }

    ret = EJ_RET_MISSING_INFO;

error:
    ejReleaseInfo(info);
    return ret;
}

void
ejReleaseInfo(ejElfInfo *info)
{
    if (!INFO_INITIALIZED(info)) {
        return;
    }

    munmap((void *)info->map.data, info->map.map_size);
    info->map.data = NULL;
}

ejAddr
ejFindGotEntry(const ejElfInfo *info, const char *func_name)
{
    uint64_t symbol_index;

    if (!INFO_INITIALIZED(info) || !func_name || !info->rels.start) {
        return EJ_ADDR_NOT_FOUND;
    }

    if (!info->find_symbol(&info->symbols, func_name, 0, NULL, &symbol_index)) {
        return EJ_ADDR_NOT_FOUND;
    }

    return info->find_got_entry(&info->rels, symbol_index);
}

ejAddr
ejFindFunction(const ejElfInfo *info, const char *func_name)
{
    ejAddr addr;

    if (!INFO_INITIALIZED(info) || !func_name) {
        return EJ_ADDR_NOT_FOUND;
    }

    if (!info->find_symbol(&info->symbols, func_name, info->text_section_index, &addr, NULL)) {
        return EJ_ADDR_NOT_FOUND;
    }

    return addr;
}

ejAddr
ejResolveAddress(const ejElfInfo *info, ejAddr addr, ejAddr file_start)
{
    if (!INFO_INITIALIZED(info)) {
        return EJ_ADDR_NOT_FOUND;
    }

    if (info->dynamic) {
        addr += file_start;
    }
    return addr - info->load_bias;
}
