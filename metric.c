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
#undef NAME
};


void metric_inc(metric_t metric) {
    metric_inc_by(metric, 1);
}

void metric_inc_by(metric_t metric, size_t count) {
    if (metric < 0 || metric >= MAX_METRIC)
        panic("metric out of range");
    debug(metric_name[metric]);
    metrics[metric] += count;
}
