/* (C) Copyright 2024 Perry Lorier (2E0ITB)
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Metrics.
 *
 * Metrics are counters that can be used to see how many times a particular event occurs.
 */
#include "metric.h"
#include "platform.h"
#include <stdint.h>

static uint32_t metrics[MAX_METRIC] = {0, };

static const char *metric_name[MAX_METRIC] = {
#define NAME(x) [METRIC_##x] = #x
    NAME(OVERRUN),
    NAME(UNDERRUN),
    NAME(BAD_ESCAPE),
    NAME(UNKNOWN_KISS_COMMAND),
    NAME(INVALID_ADDR),
    NAME(NOT_ME),
    NAME(NOT_ME_BYTES),
    NAME(REFUSED_DIGIPEAT),
    NAME(UNKNOWN_FRAME),
    NAME(TEST_RESPONSE_RECV),
    NAME(TEST_COMMAND_RECV),
    NAME(TEST_COMMAND_SENT),
    NAME(LEGACY_TYPE_RECV),
    NAME(NO_PACKETS),
    NAME(PACKETS_ALLOCATED),
    NAME(PACKETS_FREED),
    NAME(NOT_COMMAND),
    NAME(SABM_SUCCESS),
    NAME(SABM_FAIL),
    NAME(NO_CONNS),
    NAME(KISS_XMIT),
    NAME(KISS_XMIT_BYTES),
    NAME(BUFFER_ALLOC_SUCCESS),
    NAME(BUFFER_ALLOC_FAIL),
    NAME(BUFFER_FREE),
#undef NAME
};


void metric_inc(metric_t metric) {
    metric_inc_by(metric, 1);
}

void metric_inc_by(metric_t metric, size_t count) {
    if (metric < 0 || metric >= MAX_METRIC)
        panic("metric out of range");
    metrics[metric] += count;
}
