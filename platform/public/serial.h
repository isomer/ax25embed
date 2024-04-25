/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Platform specific serial definitions.
 */
#include <stdint.h>

void serial_init(int argc, char *argv[]);
void serial_putch(uint8_t serial, uint8_t data);
