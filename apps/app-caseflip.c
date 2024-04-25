/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Case flip application, swaps case from upper to lower and returns the result.
 *
 * Used for initial echo testing.
 */
#include "ax25_dl.h"
#include "platform.h"
#include "serial.h"

static void caseflip_error(dl_socket_t *sock, ax25_dl_error_t err) {
    (void) sock;
    DEBUG(STR("Got error="), STR(ax25_dl_strerror(err)));
}

static void caseflip_data(dl_socket_t *sock, const uint8_t *data, size_t datalen) {
    DEBUG(STR("Got data, len="), D8(datalen));
    buffer_t *buf = buffer_allocate(data, datalen);
    for(size_t i = 0; i < buf->len; ++i) {
        if ((buf->buffer[i] >= 'A' && buf->buffer[i] <= 'Z') || (buf->buffer[i] >= 'a' && buf->buffer[i] <= 'z')) {
            buf->buffer[i] ^= ('a' - 'A');
        }
    }
    dl_send(sock, buf->buffer, buf->len);
    buffer_free(&buf);
}

static void caseflip_disconnect(dl_socket_t *sock) {
    (void) sock;
    DEBUG(STR("disconnect"));
}

static void caseflip_connect(dl_socket_t *sock) {
    DEBUG(STR("connected"));
    sock->on_error = caseflip_error;
    sock->on_data = caseflip_data;
    sock->on_disconnect = caseflip_disconnect;
}

void caseflip_init(ssid_t *ssid, uint8_t data[], size_t datalen) {
    dl_socket_t *listener = dl_find_or_add_listener(ssid);
    listener->on_connect = caseflip_connect;
    listener->on_data = caseflip_data; /* unconnected data */
    (void) data;
    (void) datalen;
}
