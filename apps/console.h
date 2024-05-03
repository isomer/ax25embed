/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A basic "serial console" API
 */
#ifndef CONSOLE_H
#define CONSOLE_H
#include <stdint.h>

void console_recv_byte(uint8_t device, uint8_t ch);

#endif
