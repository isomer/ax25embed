/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A command to reconfigure serial ports
 */
#include "app-cli.h"
#include "ax25.h"
#include "ax25_dl.h"
#include "console.h"
#include "kiss.h"
#include "serial.h"

static void cmd_serial(dl_socket_t *sock, token_t cmdline) {
    uint8_t serial;
    if (!token_get_u8(&cmdline, &serial)) {
        OUTPUT(sock, STR("Unparsable serial port"));
        return;
    }
    if (serial > 2 /* TODO: MAX_SERIAL */) {
        OUTPUT(sock, STR("Invalid serial "), D8(serial));
        return;
    }
    skipwhite(&cmdline);
    token_t protocol;
    if (!token_get_word(&cmdline, &protocol)) {
        OUTPUT(sock, STR("Missing protocol for serial "), D8(serial));
        return;
    }

    if (token_cmp(protocol, token_from_str("kiss")) == 0) {
        register_serial(serial, kiss_recv_byte, false);
    } else if (token_cmp(protocol, token_from_str("console")) == 0) {
        register_serial(serial, console_recv_byte, true);
    } else {
        OUTPUT(sock, STR("Unknown protocol "), LENSTR(protocol.ptr, protocol.len), STR(" for serial "), D8(serial));
    }
}

static command_t command_serial = {
    .next = NULL,
    .name = "serial",
    .cmd = cmd_serial,
};

void cmd_serial_init(void) {
    register_cmd(&command_serial);
}

