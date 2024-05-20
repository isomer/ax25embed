/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Capture implementation.
 *
 * Used for monitor mode, as well as pcap.
 */
#include "capture.h"

static capture_t *capture_list = NULL;

void capture_trigger(capture_dir_t dir, const uint8_t *payload, size_t len) {
    for (capture_t *it = capture_list; it; it = it->next) {
        it->capture(dir, payload, len);
    }
}

void capture_register(capture_t *capture) {
    capture->next = capture_list;
    capture_list = capture;
}
