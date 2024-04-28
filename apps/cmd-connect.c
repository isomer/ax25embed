/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A command to connect to another node.
 */
#include "app-cli.h"
#include "ax25.h"
#include "ax25_dl.h"

static void cmd_connect(dl_socket_t *sock, token_t cmdline) {
    OUTPUT(sock, STR("Not implemented"));
    (void) cmdline;
}

static command_t command_connect = {
    .next = NULL,
    .name = "connect",
    .cmd = cmd_connect,
};

void cmd_connect_init(void) {
    register_cmd(&command_connect);
}
