/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Definitions for AX.25 DataLink layer.
 */
#ifndef AX25_DL_H
#define AX25_DL_H
#include "connection.h"
#include "ssid.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

enum {
    ADDR_DST = 0,
    ADDR_SRC = 1,
    ADDR_DIGI1 = 2,
    ADDR_DIGI2 = 3,
};

typedef enum type_t {
    TYPE_PREV0= 0b00, /* Used in previous versions */
    TYPE_CMD  = 0b01, /* Command */
    TYPE_RES  = 0b10, /* Response */
    TYPE_PREV3= 0b11, /* Used in previous versions */
} type_t;

typedef enum ax25_dl_error_t {
    ERR_A, /* F=1 received but P=1 not outstanding. */
    ERR_B, /* Unexpected DM with F=1 in states CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5). */
    ERR_C, /* Unexpected UA in states CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5). */
    ERR_D, /* UA received without F=1 when SABM or DISC was sent P=1. */
    ERR_E, /* DM received in states CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5). */
    ERR_F, /* Data link reset; i.e., SABM received in state CONNECTED(3), TIMER_RECOVERY(4) or AWAITING_CONNECT_2_2(5). */
    ERR_G, /* (G is not defined): Connection timed out */
    ERR_H, /* (H is not defined)): Connection timed out while disconnecting */
    ERR_I, /* N2 timeouts: unacknowledged data. */
    ERR_J, /* N(r) sequence error. */
    ERR_K, /* (K is not defined): FRMR received */
    ERR_L, /* Control field invalid or not implemented. */
    ERR_M, /* Information field was received in a U- or S-type frame. */
    ERR_N, /* Length of frame incorrect for frame type. */
    ERR_O, /* I frame exceeded maximum allowed length. */
    ERR_P, /* N(s) out of the window. */
    ERR_Q, /* UI response received, or UI command with P=1 received. */
    ERR_R, /* UI frame exceeded maximum allowed length. */
    ERR_S, /* I response received */
    ERR_T, /* N2 Timeouts: no reponse to enquiry */
    ERR_U, /* N2 Timeouts: extended peer busy */
    ERR_V, /* No DL machines available to establish connection */
} ax25_dl_error_t;

typedef enum ax25_dl_event_type_t {
    EV_CTRL_ERROR, /* 0 */
    EV_INFO_NOT_PERMITTED,
    EV_INCORRECT_LENGTH,
    /* Data link commands */
    EV_DL_CONNECT,
    EV_DL_DISCONNECT,
    EV_DL_DATA, /* 5 */
    EV_DL_UNIT_DATA,
    EV_DL_FLOW_ON,
    EV_DL_FLOW_OFF,
    /* Received from Link Multiplexer */
    EV_LM_SIEZE,
    EV_LM_DATA, /* 10 */
    /* Packets */
    EV_UA,
    EV_DM,
    EV_UI,
    EV_DISC,
    EV_SABM, /* 15 */
    EV_SABME,
    EV_TEST,
    EV_I,
    EV_RR,
    EV_RNR, /* 20 */
    EV_REJ,
    EV_SREJ,
    EV_FRMR,
    EV_UNKNOWN_FRAME,
    /* Timers */
    EV_TIMER_EXPIRE_T1, /* 25 */
    EV_TIMER_EXPIRE_T3,
    /* Internal */
    EV_DRAIN_SENDQ,
} ax25_dl_event_type_t;

typedef struct ax25_dl_event_t {
    ax25_dl_event_type_t event;
    uint8_t port;
    size_t address_count;
    type_t type; /* Command / Response / Other */
    uint8_t nr;
    uint8_t ns;
    const uint8_t *info;
    uint8_t info_len;
    bool p;
    bool f;
    ssid_t address[MAX_ADDRESSES];
    connection_t *conn;
    packet_t *packet;
} ax25_dl_event_t;

void ax25_dl_event(ax25_dl_event_t *ev);
const char *ax25_dl_strerror(ax25_dl_error_t err);

typedef struct dl_socket_t {
    connection_t *conn;
    void *userdata;
    void (*on_connect)(struct dl_socket_t *);
    void (*on_error)(struct dl_socket_t *, ax25_dl_error_t err);
    void (*on_data)(struct dl_socket_t *, const uint8_t *data, size_t datalen);
    void (*on_disconnect)(struct dl_socket_t *);
} dl_socket_t;

extern dl_socket_t listen_socket;

/** Create a new connection to remote, from local, on port port */
dl_socket_t *dl_connect(ssid_t *remote, ssid_t *local, uint8_t port);
void dl_send(dl_socket_t *sock, const void *data, size_t datalen);

#endif
