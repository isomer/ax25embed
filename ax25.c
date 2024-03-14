#include "ax25.h"
#include "ax25_dl.h"
#include "connection.h"
#include "kiss.h"
#include "metric.h"
#include "platform.h"
#include "packet.h"
#include "ssid.h"

/* AX.25 packet
 *
 * +--------------------+--------------------+--------------------+--------------------+--------------------+...
 * |      dest addr     |      src addr      |(optional)digipeater|(optional)digipeater|       control      |
 * |D0|D1|D2|D3|D4|D5|D6|S0|S1|S2|S3|S4|S5|S6|A0|A1|A2|A3|A4|A5|A6|A0|A1|A2|A3|A4|A5|A6|C0|C1|C3|C4|C5|C6|C7|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+...
 */

enum {
    CONTROL8_I_MASK =  0b00000001, /* Mask to recognise I frames */
    CONTROL8_SU_MASK = 0b00000011, /* Mask to recognise S or U frames */
    CONTROL8_NS_MASK = 0b00001110, /* Mask for NS field in I frames */
    CONTROL8_S_MASK  = 0b00001100, /* Mask for S field in S frames */
    CONTROL8_PF_MASK = 0b00010000, /* Mask for P/F bit */
    CONTROL8_NR_MASK = 0b11100000, /* Mask for NR field in I/S frames */
    CONTROL8_M_MASK  = 0b11101100, /* Mask for M field in U frames */

    CONTROL16_NR_MASK = 0b1111111000000000,
    CONTROL16_PF_MASK = 0b0000000100000000,
    CONTROL16_NS_MASK = 0b0000000011111110,
    CONTROL16_S_MASK  = 0b0000000000001100,
    CONTROL16_I_MASK  = 0b0000000000000001,
};

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

/* Look at the next byte in the packet, without changing the offset.
 * returns false if the packet is truncated.
 */
static bool pkt_peek(uint8_t pkt[], size_t pktlen, const size_t *offset, uint8_t *byte) {
    if (*offset < pktlen) {
        *byte = pkt[*offset];
        return true;
    } else {
        return false;
    }
}

/* returns the next byte in the packet, incrementing offset.
 * returns false if the packet is truncated
 */
static bool pkt_get(uint8_t pkt[], size_t pktlen, uint8_t *offset, uint8_t *byte) {
    if (*offset < pktlen) {
        *byte = pkt[(*offset)++];
        return true;
    } else {
        return false;
    }
}

static bool is_i_frame8(uint8_t control) {
    return (control & CONTROL8_I_MASK) == 0b0;
}

static bool is_i_frame16(uint16_t control) {
    return (control & CONTROL16_I_MASK) == 0b0;
}

static bool is_u_frame8(uint8_t control) {
    return (control & CONTROL8_SU_MASK) == 0b11;
}

void ax25_recv_ackmode(uint8_t port, uint16_t id, uint8_t pkt[], size_t pktlen) {
    ax25_dl_event_t ev;

    ev.port = port;

    /* Parse 2..4 addresses */
    size_t offset = 0;
    for (;;) {
        if (pktlen - offset < SSID_LEN) {
            metric_inc(METRIC_UNDERRUN);
            return;
        }
        if (!ssid_parse(&pkt[offset], &ev.address[ev.address_count++])) {
            metric_inc(METRIC_INVALID_ADDR);
            return;
        }
        offset += SSID_LEN;

        /* Check for more addresses */
        if ((pkt[offset - 1] & 0x01) == 0b01)
            break;

        /* Packets can only have up to MAX_ADDRESSES addresses */
        if (ev.address_count >= MAX_ADDRESSES) {
            metric_inc(METRIC_INVALID_ADDR);
            return;
        }
    }
    /* Packets need at least two addresses */
    if (ev.address_count < 2) {
        metric_inc(METRIC_INVALID_ADDR);
        return;
    }

    /* Figure out the which destination address in the packet is current */
    size_t current_dst = get_active_destination(pkt, pktlen, ev.address_count);

    /* Don't accept packets that are not to me. */
    if (!ssid_is_mine(&ev.address[current_dst])) {
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
    ev.type = (get_ch_bit(pkt, pktlen, ADDR_DST) ? 0b01 : 0b00)
            | (get_ch_bit(pkt, pktlen, ADDR_SRC) ? 0b10 : 0b00);

    /* Figure out which type of frame this is from the control field */
    if (offset >= pktlen) {
        metric_inc(METRIC_UNDERRUN);
        return;
    }

    uint8_t control;
    if (!pkt_peek(pkt, pktlen, &offset, &control)) {
        metric_inc(METRIC_UNDERRUN);
        return;
    }
    if (is_u_frame8(control)) {
        /* unconnected frames always have an 8 bit control field */
        ev.pf = (control & CONTROL8_PF_MASK) != 0;
        switch (control & 0b11101100) {
            case 0b00101100: ev.event = EV_SABM; break;
            case 0b11100000: ev.event = EV_TEST; break;
            default:
                             ev.event = EV_UNKNOWN_FRAME; break;
        }
    } else {
        /* both S and I frames are connected, and we need to know the
         * connection to know if control is 8 or 16 bits long */
        bool extended = false;
        connection_t *conn = conn_find(&ev.address[ADDR_SRC]);
        extended = conn_is_extended(conn);
        offset++;
        if (extended) {
            uint16_t control16 = (pkt[offset++] << 8) | control;
            ev.pf = (control16 & CONTROL16_PF_MASK) != 0;
            ev.nr = (control16 & CONTROL16_NR_MASK) >> 9;
            if (is_i_frame16(control16)) {
                ev.ns = (control16 & CONTROL16_NS_MASK) >> 1;
                ev.event = EV_I;
            } else {
                /* S Frame */
                switch (control16 & CONTROL16_S_MASK) {
                    case 0b00000000: ev.event = EV_RR; break;
                    case 0b00000100: ev.event = EV_RNR; break;
                    case 0b00001000: ev.event = EV_REJ; break;
                    case 0b00001100: ev.event = EV_SREJ; break;
                    default: ev.event = EV_UNKNOWN_FRAME; break;
                }
            }
        } else {
            ev.pf = control & CONTROL8_PF_MASK;
            ev.nr = control >> 5;
            if (is_i_frame8(control)) {
                ev.ns = (control & CONTROL8_NS_MASK) >> 1;
                ev.event = EV_I;
            } else {
                /* S Frame */
                switch (control & CONTROL8_S_MASK) {
                    case 0b00000000: ev.event = EV_RR; break;
                    case 0b00000100: ev.event = EV_RNR; break;
                    case 0b00001000: ev.event = EV_REJ; break;
                    case 0b00001100: ev.event = EV_SREJ; break;
                    default: ev.event = EV_UNKNOWN_FRAME; break;
                }
            }
        }
    }
    ev.info = &pkt[offset];
    ev.info_len = pktlen - offset;

    ax25_dl_event(&ev);

    return;
}

void ax25_recv(uint8_t port, uint8_t pkt[], size_t pktlen) {
    ax25_recv_ackmode(port, 0, pkt, pktlen);
}

