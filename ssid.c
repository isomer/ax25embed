/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Implementation of parsing/serialising/comparing SSIDs.
 *
 */

#include "ax25_dl.h"
#include "config.h"
#include "platform.h"
#include <string.h> // For memcmp

static const ssid_t ssid_my_addr = { .ssid = {
    CALLSIGN[0],
    CALLSIGN[1],
    CALLSIGN[2],
    CALLSIGN[3],
    CALLSIGN[4],
    CALLSIGN[5],
    SSID,
} };

bool ssid_parse(const uint8_t buffer[static SSID_LEN], ssid_t *ssid) {
    /* Verify all the low bits are 0, but ignore the low bit of the ssid */
    if (((buffer[0] | buffer[1] | buffer[2] | buffer[3] | buffer[4] | buffer[5]) & 0x01) != 0x00)
        return false;

    for(size_t i=0; i < SSID_LEN-1; ++i) {
        ssid->ssid[i] = buffer[i] >> 1;
    }

    ssid->ssid[SSID_LEN-1] = (buffer[SSID_LEN-1] & 0b00011110) >> 1;

    return true;
}

void ssid_debug(const ssid_t *ssid) {
    for(size_t i=0; i<SSID_LEN-1; ++i) {
        debug_putch(ssid->ssid[i]);
    }
    debug_putch(ssid->ssid[SSID_LEN-1]+'0');
    debug_putch('\n');
}

bool ssid_push(packet_t *packet, const ssid_t *ssid) {
    for(size_t i=0; i<SSID_LEN-1; ++i) {
        packet_push_byte(packet, ssid->ssid[i] << 1);
    }
    packet_push_byte(packet, (ssid->ssid[SSID_LEN-1] << 1) | 0b01100000);

    return false;
}

bool ssid_cmp(const ssid_t *lhs, const ssid_t *rhs) {
    return memcmp(lhs->ssid, rhs->ssid, sizeof(lhs->ssid));
}

/* Returns true if the frame is destined to this station */
bool ssid_is_mine(const ssid_t *ssid) {
    return ssid_cmp(ssid, &ssid_my_addr) == 0;
}

