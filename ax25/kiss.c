/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * KISS implementation.
 */
#include "kiss.h"
#include "ax25.h"
#include "capture.h"
#include "config.h"
#include "debug.h"
#include "metric.h"
#include "serial.h"

#undef ACKMODE

enum {
    FEND = 0xC0,
    FESC = 0xDB,
    TFEND = 0xDC,
    TFESC = 0xDD,
};

enum {
    KISS_DATA = 0, /* N bytes data */
    KISS_TXDELAY = 1, /* 1 byte tx delay, in 10ms units */
    KISS_PERSIST = 2, /* 1 byte "persistence" ?? */
    KISS_SLOTTIME = 3, /* 1 byte, slot interval in 10ms units */
    KISS_TXTAIL = 4, /* 1 byte, 10ms time to keep transmitting after frame complete (obsolete) */
    KISS_FULLDUP = 5, /* 1 byte, 0 == half duplex, 1 == full duplex */
    KISS_SETHW = 6, /* implementation defined */
    KISS_FEC = 8, /* FEC? */
    KISS_ACKMODE = 12, /* 2 byte ID, N bytes data */
    KISS_POLL = 14, /* 0 bytes data */
};

static uint8_t buffer[MAX_SERIAL][BUFFER_SIZE];
static size_t buffer_len[MAX_SERIAL] = {0,};

static enum kiss_state_t {
    STATE_WAIT,
    STATE_DATA,
    STATE_ESCAPE,
} kiss_state[MAX_SERIAL] = {STATE_WAIT, };

static uint16_t next_id = 0;

/* Internally we use "port" to refer to which port something came from.
 * But there can be multiple ports on one serial link, or multiple serial links.
 * So we use "serial" to describe which serial link, and "unit" within a serial link.
 */
static uint8_t port_to_serial(uint8_t port) {
    return port >> 4;
}
static uint8_t port_to_unit(uint8_t port) {
    return port & 0xF;
}

static uint8_t serial_unit_to_port(uint8_t serial, uint8_t port) {
    return (serial << 4) | port;
}

static void kiss_recv(uint8_t serial) {
    if (buffer_len[serial] < 1) {
        /* Ignore zero length frames - they're padding */
        return;
    }
    switch (buffer[serial][0] & 0x0F) {
        case KISS_DATA:
            ax25_recv(serial_unit_to_port(serial, buffer[serial][0] >> 4), &buffer[serial][1], buffer_len[serial]-1);
            break;
        case KISS_ACKMODE:
            if (buffer_len[serial] < 3) {
                metric_inc(METRIC_UNDERRUN);
                return;
            }
            /* TODO: range check */
            ax25_recv_ackmode(serial_unit_to_port(serial, buffer[serial][0] >> 4),
                    buffer[serial][1] << 8 | buffer[serial][2],
                    &buffer[serial][3],
                    buffer_len[serial]-3);
            break;
        default:
            DEBUG(STR("Unexpected kiss command from TNC: "), X8(buffer[serial][0]));
            metric_inc(METRIC_UNKNOWN_KISS_COMMAND);
            break;
    }
}

void kiss_recv_byte(uint8_t serial, uint8_t byte) {
    if (serial >= MAX_SERIAL)
        return;
    switch (kiss_state[serial]) {
        case STATE_WAIT:
            if (byte == FEND) {
                kiss_state[serial] = STATE_DATA;
                buffer_len[serial] = 0;
            } else {
                /* Ignore */
            }
            break;
        case STATE_DATA:
            switch (byte) {
                case FEND:
                    kiss_recv(serial);
                    buffer_len[serial] = 0;
                    kiss_state[serial] = STATE_DATA;
                    break;
                case FESC:
                    kiss_state[serial] = STATE_ESCAPE;
                    break;
                default:
                    buffer[serial][buffer_len[serial]++] = byte;
                    if (buffer_len[serial] >= BUFFER_SIZE) {
                        kiss_state[serial] = STATE_WAIT;
                        metric_inc(METRIC_OVERRUN);
                    }
                    break;
            }
            break;
        case STATE_ESCAPE:
            switch (byte) {
                case TFEND:
                    buffer[serial][buffer_len[serial]++] = FEND;
                    if (buffer_len[serial] >= BUFFER_SIZE) {
                        kiss_state[serial] = STATE_WAIT;
                        metric_inc(METRIC_OVERRUN);
                    } else {
                        kiss_state[serial] = STATE_DATA;
                    }
                    break;
                case TFESC:
                    buffer[serial][buffer_len[serial]++] = FESC;
                    if (buffer_len[serial] >= BUFFER_SIZE) {
                        metric_inc(METRIC_OVERRUN);
                        kiss_state[serial] = STATE_WAIT;
                    } else {
                        kiss_state[serial] = STATE_DATA;
                    }
                    break;
                default:
                    /* The spec says "Receipt of any character other than TFESC
                     * or TFEND while in escaped mode is an error; no action is
                     * taken and frame assembly continues.", although I'd be
                     * inclined to discard the frame and transition into
                     * STATE_WAIT */
                    kiss_state[serial] = STATE_DATA;
                    metric_inc(METRIC_BAD_ESCAPE);
                    break;
            }
    }
}

static void kiss_xmit_byte(uint8_t serial, uint8_t byte) {
    switch(byte) {
        case FEND:
            serial_putch(serial, FESC);
            serial_putch(serial, TFEND);
            break;
        case FESC:
            serial_putch(serial, FESC);
            serial_putch(serial, TFESC);
            break;
        default:
            serial_putch(serial, byte);
            break;
    }
}

uint16_t kiss_xmit(uint8_t port, uint8_t *buffer, size_t len) {
    uint16_t id;
    capture_trigger(DIR_OUT, buffer, len);
    do {
        id = next_id++;
    } while (id == 0);

    serial_putch(port_to_serial(port), FEND);
#ifdef ACKMODE
    kiss_xmit_byte(port_to_serial(port), (port_to_unit(port) << 4) | KISS_ACKMODE);
    kiss_xmit_byte(port_to_serial(port), id >> 8);
    kiss_xmit_byte(port_to_serial(port), id & 0xFF);
#else
    kiss_xmit_byte(port_to_serial(port), (port_to_unit(port) << 4) | KISS_DATA);
#endif
    for (size_t i = 0; i < len; ++i) {
        kiss_xmit_byte(port_to_serial(port), buffer[i]);
    }
    serial_putch(port_to_serial(port), FEND);
    metric_inc(METRIC_KISS_XMIT);
    metric_inc_by(METRIC_KISS_XMIT_BYTES, len);

    return id;
}

void kiss_set_txdelay(uint8_t port, uint8_t delay) {
    kiss_xmit_byte(port_to_serial(port), FEND);
    kiss_xmit_byte(port_to_serial(port), (port_to_unit(port) << 4) | KISS_TXDELAY);
    kiss_xmit_byte(port_to_serial(port), delay);
    kiss_xmit_byte(port_to_serial(port), FEND);
    DEBUG(STR("set txdelay"));
}

void kiss_set_slottime(uint8_t port, uint8_t delay) {
    kiss_xmit_byte(port_to_serial(port), FEND);
    kiss_xmit_byte(port_to_serial(port), (port_to_unit(port) << 4) | KISS_SLOTTIME);
    kiss_xmit_byte(port_to_serial(port), delay);
    kiss_xmit_byte(port_to_serial(port), FEND);
    DEBUG(STR("set slottime"));
}

void kiss_set_duplex(uint8_t port, bool full_duplex) {
    kiss_xmit_byte(port_to_serial(port), FEND);
    kiss_xmit_byte(port_to_serial(port), (port_to_unit(port) << 4) | KISS_FULLDUP);
    kiss_xmit_byte(port_to_serial(port), full_duplex ? 1 : 0);
    kiss_xmit_byte(port_to_serial(port), FEND);
    DEBUG(STR("set duplex"));
}
