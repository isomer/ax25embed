/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * additional platform apis that rely on posix
 */
#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include "debug.h"
#include "clock.h"
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

void panic(const char *msg) {
    DEBUG(STR(msg));
    abort();
}

void register_ticker(ticker_t *ticker) {
    /* Don't ever run any tickers */
}

void platform_init(int argc, char *argv[]) {
    /* Nothing to do for POSIX */
}

void platform_run(void) {
    /* Don't do anytihng */
}
