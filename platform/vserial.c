/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Indirection layer between physical devices and the serial protocols.
 */
#include "debug.h"
#include "serial.h"

typedef struct vserial_t {
    void (*rx)(uint8_t port, uint8_t ch);
    /// Does this port get debug messages
    bool debug;
} serial_t;

enum { MAX_DEVICES = 3 };

static serial_t device2vserial[MAX_DEVICES];

void register_serial(uint8_t device, void (*rx)(uint8_t device, uint8_t ch), bool debug) {
    CHECK(device < MAX_DEVICES);
    device2vserial[device].rx = rx;
    device2vserial[device].debug = debug;
}

void serial_recv_byte(uint8_t device, uint8_t byte) {
    CHECK(device < MAX_DEVICES);
    if (device2vserial[device].rx)
        device2vserial[device].rx(device, byte);
}

void debug_putch(char ch) {
    for(size_t i = 0; i < MAX_DEVICES; ++i) {
        if (device2vserial[i].debug)
            serial_putch(i, ch);
    }
}
