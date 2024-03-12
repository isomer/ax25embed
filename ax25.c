#include "ax25.h"
#include "kiss.h"
#include "metric.h"
#include "platform.h"
#include "packet.h"
#include <stdbool.h>

#include <string.h> // for memcmp only.

/* AX.25 packet
 *
 * +--------------------+--------------------+--------------------+--------------------+--------------------+...
 * |      dest addr     |      src addr      |(optional)digipeater|(optional)digipeater|       control      |
 * |D0|D1|D2|D3|D4|D5|D6|S0|S1|S2|S3|S4|S5|S6|A0|A1|A2|A3|A4|A5|A6|A0|A1|A2|A3|A4|A5|A6|C0|C1|C3|C4|C5|C6|C7|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+...
 */

enum {
    MAX_CONN = 10,
    MAX_ADDRESSES = 4,
};

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


enum { SSID_LEN = 7 };

typedef struct ssid_t {
    char ssid[SSID_LEN];
} ssid_t;

const ssid_t my_addr = { .ssid = {'M', '7', 'Q', 'Q', 'Q', ' ', 0 } };

static bool ssid_parse(uint8_t buffer[static SSID_LEN], ssid_t *ssid) {
    /* Verify all the low bits are 0, but ignore the low bit of the ssid */
    if (((buffer[0] | buffer[1] | buffer[2] | buffer[3] | buffer[4] | buffer[5]) & 0x01) != 0x00)
        return false;

    for(size_t i=0; i < SSID_LEN-1; ++i) {
        ssid->ssid[i] = buffer[i] >> 1;
    }

    ssid->ssid[SSID_LEN-1] = (buffer[SSID_LEN-1] & 0b00011110) >> 1;

    return true;
}

static void ssid_debug(const ssid_t *ssid) {
    for(size_t i=0; i<SSID_LEN-1; ++i) {
        debug_putch(ssid->ssid[i]);
    }
    debug_putch(ssid->ssid[SSID_LEN-1]+'0');
    debug_putch('\n');
}

static bool ssid_push(packet_t *packet, const ssid_t *ssid) {
    for(size_t i=0; i<SSID_LEN-1; ++i) {
        packet_push_byte(packet, ssid->ssid[i] << 1);
    }
    packet_push_byte(packet, (ssid->ssid[SSID_LEN-1] << 1) | 0b01100000);

    return false;
}

static bool ssid_cmp(const ssid_t *lhs, const ssid_t *rhs) {
    return memcmp(lhs->ssid, rhs->ssid, sizeof(lhs->ssid));
}

typedef enum state_t {
    STATE_DISCONNECTED,
    STATE_INFO_TXFR,
    STATE_SABM_SENT,
    STATE_DISC_SENT,
    STATE_ACK_WAIT,
    STATE_REMOTE_BUSY,
    STATE_LOCAL_BUSY,
    STATE_REJ_SENT,
} state_t;

typedef struct connection_t {
    ssid_t remote;
    int snd_state; //< Send State V(S)
    int ack_state; //< Acknowledgement State V(A)
    int rcv_state; //< Receive State V(R)
    state_t state;
} connection_t;

static connection_t conntbl[MAX_CONN] = { { .state = STATE_DISCONNECTED, }, };

static connection_t *conn_find(ssid_t *remote) {
    for(size_t i = 0; i < MAX_CONN; ++i) {
        if (conntbl[i].state != STATE_DISCONNECTED && ssid_cmp(&conntbl[i].remote, remote) == 0)
            return &conntbl[i];
    }
    return NULL;
}

static bool conn_is_extended(connection_t *conn) {
    /* TODO */
    return false;
}

static connection_t *conn_find_or_create(ssid_t *remote) {
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

/* Address 0 and 1 have a C bit, address 2 and 3 have an H bit */
static bool get_ch_bit(uint8_t pkt[], size_t pktlen, size_t addrnum) {
    size_t offset = addrnum * SSID_LEN + SSID_LEN-1;
    CHECK(offset < pktlen);
    return (pkt[offset] & 0b10000000) != 0;
}

static const char hexit[] = "0123456789ABCDEF";
static void debug_byte(const uint8_t byte) {
    debug_putch(hexit[byte >> 4]);
    debug_putch(hexit[byte & 0x0F]);
}

static void packet_debug(packet_t *packet) {
    for(size_t i=0; i < packet->len; ++i) {
        debug_byte(packet->buffer[i]);
        debug_putch(' ');
    }
    debug_putch('\n');
}

static packet_t *make_response(ssid_t address[], size_t address_count, uint8_t port) {
    packet_t *packet = packet_allocate();
    if (!packet)
        return NULL;
    packet->port = port;
    ssid_push(packet, &address[ADDR_SRC]); /* Swap src and dst addresses */
    ssid_push(packet, &address[ADDR_DST]);
    packet->buffer[2*SSID_LEN-1] |= 0b10000000; /* Set the "response" bit */

    /* Add digipeater addresses in reverse order */
    for(size_t i=address_count-1; i>=ADDR_DIGI1; i--) {
        ssid_push(packet, &address[i]);
    }

    /* add end of addresses marker */
    packet->buffer[packet->len-1] |= 0b00000001;

    return packet;
}

/* Make an UA (unnumbered acknowledgement) frame */
static packet_t *make_ua(ssid_t address[], size_t address_count, uint8_t port) {
    packet_t *packet = make_response(address, address_count, port);
    if (packet) {
        packet_push_byte(packet, 0b01110011);
    }
    return packet;
}

/* Make an DM (disconnected mode) frame */
static packet_t *make_dm(ssid_t address[], size_t address_count, uint8_t port) {
    packet_t *packet = make_response(address, address_count, port);
    if (packet) {
        packet_push_byte(packet, 0b01100011);
    }
    return packet;
}

static void i_frame(type_t type, uint8_t ns, bool pf, uint8_t nr, uint8_t pkt[], size_t pktlen, ssid_t address[static 2], size_t address_count) {
    (void) type;
    (void) ns;
    (void) pf;
    (void) nr;
    (void) pkt;
    (void) pktlen;
    (void) address;
    (void) address_count;
    metric_inc(METRIC_UNKNOWN_FRAME);
}

static void s_frame(type_t type, uint8_t nr, uint8_t pkt[], size_t pktlen, ssid_t address[static 2], size_t address_count) {
    (void) type;
    (void) nr;
    (void) pkt;
    (void) pktlen;
    (void) address;
    (void) address_count;
    metric_inc(METRIC_UNKNOWN_FRAME);
}

static void test_frame(type_t type, uint8_t pkt[], size_t pktlen, ssid_t address[], size_t address_count) {
    if (type == TYPE_RES) {
        metric_inc(METRIC_TEST_RESPONSE_RECV);
    } else {
        packet_t *packet = make_response(address, address_count, /* TODO: port */ 0);
        if (!packet)
            return;
        packet_push_byte(packet, 0b11101111); /* control byte - test */;
        packet_push(packet, pkt, pktlen); /* copy payload */
        kiss_xmit(packet->port, packet->buffer, packet->len);
        packet_free(&packet);
        metric_inc(METRIC_TEST_COMMAND_RECV);
    }
}

/* Set Async Balanced Mode - aka, initiate a connection */
static void sabm_frame(type_t type, ssid_t address[], size_t address_count) {
    if (type != TYPE_CMD) {
        debug_byte(type);
        metric_inc(METRIC_NOT_COMMAND);
        return;
    }
    connection_t *conn = conn_find_or_create(&address[ADDR_SRC]);
    packet_t *packet = NULL;
    if (conn) {
        packet = make_ua(address, address_count, /* TODO: port */ 0);
        metric_inc(METRIC_SABM_SUCCESS);
        /* Reinitialise the connection */
        conn->remote = address[ADDR_SRC];
        conn->snd_state = 0;
        conn->ack_state = 0;
        conn->rcv_state = 0;
        conn->state = STATE_INFO_TXFR;
    } else {
        packet = make_dm(address, address_count, /* TODO: port */ 0);
        metric_inc(METRIC_SABM_FAIL);
    }
    if (packet) {
        kiss_xmit(packet->port, packet->buffer, packet->len);
        packet_free(&packet);
    }
}

static void u_frame(type_t type, uint8_t control, uint8_t pkt[], size_t pktlen, ssid_t address[], size_t address_count) {
    if (pktlen < 1) {
        metric_inc(METRIC_UNDERRUN);
        return;
    }
    if ((control & 0b11101111) == 0b00101111) {
        sabm_frame(type, address, address_count);
    } else if ((control & 0b11101111) == 0b11100011) {
        test_frame(type, pkt, pktlen, address, address_count);
    } else {
        metric_inc(METRIC_UNKNOWN_FRAME);
    }
}

/* Figure out the active destination address.
 *
 * This is the first unused (H bit not set) digipeater address, or if there are
 * no unused digipeater addresses, this is the destination address.
 */
static size_t get_active_destination(uint8_t pkt[], size_t pktlen, size_t address_count) {
    for(size_t addr = ADDR_DIGI1; addr < address_count; ++addr) {
        if (!get_ch_bit(pkt, pktlen, addr))
            return addr;
    }
    return ADDR_DST;
}

void ax25_recv_ackmode(uint8_t port, uint16_t id, uint8_t pkt[], size_t pktlen) {

    /* Parse 2..4 addresses */
    ssid_t address[MAX_ADDRESSES];
    size_t address_count = 0;
    size_t offset = 0;
    for (;;) {
        if (pktlen - offset < SSID_LEN) {
            metric_inc(METRIC_UNDERRUN);
            return;
        }
        if (!ssid_parse(&pkt[offset], &address[address_count++])) {
            metric_inc(METRIC_INVALID_ADDR);
            return;
        }
        offset += SSID_LEN;

        /* Check for more addresses */
        if ((pkt[offset - 1] & 0x01) == 0b01)
            break;

        /* Packets can only have up to 4 addresses */
        if (address_count >= 4) {
            metric_inc(METRIC_INVALID_ADDR);
            return;
        }
    }
    /* Packets need at least two addresses */
    if (address_count < 2) {
        metric_inc(METRIC_INVALID_ADDR);
        return;
    }

    /* Figure out the which destination address in the packet is current */
    size_t current_dst = get_active_destination(pkt, pktlen, address_count);

    /* Don't accept packets that are not to me. */
    if (ssid_cmp(&address[current_dst], &my_addr) != 0) {
        metric_inc(METRIC_NOT_ME);
        metric_inc_by(METRIC_NOT_ME_BYTES, pktlen);
        return;
    }

    /* Am I being asked to digipeat this packet? */
    if (current_dst != ADDR_DST) {
        metric_inc(METRIC_REFUSED_DIGIPEAT);
        return;
    }

    /* The type of the packet.  For AX.25 Version 2.2, this is either:
     * "CMD" (Command)
     * "RSP" (Response)
     */
    type_t type = (get_ch_bit(pkt, pktlen, ADDR_DST) ? 0b01 : 0b00)
                | (get_ch_bit(pkt, pktlen, ADDR_SRC) ? 0b10 : 0b00);

    /* Figure out which type of frame this is from the control field */
    if (offset >= pktlen) {
        metric_inc(METRIC_UNDERRUN);
        return;
    }

    uint8_t control = pkt[offset];
    bool pf;
    if ((control & 0b00000011) == 0b11) {
        /* unconnected frames always have an 8 bit control field */
        pf = (control & 0b00010000) != 0;
        u_frame(type, control, &pkt[offset+1], pktlen - (offset+1), address, address_count);
    } else {
        /* these are both connected frames */
        connection_t *conn = conn_find(&address[ADDR_SRC]);
        if (!conn) {
            /* TODO */
        }
        offset++;
        uint8_t nr;
        if (conn_is_extended(conn)) {
            uint8_t control2 = pkt[offset++];
            pf = (control2 & 0b00000001) != 0;
            nr = control2 >> 1;
        } else {
            pf = control & 0b00010000;
            nr = control >> 5;
        }
        if ((control & 0b0000001) == 0b0) {
            uint8_t ns;
            if (conn_is_extended(conn)) {
                ns = control >> 1;
            } else {
                ns = (control >> 1) & 0b00001110;
            }
            i_frame(type, ns, pf, nr, &pkt[offset], pktlen - offset, address, address_count);
        }
        else if ((control & 0b00000011) == 0b01) {
            uint8_t s;
            s = (control >> 2) & 0b00001100;
            s_frame(type, nr, &pkt[offset], pktlen - offset, address, address_count);
        } else
            CHECK(!"can't happen");
    }

    return;
}

void ax25_recv(uint8_t port, uint8_t pkt[], size_t pktlen) {
    ax25_recv_ackmode(port, 0, pkt, pktlen);
}

