/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Connection state information for AX.25 Datalink layer.
 */
#ifndef CONNECTION_H
#define CONNECTION_H
#include "platform.h"
#include "buffer.h"
#include "ssid.h"
#include "time.h"

struct dl_socket_t;

typedef enum version_t {
    AX_2_0,
    AX_2_1,
} version_t;

typedef enum conn_state_t {
    STATE_DISCONNECTED = 0,
    STATE_AWAITING_CONNECTION = 1,
    STATE_AWAITING_RELEASE = 2,
    STATE_CONNECTED = 3,
    STATE_TIMER_RECOVERY = 4,
    STATE_AWAITING_CONNECT_2_2 = 5,
} conn_state_t;

typedef struct connection_t {
    uint8_t port;
    ssid_t local;
    ssid_t remote;
    uint8_t snd_state; //< Send State V(S)
    uint8_t ack_state; //< Acknowledgement State V(A)
    uint8_t rcv_state; //< Receive State V(R)
    uint8_t window_size; //< Window size (k)
    uint8_t rc; //< Retry Count
    uint16_t n1; //< Maximum frame size
    uint8_t n2; //< Maximum number of retries permitted
    conn_state_t state;
    version_t version;
    uint8_t modulo;
    bool l3_initiated;
    bool self_busy;
    bool peer_busy;
    bool ack_pending;
    bool srej_enabled;
    bool rej_exception;
    uint8_t srej_exception;
    buffer_t *srej_queue[128];
    packet_t *sent_buffer[128];
    buffer_t *send_queue_head;
    buffer_t *send_queue_tail;
    duration_t srtt; //< smoothed round trip time
    instant_t t1_expiry; //< instant when t1 will expire next, or INSTANT_ZERO if not set
    duration_t t1_remaining; //< time remaining when t1 was last stopped.
    duration_t t1v; //< Next value for T1; initial value is initial value of SRT
    duration_t t2;
    instant_t t3_expiry;
    struct dl_socket_t *socket;
} connection_t;

/** Returns true if low <= x <= high, assuming modulo n */
static inline bool seqno_in_range(uint8_t low, uint8_t x, uint8_t high) {
    if (low <= high) {
        /*      low ...... high     */
        return low <= x && x <= high;
    } else {
        /* .... high       low .... */
        return low <= x || x <= high;
    }
}

static inline bool seqno_add(uint8_t augend, uint8_t addend, uint8_t modulo) { return (augend + addend) % modulo; }

connection_t *conn_find(ssid_t *local, ssid_t *remote, uint8_t port);
connection_t *conn_find_or_create(ssid_t *local, ssid_t *remote, uint8_t port);

static inline conn_state_t conn_get_state(connection_t *connection) { return connection ? connection->state : STATE_DISCONNECTED; }
bool conn_is_extended(connection_t *conn);
void conn_release(connection_t *connection);
void conn_expire_timers(void);
void conn_dequeue(void);
#endif
