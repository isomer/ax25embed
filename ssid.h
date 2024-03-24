/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * API for dealing with SSIDs, including parsing and encoding.
 */
#ifndef SSID_H
#define SSID_H 1
#include "config.h"
#include "packet.h"
#include <stdint.h>
#include <stdbool.h>

enum {
    SSID_LEN = 7,
};

typedef struct ssid_t {
    char ssid[SSID_LEN];
} ssid_t;

bool ssid_parse(const uint8_t buffer[static SSID_LEN], ssid_t *ssid);
void ssid_debug(const ssid_t *ssid);
bool ssid_push(packet_t *packet, const ssid_t *ssid);
bool ssid_cmp(const ssid_t *lhs, const ssid_t *rhs);
bool ssid_is_mine(const ssid_t *ssid);

#endif
