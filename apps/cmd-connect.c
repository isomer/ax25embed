/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A command to connect to another node.
 */
#include "app-cli.h"
#include "ax25.h"
#include "ax25_dl.h"
#include "cmd.h"
#include "tty.h"

static terminal_t *term1, *term2;

static void connect_term_rx(terminal_t *term, token_t token) {
    if (term == term1)
        terminal_tx(term2, token);
    else
        terminal_tx(term1, token);
}

static void connect_data(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    (void) sock;
    (void) data;
    terminal_t *term = terminal_find_from_sock(sock);
    uint8_t buffer[datalen];
    for(size_t i = 0; i < datalen; ++i) {
        switch (data[i]) {
            case '\r':
                buffer[i] = '\n';
                break;
            default:
                buffer[i] = data[i];
                break;
        }
    }
    terminal_rx(term, token_new(buffer, datalen));
}

static void connect_disconnect(dl_socket_t *sock) {
    (void) sock;
    DEBUG(STR("disconnect"));
}

static void connect_connect(dl_socket_t *sock) {
    DEBUG(STR("connected"));
    terminal_t *term = terminal_find_or_allocate_from_sock(sock);
    term->rx = connect_term_rx;
    term2 = term;
}


static void cmd_connect(terminal_t *term, token_t cmdline) {
    ssid_t local;
    ssid_t remote;
    uint8_t port;
    if (!token_get_ssid(&cmdline, &local)) {
        OUTPUT(term, STR("Failed to parse local SSID"));
        return;
    }
    if (!token_get_ssid(&cmdline, &remote)) {
        OUTPUT(term, STR("Failed to parse remote SSID"));
        return;
    }
    if (!token_get_u8(&cmdline, &port)) {
        OUTPUT(term, STR("expected port"));
        return;
    }
    dl_socket_t *conn = dl_connect(&remote, &local, port);
    conn->on_connect = connect_connect;
    conn->on_data = connect_data;
    conn->on_disconnect = connect_disconnect;

    term->rx = connect_term_rx;
    term1 = term;
    term2 = NULL;

    (void) cmdline;
}

static command_t command_connect = {
    .next = NULL,
    .name = "connect",
    .help = "connect <local ssid> <remote ssid> <portnum>",
    .cmd = cmd_connect,
};

void cmd_connect_init(void) {
    register_cmd(&command_connect);
}
