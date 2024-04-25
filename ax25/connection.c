/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Definitions for connection table.
 */
#include "connection.h"
#include "config.h"
#include "ax25_dl.h"
#include "metric.h"
#include "platform.h"

static connection_t conntbl[MAX_CONN] = { { .state = STATE_DISCONNECTED, }, };

bool conn_is_extended(connection_t *conn) {
    if (!conn)
        return false;
    return conn->version == AX_2_2;
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
        conn->window_size = 0;
        conn->t1_expiry = INSTANT_ZERO;
        conn->t2_expiry = INSTANT_ZERO;
        conn->t3_expiry = INSTANT_ZERO;
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

static duration_t conn_expire_timers(void) {
    instant_t now;
    instant_t next;
    bool triggered;
    do {
        triggered = false;
        now = instant_now();
        next = instant_add(now, duration_seconds(3600));

        for(size_t i = 0; i < MAX_CONN; ++i) {

            if (conntbl[i].state == STATE_DISCONNECTED)
                continue;

            if (instant_cmp(conntbl[i].t1_expiry, INSTANT_ZERO) != 0) {
                if (instant_cmp(conntbl[i].t1_expiry, now) <= 0) {
                    ax25_dl_event_t ev;
                    conntbl[i].t1_expiry = INSTANT_ZERO;
                    ev.event = EV_TIMER_EXPIRE_T1;
                    ev.conn = &conntbl[i];
                    ev.address_count = 0;
                    ax25_dl_event(&ev);
                    triggered = true;
                } else
                    next = instant_min(next, conntbl[i].t1_expiry);
            }

            if (instant_cmp(conntbl[i].t2_expiry, INSTANT_ZERO) != 0) {
                if (instant_cmp(conntbl[i].t2_expiry, now) <= 0) {
                    ax25_dl_event_t ev;
                    conntbl[i].t2_expiry = INSTANT_ZERO;
                    ev.event = EV_TIMER_EXPIRE_T2;
                    ev.conn = &conntbl[i];
                    ev.address_count = 0;
                    ax25_dl_event(&ev);
                    triggered = true;
                } else
                    next = instant_min(next, conntbl[i].t2_expiry);
            }

            if (instant_cmp(conntbl[i].t3_expiry, INSTANT_ZERO) != 0) {
                if (instant_cmp(conntbl[i].t3_expiry, now) <= 0) {
                    ax25_dl_event_t ev;
                    conntbl[i].t3_expiry = INSTANT_ZERO;
                    ev.event = EV_TIMER_EXPIRE_T3;
                    ev.conn = &conntbl[i];
                    ev.address_count = 0;
                    ax25_dl_event(&ev);
                    triggered = true;
                } else
                    next = instant_min(next, conntbl[i].t3_expiry);
            }
        }
    } while (triggered);

    return instant_sub(next, now);
}

static duration_t conn_dequeue(void) {
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

    return duration_seconds(3600);
}

static ticker_t conn_dequeue_ticker = {
    .next = NULL,
    .tick = conn_dequeue,
};

static ticker_t conn_expire_ticker = {
    .next = NULL,
    .tick = conn_expire_timers,
};

void ax25_init(void) {
    register_ticker(&conn_expire_ticker);
    register_ticker(&conn_dequeue_ticker);
}
