/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Platform specific code.
 */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include <stdnoreturn.h>
#include "clock.h"

typedef struct ticker_t {
    struct ticker_t *next;
    duration_t (*tick)(void);
} ticker_t;

/** A ticker is a function that will be called every "tick".
 * The function can return a hint of how long the process should sleep until
 * the next tick, when all ticker functions will be called again.
 */
void register_ticker(ticker_t *ticker_t);

/* Called at startup to initialise the platform APIs. */
void platform_init(int argc, char *argv[]);

/* Runs the program, calling back for configured events. */
void platform_run(void);

/* Crash the program with a message */
noreturn void panic(const char *msg);

/* Output one charactor to the debug port */
void debug_putch(char ch);

#endif
