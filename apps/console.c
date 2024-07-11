/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A basic "serial console"
 */
#include "console.h"
#include "ax25.h"
#include "debug.h"
#include "serial.h"
#include "token.h"
#include "tty.h"
#include <stddef.h>

static uint8_t console_buf[1024] = {0, };
static uint8_t *console_ptr = &console_buf[0];

static size_t console_len(void) {
    return console_ptr - console_buf;
}


void console_recv_byte(uint8_t device, uint8_t ch) {
    terminal_t *term = terminal_find_from_serial(device);
    switch (ch) {
        case '\r':
        case '\n':
            terminal_rx(term, (token_t) {
                    .ptr = console_buf,
                    .len = console_len(),
                    });
            console_ptr = &console_buf[0];
            break;
        /* TODO: Do I need to handle ^U, ^G, ^H etc? */
        default:
            if (console_len() < sizeof(console_buf))
                *(console_ptr++) = ch;
            break;
    }
}

