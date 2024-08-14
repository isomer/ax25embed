/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A dummy serial port that writes everything to /dev/null and never receives
 * any data.
 *
 * Used for fuzz testing.
 */
#include "serial.h"

void serial_putch(uint8_t serial, uint8_t data) {
    /* Don't send any data */
    (void) serial;
    (void) data;
}

