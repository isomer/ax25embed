#ifndef CONNECTION_H
#define CONNECTION_H
#include "platform.h"
#include "buffer.h"
#include "ssid.h"

enum { SRTT_DEFAULT = MICROS(20000), };

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
    ssid_t remote;
    uint8_t snd_state; //< Send State V(S)
    uint8_t ack_state; //< Acknowledgement State V(A)
    uint8_t rcv_state; //< Receive State V(R)
    uint8_t window_size; //< Window size (k)
    int srtt; //< smoothed RTT
    int t1v;
    uint8_t rc;
    uint8_t n2;
    conn_state_t state;
    version_t version;
    bool l3_initiated;
    bool self_busy;
    bool peer_busy;
    bool ack_pending;
    bool srej_enabled;
    bool rej_exception;
    uint8_t srej_exception;
    buffer_t *srej_queue[128];
} connection_t;

connection_t *conn_find(ssid_t *remote);
connection_t *conn_find_or_create(ssid_t *remote);

static inline conn_state_t conn_get_state(connection_t *connection) { return connection ? connection->state : STATE_DISCONNECTED; }
bool conn_is_extended(connection_t *conn);
void conn_release(connection_t *connection);
#endif
