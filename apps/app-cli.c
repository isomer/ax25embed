/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * A "cli" application that lets you interact and configure the node.
 */
#include "ax25.h"
#include "ax25_dl.h"
#include "debug.h"
#include "platform.h"
#include "serial.h"
#include <string.h> // for memcmp

#define OUTPUT(sock, ...) \
    do { \
        char output_buffer[1024]; \
        char *output_ptr = &output_buffer[1]; \
        output_buffer[0] = '\xF0'; \
        size_t output_buffer_len = sizeof(output_buffer)-1; \
        FORMAT(&output_ptr, &output_buffer_len, __VA_ARGS__); \
        output((sock), output_buffer, sizeof(output_buffer) - output_buffer_len); \
    } while(0)


typedef struct cli_command_t {
    const uint8_t *name;
    void (*command)(dl_socket_t *sock, const uint8_t data[], size_t datalen);
} cli_command_t;

static void cli_error(dl_socket_t *sock, ax25_dl_error_t err) {
    (void) sock;
    DEBUG(STR("Got error="), STR(ax25_dl_strerror(err)));
}

/* Copies bytelen bytes from data to byte, returning true on success */
static bool peak_bytes(const uint8_t data[], size_t datalen, uint8_t *byte, size_t bytelen) {
    if (datalen < bytelen) {
        return false;
    }
    memcpy(byte, data, bytelen);
    return true;
}

/* copies bytelen bytes from data to byte, and moves data on by that many bytes
 * (and shrinks datalen). returs true on success */
static bool get_bytes(const uint8_t **data, size_t *datalen, uint8_t *byte, size_t bytelen) {
    if (!peak_bytes(*data, *datalen, byte, bytelen))
        return false;
    (*data) += bytelen;
    (*datalen) -= bytelen;
    return true;
}

static bool is_white(uint8_t ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

static void skipwhite(const uint8_t **data, size_t *datalen) {
    while (*datalen > 0 && is_white((*data)[0])) {
        (*data)++;
        (*datalen)--;
    }
}

static bool get_token(const uint8_t **data, size_t *datalen, uint8_t *token, size_t *tokenlen) {
    size_t tokenrem = *tokenlen;
    while(*datalen > 0 && !is_white((*data)[0])) {
        if (tokenrem > 0) {
            *(token++) = (*data)[0];
            tokenrem--;
        } else
            return false;
        (*data)++;
        (*datalen)--;
    }
    *tokenlen = *tokenlen - tokenrem;
    return true;
}

static int cmp_token(const uint8_t *lhs, size_t lhslen, const uint8_t *rhs, size_t rhslen) {
    int ret = memcmp(lhs, rhs, lhslen < rhslen ? lhslen : rhslen);
    if (ret != 0)
        return ret;
    return lhslen - rhslen;
}

static size_t my_strlen(const uint8_t *str) {
    size_t len = 0;
    while(*str) {
        len++;
        str++;
    }
    return len;
}

static void output(dl_socket_t *sock, const char *msg, size_t len) {
    if (sock) {
        dl_send(sock, msg, len);
    } else {
        DEBUG(LENSTR(&msg[1], len-2)); /* skip the PID and truncate the \n */
    }
}


static void cmd_connect(dl_socket_t *sock, const uint8_t data[], size_t datalen) {
    (void) sock;
    (void) data;
    (void) datalen;
    OUTPUT(sock, STR("Not implemented"));
    UNIMPLEMENTED();
}

extern void caseflip_init(ssid_t *ssid, const uint8_t data[], size_t datalen);
static void cli_init(ssid_t *ssid, const uint8_t data[], size_t datalen);

static const struct apps_t {
    uint8_t *name;
    void (*init)(ssid_t *ssid, const uint8_t data[], size_t datalen);
} apps[] = {
    { (uint8_t *)"cli", cli_init },
    { (uint8_t *)"caseflip", caseflip_init },
    { NULL, NULL },
};

static void cmd_register(dl_socket_t *sock, const uint8_t data[], size_t datalen) {
    const char *help = "register <ssid> <app>\n";
    uint8_t ssid[8];
    size_t ssid_len = sizeof(ssid);
    uint8_t app[8];
    size_t app_len = sizeof(app);
    ssid_t local_ssid;
    skipwhite(&data, &datalen);

    if (!get_token(&data, &datalen, ssid, &ssid_len)) {
        OUTPUT(sock, STR(help));
        return;
    }

    if (!ssid_from_string((char *)ssid, &local_ssid)) {
        OUTPUT(sock, STR("Invalid SSID"));
        return;
    }

    skipwhite(&data, &datalen);

    if (!get_token(&data, &datalen, app, &app_len)) {
        OUTPUT(sock, STR(help));
        return;
    }

    for(size_t it = 0; apps[it].name; ++it) {
        if (cmp_token(app, app_len, apps[it].name, my_strlen(apps[it].name)) == 0) {
            apps[it].init(&local_ssid, data, datalen);
            OUTPUT(sock, STR("Registered: "), LENSTR(apps[it].name, my_strlen(apps[it].name)));
            return;
        }
    }

    OUTPUT(sock, STR("Unknown app: "), LENSTR(data, datalen));

    return;
}

static const cli_command_t commands[] = {
    { (uint8_t *)"register", cmd_register },
    { (uint8_t *)"connect", cmd_connect },
    { NULL, NULL },
};

static void do_cmd(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    uint8_t cmd[8];
    size_t cmdlen = sizeof(cmd);
    DEBUG(STR("Command: "), LENSTR(data, datalen));

    skipwhite(&data, &datalen);
    if (!get_token(&data, &datalen, cmd, &cmdlen)) {
        OUTPUT(sock, STR("Failed to read command"));
        return;
    }

    for(size_t it = 0; commands[it].name; ++it) {
        if (cmp_token(cmd, cmdlen, commands[it].name, my_strlen(commands[it].name)) == 0) {
            commands[it].command(sock, data, datalen);
            return;
        }
    }

    OUTPUT(sock, STR("Unknown command: ["), LENSTR(cmd, cmdlen), STR("]"));
}

static void cli_data(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    uint8_t pid;
    if (!get_bytes(&data, &datalen, &pid, sizeof(pid))) {
        DEBUG(STR("truncated packet?"));
        return;
    }

    switch (pid) {
        case PID_NOL3:
            do_cmd(sock, data, datalen);
            break;
        default:
            DEBUG(STR("Unexpected PID="), X8(pid));
            break;
    }
}

static void cli_disconnect(dl_socket_t *sock) {
    (void) sock;
    DEBUG(STR("disconnect"));
}

static void cli_connect(dl_socket_t *sock) {
    DEBUG(STR("connected"));
    sock->on_error = cli_error;
    sock->on_data = cli_data;
    sock->on_disconnect = cli_disconnect;
}

static void cli_init(ssid_t *local, const uint8_t data[], size_t datalen) {
    dl_socket_t *listener = dl_find_or_add_listener(local);
    listener->on_connect = cli_connect;
    (void) data;
    (void) datalen;
}

static const uint8_t *init_script[] = {
    (uint8_t *)"register 2E0ITB-1 cli",
    (uint8_t *)"register 2E0ITB-2 caseflip",
    NULL,
};

int main(int argc, char *argv[]) {
    platform_init(argc, argv);
    ax25_init();
    for(size_t it = 0; init_script[it]; ++it) {
        do_cmd(NULL, init_script[it], my_strlen(init_script[it]));
    }
    DEBUG(STR("Running"));
    platform_run();
}

