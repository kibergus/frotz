#include "chibi_frotz.h"

#include <ch.h>
#include <hal.h>
#include <chprintf.h>

#include <string.h>
#include <stdarg.h>

#define __UNIX_PORT_FILE
#include "../common/frotz.h"

void os_fatal (const char *s, ...)
{
    ch_fputs("\nFatal error: ");
    ch_fputs(s);
    ch_fputc('\n');    
    chThdExit(1);
}

time_t time(time_t *tloc)
{
    time_t t = ST2S(chVTGetSystemTime());
    if (tloc)
        *tloc = t;
    return t;
}

int user_random_seed = -1;
int os_random_seed (void)
{
    if (user_random_seed == -1) {
        /* Use the epoch as seed value */
        RTCDateTime timespec;
        rtcGetTime(&RTCD1, &timespec);
        return timespec.millisecond & 0x7fff;
    } else {
        return user_random_seed;
    }
}

void* ch_malloc(size_t size)
{
    return chHeapAlloc(NULL, size);
}

void ch_free(void *ptr)
{
    chHeapFree(ptr);
}

void* ch_realloc(void *ptr, size_t size)
{
    union heap_header* hp = (union heap_header *)ptr - 1;
    if (size <= hp->h.size)
        return ptr;

    void* newPtr = ch_malloc(size);
    memcpy(newPtr, ptr, hp->h.size);
    ch_free(ptr);

    return newPtr;
}
