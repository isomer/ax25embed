#include "connection.h"
#include "metric.h"

enum { MAX_CONN = 8 };

static connection_t conntbl[MAX_CONN] = { { .state = STATE_DISCONNECTED, }, };

bool conn_is_extended(connection_t *conn) {
    if (!conn)
        return false;
    /* TODO */
    return false;
}

connection_t *conn_find(ssid_t *remote) {
    for(size_t i = 0; i < MAX_CONN; ++i) {
        if (conntbl[i].state != STATE_DISCONNECTED && ssid_cmp(&conntbl[i].remote, remote) == 0)
            return &conntbl[i];
    }
    return NULL;
}

connection_t *conn_find_or_create(ssid_t *remote) {
    connection_t *conn = NULL;
    for(size_t i = 0; i < MAX_CONN; ++i) {
        if (conntbl[i].state != STATE_DISCONNECTED) {
            /* Is this in use by the address we are looking for? */
            if (ssid_cmp(&conntbl[i].remote, remote) == 0) {
                return &conntbl[i];
            }
        } else {
            /* Remember we've found a free conntbl entry in case we need it */
            conn = &conntbl[i];
        }
    }
    if (conn) {
        /* Initialise the structure */
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
    /* TODO */
}
