#include <stdio.h>
#include <string.h>

#include <elfjack/elfjack.h>

static void
usage(const char *executable)
{
    fprintf(stderr, "Usage: %s [path to elf] [function name]", executable);
}

int
main(int argc, char **argv)
{
    int ret;
    const char *executable = argv[0], *function_name;
    ejAddr addr;
    ejElfInfo info;

    if (argc < 2) {
        usage(executable);
        return EJ_RET_BAD_USAGE;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage(executable);
        return 0;
    }

    if (argc != 3) {
        usage(executable);
        return EJ_RET_BAD_USAGE;
    }

    ret = ejParseElf(argv[1], &info);
    if (ret != EJ_RET_OK) {
        fprintf(stderr, "Failed to parse ELF file: %s\n", ejGetError());
        return ret;
    }

    function_name = argv[2];
    addr = ejFindFunction(&info, function_name);
    if (addr == EJ_ADDR_NOT_FOUND) {
        ret = 1;
    }
    else {
        printf("0x%llx\n", addr);
    }

    ejReleaseInfo(&info);
    return ret;
}
