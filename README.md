Elfjack is a C library which performs parsing of ELF files with the goal of identifying function addresses and GOT (global offset table) entries.

API
===

The functionality can be accessed by

```c
#include <elfjack/elfjack.h>
```

An ELF file can be parsed with

```c
int
ejParseElf(const char *path, ejElfInfo *info);
```

This function returns `EJ_RET_OK` if successful and an error code otherwise (defined in [elfjack/elfjack.h](include/elfjack/elfjack.h)).

If `ejParseElf` fails, you can get a more descriptive explanation with

```c
char *
ejGetError(void);
```

This returns a pointer to a thread-local buffer of size `EJ_ERROR_BUFFER_SIZE` (defined in [elfjack/config.h](include/elfjack/config.h)).

When done with the info object, its resources can be released with

```c
ejReleaseInfo(ejElfInfo *info);
```

`ejElfInfo` is a mostly opaque structure, at least in intention.  That is, most of its fields should not be accessed by users of the library.  The exception is

```c
typedef struct ejElfInfo {
    // fields for internal use
    // ...
    struct {
        uint16_t machine;
        unsigned char pointer_size;
        unsigned int little_endian : 1;
    } visible;
} ejElfInfo;
```

`machine` holds the value of the `e_machine` field from the ELF header (see [`man elf`](https://www.man7.org/linux/man-pages/man5/elf.5.html)).  The other two fields should be self-explanatory.

Once you've parsed an ELF file, you can attempt to find the addresses of GOT entries with

```c
ejAddr
ejFindGotEntry(const ejElfInfo *info, const char *func_name);
```

where

```c
typedef unsigned long long ejAddr;
```

This function returns the relative address of the GOT entry if one was found and `EJ_ADDR_NOT_FOUND` otherwise.

You can likewise locate the start of a function within the `.text` section with

```c
ejAddr
ejFindFunction(const ejElfInfo *info, const char *func_name);
```

Once you know where an ELF file is loaded in virtual memory, you can convert a relative address to an absolute one with

```c
ejAddr
ejResolveAddress(const ejElfInfo *info, ejAddr addr, ejAddr file_start);
```

Building Elfjack
================

Shared and static libraries are built using make.  Adding `debug=yes` to the make invocation will disable optimization and build the libraries with debugging symbols.

You can also include Elfjack in a larger project by including make.mk.  Before doing so, however, the `EJ_DIR` variable must be set to the location of the Elfjack directory.  You can also tell make where to place the shared and static libraries by defining the `EJ_LIB_DIR` variable (defaults to `$(EJ_DIR)`).  Similarly, you can define the `EJ_OBJ_DIR` variable which tells make where to place the object files (defaults to `$(EJ_DIR`)/source).

make.mk adds a target to the `CLEAN_TARGETS` variable.  This is so that implementing

```make
clean: $(CLEAN_TARGETS)
    ...
```

in your project's Makefile will cause Elfjack to be cleaned up as well.

The `CLEAN_TARGETS` variable should be added to `.PHONY` if you're using GNU make.

make.mk defines the variables `EJ_SHARED_LIBRARY` and `EJ_STATIC_LIBRARY` which contain the paths of the specified libraries.

If passed no arguments, make creates the shared and static libraries as well as two executables, find_function and find_got.  They provide simple access to Elfjack's features.  E.g.,

```text
$ ./find_got some/elf/file some_func
GOT entry for some_func is at relative address 0xbeef
```
