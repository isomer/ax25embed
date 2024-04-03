/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Platform specific code.
 */
#ifndef PLATFORM_H
#define PLATFORM_H
#include <stdint.h>
#include <stdnoreturn.h>

/* Called at startup to initialise the platform APIs. */
void platform_init(void);

/* Crash the program with a message */
noreturn void panic(const char *msg);

/* Output one charactor to the debug port */
void debug_putch(char ch);

#endif
