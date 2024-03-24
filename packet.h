/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * API for dealing with packet buffers.
 */
#ifndef PACKET_H
#define PACKET_H 1
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum {
    MAX_PACKET_SIZE = 2048,
};

typedef struct packet_t {
    struct packet_t *next;
    bool in_use;
    uint8_t port;
    /* TODO: Replace with buffer_t API */
    size_t len;
    uint8_t buffer[MAX_PACKET_SIZE];
} packet_t;

packet_t *packet_allocate(void);
void packet_free(packet_t **packet);
void packet_push(packet_t *packet, const void *ptr, size_t ptrlen);
void packet_push_byte(packet_t *packet, uint8_t byte);

#endif
