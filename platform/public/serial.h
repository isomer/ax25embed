/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Platform specific serial definitions.
 */
#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>

void serial_putch(uint8_t serial, uint8_t data);
void register_serial(uint8_t device, void (*rx)(uint8_t device, uint8_t ch), bool debug);

/** Receive byte from device.
 *
 * Not reentrant. (Don't call from interrupt handlers).
 *
 * Device can be 0..15.
 * Byte is the byte that was received on the serial device.
 */
void serial_recv_byte(uint8_t device, uint8_t byte);

#endif
