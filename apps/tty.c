/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Abstract Terminal
 */
#include "tty.h"
#include "ax25.h"
#include "debug.h"
#include "ax25_dl.h"
#include "serial.h"
#include <string.h> // for memcpy

enum { MAX_TERMINALS = 5 };

static terminal_t terminals[MAX_TERMINALS] = { { .type = TERM_FREE }, };
static terminal_t null_terminal = {
    .type = TERM_NULL,
    .rx = NULL,
};

terminal_t *terminal_find_from_sock(dl_socket_t *sock) {
    for(size_t i = 0; i < MAX_TERMINALS; ++i) {
        if (terminals[i].type == TERM_SOCK && terminals[i].sock == sock)
            return &terminals[i];
    }
    return NULL;
}

static terminal_t *terminal_allocate(void) {
    for(size_t i = 0; i < MAX_TERMINALS; ++i) {
        if (terminals[i].type == TERM_FREE)
            return &terminals[i];
    }
    return NULL; /* Table full */
}

terminal_t *terminal_get_null(void) {
    return &null_terminal;
}

terminal_t *terminal_find_or_allocate_from_sock(dl_socket_t *sock) {
    terminal_t *term = terminal_find_from_sock(sock);
    if (term)
        return term;
    term = terminal_allocate();
    if (term) {
        term->type = TERM_SOCK;
        term->sock = sock;
        term->rx = NULL;
    }
    return term;
}

terminal_t *terminal_find_from_serial(uint8_t serial) {
    for(size_t i = 0; i < MAX_TERMINALS; ++i) {
        if (terminals[i].type == TERM_SERIAL && terminals[i].serial == serial)
            return &terminals[i];
    }
    return NULL;
}

terminal_t *terminal_find_or_allocate_from_serial(uint8_t serial) {
    terminal_t *term = terminal_find_from_serial(serial);
    if (term)
        return term;
    term = terminal_allocate();
    if (term) {
        term->type = TERM_SERIAL;
        term->serial = serial;
        term->rx = NULL;
    }
    return term;
}

void terminal_free(terminal_t **term) {
    (*term)->type = TERM_FREE;
    (*term) = NULL;
}

void terminal_tx(terminal_t *term, token_t data) {
    CHECK(term != NULL);
    switch (term->type) {
        case TERM_FREE:
            panic("TX on free terminal?!");
        case TERM_SOCK: {
            uint8_t buf[data.len+2];
            buf[0] = PID_NOL3;
            memcpy(&buf[1], data.ptr, data.len);
            buf[data.len+1] = '\r';

            dl_send(term->sock, buf, data.len+2);
            return;
        }
        case TERM_SERIAL:
            for (size_t i = 1 /* Skip PID */; i < data.len; ++i) {
                serial_putch(term->serial, data.ptr[i]);
            }
            serial_putch(term->serial, '\r');
            return;
        case TERM_NULL:
            return;
    }
    UNIMPLEMENTED();
}

