/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * AX.25 Framing code.
 */
#include "ax25.h"
#include "ax25_dl.h"
#include "connection.h"
#include "debug.h"
#include "kiss.h"
#include "metric.h"
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
static bool get_ch_bit(const uint8_t pkt[], size_t pktlen, size_t addrnum) {
    size_t offset = addrnum * SSID_LEN + SSID_LEN-1;
    CHECK(offset < pktlen);
    return (pkt[offset] & 0b10000000) != 0;
}

/* Figure out the active destination address.
 *
 * This is the first unused (H bit not set) digipeater address, or if there are
 * no unused digipeater addresses, this is the destination address.
 */
static size_t get_active_destination(const uint8_t pkt[], size_t pktlen, size_t address_count) {
    for(size_t addr = ADDR_DIGI1; addr < address_count; ++addr) {
        if (!get_ch_bit(pkt, pktlen, addr))
            return addr;
    }
    return ADDR_DST;
}

/* Look at the next byte in the packet, without changing the offset.
 * returns false if the packet is truncated.
 */
static bool pkt_peek(const uint8_t pkt[], size_t pktlen, const size_t *offset, uint8_t *byte) {
    if (*offset < pktlen) {
        *byte = pkt[*offset];
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

void ax25_recv_ackmode(uint8_t port, uint16_t id, const uint8_t pkt[], size_t pktlen) {
    (void) id;
    ax25_dl_event_t ev;

    ev.port = port;
    ev.address_count = 0;
    ev.conn = NULL;

    /* Parse 2..4 addresses */
    size_t offset = 0;
    for (;;) {
        if (pktlen - offset < SSID_LEN) {
            DEBUG(STR("addr underrun"));
            metric_inc(METRIC_UNDERRUN);
            return;
        }
        if (!ssid_parse(&pkt[offset], &ev.address[ev.address_count++])) {
            DEBUG(STR("invalid addr"));
            metric_inc(METRIC_INVALID_ADDR);
            return;
        }
        offset += SSID_LEN;

        /* Check for more addresses */
        if ((pkt[offset - 1] & 0x01) == 0b01)
            break;

        /* Packets can only have up to MAX_ADDRESSES addresses */
        if (ev.address_count >= MAX_ADDRESSES) {
            DEBUG(STR("too many addresses"));
            metric_inc(METRIC_INVALID_ADDR);
            return;
        }
    }
    /* Packets need at least two addresses */
    if (ev.address_count < 2) {
        DEBUG(STR("too few addresses"));
        metric_inc(METRIC_INVALID_ADDR);
        return;
    }

    /* Figure out the which destination address in the packet is current */
    size_t current_dst = get_active_destination(pkt, pktlen, ev.address_count);

    /* Don't accept packets that are not to me. */
    if (!ssid_is_mine(&ev.address[current_dst])) {
        DEBUG(STR("not to me"));
        metric_inc(METRIC_NOT_ME);
        metric_inc_by(METRIC_NOT_ME_BYTES, pktlen);
        return;
    }

    /* Am I being asked to digipeat this packet? */
    if (current_dst != ADDR_DST) {
        DEBUG(STR("refused digipeat"));
        metric_inc(METRIC_REFUSED_DIGIPEAT);
        return;
    }

    /* The type of the packet.  For AX.25 Version 2.2, this is either:
     * "CMD" (Command)
     * "RSP" (Response)
     */
    ev.type = (get_ch_bit(pkt, pktlen, ADDR_DST) ? 0b01 : 0b00)
            | (get_ch_bit(pkt, pktlen, ADDR_SRC) ? 0b10 : 0b00);

    ev.conn = conn_find(&ev.address[ADDR_DST], &ev.address[ADDR_SRC], ev.port);

    uint8_t control;
    if (!pkt_peek(pkt, pktlen, &offset, &control)) {
        metric_inc(METRIC_UNDERRUN);
        return;
    }
    if (is_u_frame8(control)) {
        /* unconnected frames always have an 8 bit control field */
        if (ev.type == TYPE_CMD) {
            ev.p = (control & CONTROL8_PF_MASK) != 0;
            ev.f = 0;
        }
        else {
            ev.f = (control & CONTROL8_PF_MASK) != 0;
            ev.p = 0;
        }
        switch (control & 0b11101100) {
            case 0b00101100: ev.event = EV_SABM; break;
            case 0b01101100: ev.event = EV_SABME; break;
            case 0b01000000: ev.event = EV_DISC; break;
            case 0b00001100: ev.event = EV_DM; break;
            case 0b01100000: ev.event = EV_UA; break;
            case 0b10000100: ev.event = EV_FRMR; break;
            case 0b00000000: ev.event = EV_UI; break;
            case 0b10101100: ev.event = EV_XID; break;
            case 0b11100000: ev.event = EV_TEST; break;
            default:
                             DEBUG(STR("Unknown uframe, control="), X8(control));
                             ev.event = EV_UNKNOWN_FRAME;
                             break;
        }
    } else {
        /* both S and I frames are connected, and we need to know the
         * connection to know if control is 8 or 16 bits long */
        bool extended = false;
        extended = conn_is_extended(ev.conn);
        offset++;
        if (extended) {
            uint16_t control16 = (pkt[offset++] << 8) | control;
            if (ev.type == TYPE_CMD) {
                ev.p = (control16 & CONTROL16_PF_MASK) != 0;
                ev.f = 0;
            } else {
                ev.f = (control16 & CONTROL16_PF_MASK) != 0;
                ev.p = 0;
            }

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
                    default:
                                     ev.event = EV_UNKNOWN_FRAME;
                                     DEBUG(STR("Unknown sframe, control="), X8(control16));
                                     break;
                }
            }
        } else {
            if (ev.type == TYPE_CMD) {
                ev.p = (control & CONTROL8_PF_MASK) != 0;
                ev.f = 0;
            } else {
                ev.f = (control & CONTROL8_PF_MASK) != 0;
                ev.p = 0;
            }
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
                    default:
                                     ev.event = EV_UNKNOWN_FRAME;
                                     DEBUG(STR("Unknown sframe, control="), X8(control));
                                     break;
                }
            }
        }
    }
    ev.info = &pkt[offset];
    ev.info_len = pktlen - offset;

    ax25_dl_event(&ev);

    return;
}

void ax25_recv(uint8_t port, const uint8_t pkt[], size_t pktlen) {
    ax25_recv_ackmode(port, 0, pkt, pktlen);
}

