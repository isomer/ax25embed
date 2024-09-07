/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Additional platform apis that rely on posix
 */
#define _POSIX_C_SOURCE 200809L
#include "platform.h"
#include "debug.h"
#include "clock.h"
#include <stdlib.h>
#include <time.h>

/* This returns a monotonic increasing timestamp. */
instant_t instant_now(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        panic("clock_gettime(CLOCK_MONOTONIC)");
    }

    return instant_add(INSTANT_ZERO,
            duration_add(
                duration_seconds(ts.tv_sec),
                duration_nanos(ts.tv_nsec)));
}

/* This should panic the system, potentially displaying a message */
void panic(const char *msg) {
    DEBUG(STR(msg));
    abort();
}

void register_ticker(ticker_t *ticker) {
    /* For the null platform we don't register (nor run) tickers, but for a
     * functional system tickers registered with this should all be called
     * relatively frequently (eg every 10ms).  Tickers can give a hint when
     * they next should be called to allow lower power states.
     */
}

void platform_init(int argc, char *argv[]) {
    /* Nothing to do by default */
}

void platform_run(void) {
    /* This function should call ticker's occasionally, and call
     * `void * serial_recv_byte(uint8_t serialport, uint8_t byte);` when a byte
     * is received on a serialport.  serialport numbers are 0..15.
     *
     * This function is expected to not exit.
     */
}
