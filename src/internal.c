#include <stdarg.h>
#include <stdio.h>

#include <elfjack/config.h>

#include "internal.h"

static _Thread_local char error_buffer[EJ_ERROR_BUFFER_SIZE];

char *
ejGetError(void)
{
    return error_buffer;
}

void
emitError(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(error_buffer, sizeof(error_buffer), format, args);
    va_end(args);
}
