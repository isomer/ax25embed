/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Connects to an SSID and says "Hello World".
 *
 * Used for testing outbound connections.
 */
#include "ax25_dl.h"
#include "platform.h"
#include "serial.h"

static const char greet_message[] = "Hello World";
static const uint8_t greet_port = 0;

static void greet_error(dl_socket_t *sock, ax25_dl_error_t err) {
    (void) sock;
    DEBUG(STR("Got error="), STR(ax25_dl_strerror(err)));
}

static void greet_data(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    (void) sock;
    (void) data;
    DEBUG(STR("Got data, len="), D8(datalen), STR(" Buffer="), BUF(data, datalen));
}

static void greet_disconnect(dl_socket_t *sock) {
    (void) sock;
    DEBUG(STR("disconnect"));
}

static void greet_connect(dl_socket_t *sock) {
    DEBUG(STR("connected"));
    dl_send(sock, greet_message, sizeof(greet_message));
}

static void greet_init(ssid_t *local, ssid_t *remote) {
    dl_socket_t *sock = dl_connect(remote, local, greet_port);
    sock->on_connect = greet_connect;
    sock->on_error = greet_error;
    sock->on_data = greet_data;
    sock->on_disconnect = greet_disconnect;
}

int main(int argc, char *argv[]) {
    ssid_t local;
    ssid_t remote;
    platform_init();
    DEBUG(STR("Initializing greet"));
    if (!ssid_from_string("M7QQQ-2", &local))
        panic("can't set local callsign");
    if (!ssid_from_string("M7QQQ-1", &remote))
        panic("can't set remote callsign");
    serial_init(argc, argv);
    greet_init(&local, &remote);
    DEBUG(STR("Running"));
    serial_wait();
}
