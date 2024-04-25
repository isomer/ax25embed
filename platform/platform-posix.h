/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * POSIX specific code.
 */
#ifndef PLATFORM_POSIX_H
#define PLATFORM_POSIX_H
#include "platform.h"

typedef struct fd_event_t {
    struct fd_event_t *next;
    int fd;
    void (*callback)(void);
} fd_event_t;

/* Register a function to be called when a FD has a read event */
void register_fd_event(fd_event_t *event);

#endif

