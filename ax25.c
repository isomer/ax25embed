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

enum ax25_state_t {
    AX25_READY,
    AX25_RECEIVING,
    AX25_TRANSMITTER_SUPPRESSION,
    AX25_TRANSMITTER_START,
    AX25_TRANSMITTING,
    AX25_DIGIPEATING,
    AX25_RECEIVER_START,
};

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

typedef struct connection_t {
    ssid_t remote;
    ssid_t local;
    int snd_state; //< Send State V(S)
    int ack_state; //< Acknowledgement State V(A)
    int rcv_state; //< Receive State V(R)
    bool in_use;
} connection_t;


static connection_t conntbl[MAX_CONN] = { { .in_use = false, }, };

void ax25_recv(uint8_t port, uint8_t pkt[], size_t pktlen) {
    ax25_recv_ackmode(port, 0, pkt, pktlen);
}

/* Address 0 and 1 have a C bit, address 2 and 3 have an H bit */
static bool get_ch_bit(uint8_t pkt[], size_t pktlen, size_t addrnum) {
    size_t offset = addrnum * SSID_LEN + SSID_LEN-1;
    CHECK(offset < pktlen);
    return (pkt[offset] & 0b10000000) != 0;
}

static const char hexit[] = "0123456789ABCDEF";
static void packet_debug(packet_t *packet) {
    for(size_t i=0; i < packet->len; ++i) {
        debug_putch(hexit[packet->buffer[i] >> 4]);
        debug_putch(hexit[packet->buffer[i] & 0x0F]);
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
    packet->buffer[SSID_LEN-1] |= 0b10000000; /* Set the "response" bit */

    /* Add digipeater addresses in reverse order */
    for(size_t i=address_count-1; i>=ADDR_DIGI1; i--) {
        ssid_push(packet, &address[i]);
    }

    /* add end of addresses marker */
    packet->buffer[packet->len-1] |= 0b00000001;

    return packet;
}

static void i_frame(type_t type, uint8_t control, uint8_t pkt[], size_t pktlen, ssid_t address[static 2], size_t address_count) {
    (void) type;
    (void) control;
    (void) pkt;
    (void) pktlen;
    (void) address;
    (void) address_count;
    metric_inc(METRIC_UNKNOWN_FRAME);
}

static void s_frame(type_t type, uint8_t control, uint8_t pkt[], size_t pktlen, ssid_t address[static 2], size_t address_count) {
    (void) type;
    (void) control;
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

static void u_frame(type_t type, uint8_t control, uint8_t pkt[], size_t pktlen, ssid_t address[], size_t address_count) {
    if ((control & 0b11101111) == 0b11100011) {
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
    type_t type = (get_ch_bit(pkt, pktlen, ADDR_DST) ? 0b10 : 0b00)
                | (get_ch_bit(pkt, pktlen, ADDR_SRC) ? 0b01 : 0b00);

    /* Figure out which type of frame this is from the control field */
    if (offset >= pktlen) {
        metric_inc(METRIC_UNDERRUN);
        return;
    }

    /* TODO: The control frame might be 16bits long */
    uint8_t control = pkt[offset++];
    if ((control & 0b0000001) == 0b0)
        i_frame(type, control, &pkt[offset], pktlen - offset, address, address_count);
    else if ((control & 0b00000011) == 0b01)
        s_frame(type, control, &pkt[offset], pktlen - offset, address, address_count);
    else if ((control & 0b00000011) == 0b11)
        u_frame(type, control, &pkt[offset], pktlen - offset, address, address_count);
    else
        CHECK(!"can't happen");

    return;
}
