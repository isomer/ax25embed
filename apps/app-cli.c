/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A "cli" application that lets you interact and configure the node.
 */
#include "app-cli.h"
#include "ax25.h"
#include "ax25_dl.h"
#include "debug.h"
#include "platform.h"
#include "serial.h"
#include <string.h> // for memcmp

static command_t *commands = NULL;

void register_cmd(command_t *command) {
    command->next = commands;
    commands = command;
}

void cli_output(dl_socket_t *sock, const char *msg, size_t len) {
    if (sock) {
        dl_send(sock, msg, len);
    } else {
        DEBUG(LENSTR(&msg[1], len-2)); /* skip the PID and truncate the \n */
    }
}

static void do_cmd(dl_socket_t *sock, token_t data) {
    token_t cmd;
    DEBUG(STR("Command: "), LENSTR(data.ptr, data.len));

    skipwhite(&data);
    if (!token_get_word(&data, &cmd)) {
        OUTPUT(sock, STR("Failed to read command"));
        return;
    }

    for(command_t *it = commands; it; it=it->next) {
        if (token_cmp(cmd, token_from_str(it->name)) == 0) {
            it->cmd(sock, data);
            return;
        }
    }

    OUTPUT(sock, STR("Unknown command: ["), LENSTR(cmd.ptr, cmd.len), STR("]"));
}

static void cli_data(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    token_t cmd = token_new(data, datalen);
    token_t pid;

    if (!token_get_bytes(&cmd, &pid, 1)) {
        DEBUG(STR("truncated packet?"));
        return;
    }

    switch (*pid.ptr) {
        case PID_NOL3:
            do_cmd(sock, cmd);
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
}

void cli_init(token_t cmdline) {
    token_t ssid;
    if (!token_get_word(&cmdline, &ssid)) {
        DEBUG(STR("Missing SSID"));
        return;
    }
    ssid_t local;
    if (!ssid_from_string((const char *)ssid.ptr, &local)) {
        DEBUG(STR("Failed to parse SSID: "), LENSTR(ssid.ptr, ssid.len));
        return;
    }
    dl_socket_t *listener = dl_find_or_add_listener(&local);
    listener->on_connect = cli_connect;
}

static const char *init_script[] = {
    "register cli 2E0ITB-1",
    "register caseflip 2E0ITB-2",
    NULL,
};

int main(int argc, char *argv[]) {
    platform_init(argc, argv);
    ax25_init();
    cmd_connect_init();
    cmd_register_init();
    for(size_t it = 0; init_script[it]; ++it) {
        do_cmd(NULL, token_from_str(init_script[it]));
    }
    DEBUG(STR("Running"));
    platform_run();
}

