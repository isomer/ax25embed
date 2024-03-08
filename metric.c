#include "metric.h"
#include "platform.h"
#include <stdint.h>

static uint32_t metrics[MAX_METRIC] = {0, };


void metric_inc(metric_t metric) {
    metric_inc_by(metric, 1);
}

void metric_inc_by(metric_t metric, size_t count) {
    if (metric < 0 || metric >= MAX_METRIC)
        panic();
    metrics[metric] += count;
}
