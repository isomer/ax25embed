/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Capture implementation.
 *
 * Used for monitor mode, as well as pcap.
 */
#ifndef CAPTURE_H
#define CAPTURE_H
#include <stddef.h>
#include <stdint.h>

typedef enum capture_dir_t {
    /** Packet received by this node and processed by this node */
    DIR_IN,
    /** Packet sent by this node */
    DIR_OUT,
    /** Packet received by this node, but not processed (for some other node) */
    DIR_OTHER,
} capture_dir_t;

typedef struct capture_t {
    struct capture_t *next;
    void (*capture)(capture_dir_t dir, const uint8_t *payload, size_t len);
} capture_t;

void capture_trigger(capture_dir_t dir, const uint8_t *payload, size_t len);
void capture_register(capture_t *capture);

#endif
