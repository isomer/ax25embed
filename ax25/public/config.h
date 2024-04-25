/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Configuration knobs
 */
#ifndef CONFIG_H
#define CONFIG_H

enum {
    MAX_SOCKETS = 16,
    T3_DURATION_MINUTES = 15,
    MAX_BUFFERS = 20,
    MAX_CONN = 16,
    BUFFER_SIZE = 2048,
    MAX_SERIAL = 2,
    MAX_PACKET_SIZE = 2048,
    MAX_PACKETS = 20,
    MAX_ADDRESSES = 4,
};

#endif
