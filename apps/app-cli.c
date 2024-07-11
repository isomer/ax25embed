/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A "cli" application that lets you interact and configure the node.
 */
#include "app-cli.h"
#include "ax25.h"
#include "ax25_dl.h"
#include "cmd.h"
#include "debug.h"
#include "platform.h"
#include "serial.h"
#include <string.h> // for memcmp

static void cli_data(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    terminal_t *term = terminal_find_from_sock(sock);

    token_t cmd = token_new(data, datalen);
    token_t pid;

    if (!token_get_bytes(&cmd, &pid, 1)) {
        DEBUG(STR("truncated packet?"));
        return;
    }


    switch (*pid.ptr) {
        case PID_NOL3:
            terminal_rx(term, cmd);
            break;
        default:
            DEBUG(STR("Unexpected PID="), X8(*pid.ptr));
            break;
    }
}

static void cli_disconnect(dl_socket_t *sock) {
    (void) sock;
    DEBUG(STR("disconnect"));
}

static void cli_error(dl_socket_t *sock, ax25_dl_error_t err) {
    (void) sock;
    DEBUG(STR("Got error="), STR(ax25_dl_strerror(err)));
}

static void cli_connect(dl_socket_t *sock) {
    DEBUG(STR("connected"));
    sock->on_error = cli_error;
    sock->on_data = cli_data;
    sock->on_disconnect = cli_disconnect;
    terminal_t *term = terminal_find_or_allocate_from_sock(sock);
    term->rx = cmd_run;
}

void cli_init(token_t cmdline) {
    ssid_t local;
    if (!token_get_ssid(&cmdline, &local)) {
        DEBUG(STR("Failed to parse SSID"));
        return;
    }
    dl_socket_t *listener = dl_find_or_add_listener(&local);
    listener->on_connect = cli_connect;
}

static const char *init_script[] = {
    "serial 2 console",
    "serial 0 kiss",
    "serial 1 kiss",
    "register cli 2E0ITB-1",
    "register caseflip 2E0ITB-2",
    NULL,
};

int main(int argc, char *argv[]) {
    platform_init(argc, argv);
    ax25_init();
    cmd_connect_init();
    cmd_register_init();
    cmd_serial_init();
    terminal_t *term = terminal_get_null();
    for(size_t it = 0; init_script[it]; ++it) {
        cmd_run(term, token_from_str(init_script[it]));
    }
    DEBUG(STR("Running"));
    platform_run();
}

