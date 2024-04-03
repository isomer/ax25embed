/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * additional platform apis that rely on posix
 */
#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include "debug.h"
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

void panic(const char *msg) {
    DEBUG(STR(msg));
    abort();
}

void platform_init(void) {
    /* Nothing to do for POSIX */
}
