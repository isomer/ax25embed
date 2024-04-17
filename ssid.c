/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Implementation of parsing/serialising/comparing SSIDs.
 *
 */

#include "ax25_dl.h"
#include "config.h"
#include "debug.h"
#include "platform.h"
#include <string.h> // For memcmp

static ssid_t ssid_my_addr;

bool ssid_from_string(const char *str, ssid_t *ssid) {
    ssid->ssid[SSID_LEN-1] = '\x00';

    for(size_t i = 0; i < SSID_LEN-1; ++i) {
        switch (str[i]) {
            case '-':
                ssid->ssid[SSID_LEN-1] = str[i+1] - '0';
                /* fall through */
            case '\0':
                /* pad string with NULs */
                for( ; i < SSID_LEN-1; ++i) {
                    ssid->ssid[i] = ' ';
                }
                return true;
            default:
                ssid->ssid[i] = str[i];
                break;
        }
    }
    return true;
}

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

void ssid_set_local(const ssid_t *ssid) {
    ssid_my_addr = *ssid;
}

static void debug_internal_ssid(struct debug_t *self) {
    ssid_t *ssid = self->ptr;
    for(size_t i = 0; i < SSID_LEN-1; ++i) {
        if (ssid->ssid[i] != ' ')
            debug_putch(ssid->ssid[i]);
        else
            break;
    }
    if (ssid->ssid[SSID_LEN-1] != '\0') {
        debug_putch('-');
        debug_putch(ssid->ssid[SSID_LEN-1] + '0');
    }
}

struct debug_t debug_ssid(ssid_t *ssid) {
    return (struct debug_t) { .fmt = debug_internal_ssid, .ptr = ssid };
}

