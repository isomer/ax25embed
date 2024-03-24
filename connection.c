/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Definitions for connection table.
 */
#include "connection.h"
#include "ax25_dl.h"
#include "metric.h"

enum { MAX_CONN = 8 };

static connection_t conntbl[MAX_CONN] = { { .state = STATE_DISCONNECTED, }, };

bool conn_is_extended(connection_t *conn) {
    if (!conn)
        return false;
    return conn->version == AX_2_1;
}

connection_t *conn_find(ssid_t *local, ssid_t *remote, uint8_t port) {
    for(size_t i = 0; i < MAX_CONN; ++i) {
        if (conntbl[i].state != STATE_DISCONNECTED
                && conntbl[i].port == port
                && ssid_cmp(&conntbl[i].local, local) == 0
                && ssid_cmp(&conntbl[i].remote, remote) == 0)
            return &conntbl[i];
    }
    return NULL;
}

connection_t *conn_find_or_create(ssid_t *local, ssid_t *remote, uint8_t port) {
    connection_t *conn = NULL;
    for(size_t i = 0; i < MAX_CONN; ++i) {
        if (conntbl[i].state != STATE_DISCONNECTED) {
            /* Is this in use by the address we are looking for? */
            if (conntbl[i].port == port
                    && ssid_cmp(&conntbl[i].local, local) == 0
                    && ssid_cmp(&conntbl[i].remote, remote) == 0)
                return &conntbl[i];
        } else {
            /* Remember we've found a free conntbl entry in case we need it */
            conn = &conntbl[i];
        }
    }
    if (conn) {
        /* Initialise the structure */
        /* TODO: should probably initialise more of this */
        conn->port = port;
        conn->local = *local;
        conn->remote = *remote;
        conn->snd_state = 0;
        conn->ack_state = 0;
        conn->rcv_state = 0;
        conn->state = STATE_DISCONNECTED;
    } else {
        /* Record that there were no more available connctions */
        metric_inc(METRIC_NO_CONNS);
    }
    return conn;
}

void conn_release(connection_t *connection) {
    CHECK(connection->state == STATE_DISCONNECTED);
    CHECK(instant_cmp(connection->t1_expiry, INSTANT_ZERO) == 0);
    CHECK(instant_cmp(connection->t3_expiry, INSTANT_ZERO) == 0);
}

void conn_expire_timers(void) {
    instant_t now = instant_now();
    for(size_t i = 0; i < MAX_CONN; ++i) {

        if (conntbl[i].state == STATE_DISCONNECTED)
            continue;

        if (instant_cmp(conntbl[i].t1_expiry, INSTANT_ZERO) != 0 &&
                instant_cmp(conntbl[i].t1_expiry, now) <= 0) {
            ax25_dl_event_t ev;
            conntbl[i].t1_expiry = INSTANT_ZERO;
            ev.event = EV_TIMER_EXPIRE_T1;
            ev.conn = &conntbl[i];
            ev.address_count = 0;
            ax25_dl_event(&ev);
        }

        if (instant_cmp(conntbl[i].t3_expiry, INSTANT_ZERO) != 0 &&
                instant_cmp(conntbl[i].t3_expiry, now) <= 0) {
            ax25_dl_event_t ev;
            conntbl[i].t3_expiry = INSTANT_ZERO;
            ev.event = EV_TIMER_EXPIRE_T3;
            ev.conn = &conntbl[i];
            ev.address_count = 0;
            ax25_dl_event(&ev);
        }
    }
}

void conn_dequeue(void) {
    for(size_t i = 0; i < MAX_CONN; ++i) {
        if (conntbl[i].state == STATE_DISCONNECTED)
            continue;

        if (!conntbl[i].peer_busy && conntbl[i].send_queue_head) {
            ax25_dl_event_t ev;
            ev.conn = &conntbl[i];
            ev.event = EV_DRAIN_SENDQ;
            ev.address_count = 0;
            ax25_dl_event(&ev);
        }
    }
}
