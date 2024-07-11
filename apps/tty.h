/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Abstract Terminal
 */
#ifndef TTY_H
#define TTY_H
#include "token.h"

typedef struct terminal_t {
    enum { TERM_FREE = 0, TERM_SOCK, TERM_SERIAL, TERM_NULL } type;
    union {
        struct dl_socket_t *sock;
        uint8_t serial;
    };
    void (*rx)(struct terminal_t *term, token_t data);
} terminal_t;

void terminal_tx(terminal_t *term, token_t data);
static inline void terminal_rx(terminal_t *term, token_t data) { if (term->rx) term->rx(term, data); }
terminal_t *terminal_get_null(void);
terminal_t *terminal_find_from_sock(struct dl_socket_t *sock);
terminal_t *terminal_find_or_allocate_from_sock(struct dl_socket_t *sock);
terminal_t *terminal_find_from_serial(uint8_t serial);
terminal_t *terminal_find_or_allocate_from_serial(uint8_t serial);
void terminal_free(terminal_t **term);


#endif
