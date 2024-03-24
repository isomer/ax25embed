/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Implementation of buffer pool.
 */
#include "buffer.h"
#include "metric.h"
#include "platform.h"
#include <string.h> // for memcpy

enum { MAX_BUFFERS = 20, };

/* TODO: Instead of always using the largest buffer size, we should have
 * multiple bufferpools, eg: one at 2048 bytes, one at 512 bytes, one at 128
 * bytes.  Try and allocate from the smallest buffer that will fit, otherwise
 * try the next largest etc.
 */
static buffer_t bufferpool[MAX_BUFFERS] = { { .in_use = false }, };

buffer_t *buffer_allocate(const uint8_t *src, size_t len) {
    CHECK(len < MAX_PACKET_SIZE);
    for(int i=0; i < MAX_BUFFERS; ++i) {
        if (!bufferpool[i].in_use) {
            bufferpool[i].in_use = true;
            bufferpool[i].len = len;
            bufferpool[i].next = NULL;
            memcpy(bufferpool[i].buffer, src, len);
            metric_inc(METRIC_BUFFER_ALLOC_SUCCESS);
            return &bufferpool[i];
        }
    }
    metric_inc(METRIC_BUFFER_ALLOC_FAIL);
    return NULL;
}

void buffer_free(buffer_t *buffer) {
    metric_inc(METRIC_BUFFER_FREE);
    buffer->in_use = false;
    buffer->len = 0;
}
