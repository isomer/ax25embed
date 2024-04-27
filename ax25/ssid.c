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

bool ssid_from_string(const char *str, ssid_t *ssid) {
    ssid->ssid[SSID_LEN-1] = '\x00'; /* By default it's -0 */

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
                if (str[i] >= 'a' && str[i] <= 'z') {
                    /* upper case */
                    ssid->ssid[i] = str[i] - 'a' + 'A';
                } else {
                    ssid->ssid[i] = str[i];
                }
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

static bool format_internal_ssid(char **buffer, size_t *buffer_len, struct format_t *self) {
#define RETURN_IF_FALSE(x) do { if (!(x)) return false; } while(0)
    const ssid_t *ssid = self->ptr;
    for(size_t i = 0; i < SSID_LEN-1; ++i) {
        if (ssid->ssid[i] != ' ')
            RETURN_IF_FALSE(format_putch(buffer, buffer_len, ssid->ssid[i]));
        else
            break;
    }
    if (ssid->ssid[SSID_LEN-1] != '\0') {
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, '-'));
        RETURN_IF_FALSE(format_putch(buffer, buffer_len, ssid->ssid[SSID_LEN-1] + '0'));
    }
#undef RETURN_IF_FALSE
    return true;
}

struct format_t format_ssid(ssid_t *ssid) {
    return (struct format_t) { .fmt = format_internal_ssid, .ptr = ssid };
}

