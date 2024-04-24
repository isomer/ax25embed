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
    while(*datalen > 0 && !is_white((*data)[0])) {
        if (*tokenlen > 0) {
            *(token++) = (*data)[0];
            (*tokenlen)--;
        } else
            return false;
        (*data)++;
        (*datalen)--;
    }
    if (*tokenlen > 0) {
        (*token) = '\0'; // Null termination
        return true;
    } else {
        return false;
    }
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


static void cmd_connect(dl_socket_t *sock, const uint8_t data[], size_t datalen) {
    (void) sock;
    (void) data;
    (void) datalen;
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
    const uint8_t *help = (uint8_t*)"register <ssid> <app>\n";
    const uint8_t *unknown_app = (uint8_t*)"Unknown command\n";
    const uint8_t *invalid_ssid = (uint8_t*)"Invalid SSID\n";
    uint8_t ssid[8];
    size_t ssid_len = sizeof(ssid);
    uint8_t app[8];
    size_t app_len = sizeof(app);
    ssid_t local_ssid;
    skipwhite(&data, &datalen);

    if (!get_token(&data, &datalen, ssid, &ssid_len)) {
        dl_send(sock, help, my_strlen(help));
        return;
    }

    if (!ssid_from_string((char *)ssid, &local_ssid)) {
        dl_send(sock, invalid_ssid, my_strlen(invalid_ssid));
        return;
    }

    skipwhite(&data, &datalen);

    if (!get_token(&data, &datalen, app, &app_len)) {
        dl_send(sock, help, my_strlen(help));
        return;
    }

    for(size_t it = 0; apps[it].name; ++it) {
        if (cmp_token(app, app_len, apps[it].name, my_strlen(apps[it].name)) == 0) {
            apps[it].init(&local_ssid, data, datalen);
            return;
        }
    }

    dl_send(sock, unknown_app, my_strlen(unknown_app));

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
    skipwhite(&data, &datalen);
    if (!get_token(&data, &datalen, cmd, &cmdlen))
        return;

    for(size_t it = 0; commands[it].name; ++it) {
        if (cmp_token(cmd, cmdlen, commands[it].name, my_strlen(commands[it].name)) == 0) {
            commands[it].command(sock, data, datalen);
            return;
        }
    }

    static const char unknown_command[] = "\xF0Unknown command\n";
    dl_send(sock, unknown_command, sizeof(unknown_command)-1);
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
    platform_init();
    DEBUG(STR("Initializing"));
    serial_init(argc, argv);
    for(size_t it = 0; init_script[it]; ++it) {
        do_cmd(NULL, init_script[it], my_strlen(init_script[it]));
    }
    //ssid_t local;
    //ssid_from_string("M7QQQ-1", &local);
    //cli_init(&local);
    serial_wait();
}

