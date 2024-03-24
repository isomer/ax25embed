/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * additional platform apis that rely on posix
 */
#include "platform.h"
#include "time.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TIME_BASE INT64_C(1000000000)

instant_t instant_now(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        panic("clock_gettime(CLOCK_MONOTONIC)");
    }

    return (instant_t) { .instant = ts.tv_sec * TIME_BASE + ts.tv_nsec, };
}

void debug_putch(char ch) {
    putchar(ch);
}

void debug(const char *msg) {
    puts(msg);
}

void panic(const char *msg) {
    debug(msg);
    abort();
}

void debug_internal_x8(struct debug_t *v) {
    printf("%x", v->u8);
}

void debug_internal_d8(struct debug_t *v) {
    printf("%d", v->u8);
}

void debug_internal_str(struct debug_t *v) {
    printf("%s", (const char *)v->ptr);
}

void debug_internal_eol(void) {
    putchar('\n');
    fflush(stdout);
}
