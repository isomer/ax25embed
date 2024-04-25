/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Additional platform apis that rely on posix
 */
#define _POSIX_C_SOURCE 200809L
#include "platform-posix.h"
#include "debug.h"
#include "clock.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TIME_BASE INT64_C(1000000000)

static ticker_t *tickers = NULL;
static fd_event_t *fd_events = NULL;

void register_ticker(ticker_t *ticker) {
    ticker->next = tickers;
    tickers = ticker;
}

void register_fd_event(fd_event_t *event) {
    event->next = fd_events;
    fd_events = event;
}

instant_t instant_now(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        panic("clock_gettime(CLOCK_MONOTONIC)");
    }

    return (instant_t) { .instant = ts.tv_sec * TIME_BASE + ts.tv_nsec, };
}

void debug_putch(char ch) {
    putchar(ch);
}

void debug_putbuf(const char *buf, size_t buflen) {
    for(size_t i=0; i<buflen; ++i) {
        debug_putch(buf[i]);
    }
}

void panic(const char *msg) {
    DEBUG(STR(msg));
    abort();
}

static duration_t platform_run_tickers(void) {
    duration_t wait = duration_seconds(3600); /* Always tick at least every hour */
    for(ticker_t *ticker = tickers; ticker; ticker=ticker->next) {
        duration_t time = ticker->tick();
        wait = duration_min(wait, time);
    }
    return wait;
}

static fd_set platform_get_fds(int *maxfd) {
    fd_set fds;
    *maxfd = 0;
    FD_ZERO(&fds);
    for(fd_event_t *event = fd_events; event; event=event->next) {
        FD_SET(event->fd, &fds);
        if (event->fd > *maxfd)
            *maxfd = event->fd;
    }
    return fds;
}

static void platform_run_fds(fd_set *fds) {
    for(fd_event_t *event = fd_events; event; event=event->next) {
        if (FD_ISSET(event->fd, fds)) {
            event->callback();
        }
    }
}

void platform_run(void) {
    duration_t wait;
    for (;;) {
        int maxfd;
        fd_set rfd = platform_get_fds(&maxfd);
        wait = platform_run_tickers();
        int64_t wait_us = duration_as_micros(wait);
        struct timeval timeout = { .tv_sec = wait_us / 1000000, .tv_usec = wait_us % 1000000 };
        select(maxfd+1, &rfd, NULL, NULL, &timeout);
        platform_run_fds(&rfd);
    }

}

void platform_init(void) {
    /* Nothing to do for POSIX */
}
