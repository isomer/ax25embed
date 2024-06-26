/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Generic buffer management
 */
#ifndef BUFFER_H
#define BUFFER_H 1
#include <stdbool.h>
#include "packet.h" // for MAX_PACKET_SIZE

typedef struct buffer_t {
    bool in_use;
    uint8_t buffer[MAX_PACKET_SIZE];
    size_t len;
    struct buffer_t *next;
} buffer_t;

buffer_t *buffer_allocate(const uint8_t *src, size_t len);
buffer_t *buffer_allocate_with_size(size_t len);
void buffer_free(buffer_t **buffer);

#endif
